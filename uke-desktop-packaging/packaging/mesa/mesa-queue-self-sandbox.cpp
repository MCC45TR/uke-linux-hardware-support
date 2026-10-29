// SPDX-License-Identifier: MIT
// Exercise real Mesa jobs under Chromium's self-only scheduling contracts.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <array>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>
#include "util/u_queue.h"

#if defined(__x86_64__)
constexpr uint32_t audit_arch = AUDIT_ARCH_X86_64;
#elif defined(__aarch64__)
constexpr uint32_t audit_arch = AUDIT_ARCH_AARCH64;
#else
#error This Linux scheduling fixture supports x86_64 and AArch64.
#endif

static std::atomic<unsigned> rewrites{0};
static_assert(std::atomic<unsigned>::is_always_lock_free);

static void check(bool condition, const char *message)
{
   if (!condition) {
      std::fprintf(stderr, "FAIL: %s (errno=%d)\n", message, errno);
      _exit(1);
   }
}

static void scheduling_trap(int, siginfo_t *info, void *opaque)
{
   auto *context = static_cast<ucontext_t *>(opaque);
   unsigned long target, policy, param;
#if defined(__x86_64__)
   target = context->uc_mcontext.gregs[REG_RDI];
   policy = context->uc_mcontext.gregs[REG_RSI];
   param = context->uc_mcontext.gregs[REG_RDX];
#else
   target = context->uc_mcontext.regs[0];
   policy = context->uc_mcontext.regs[1];
   param = context->uc_mcontext.regs[2];
#endif
   if (info->si_syscall != __NR_sched_setscheduler)
      _exit(98);
   // SIGSYSSchedHandler rewrites only the calling thread's TID to pid0.
   if (target != static_cast<unsigned long>(syscall(__NR_gettid))) {
      constexpr char message[] = "sibling-scheduler-request rejected\n";
      syscall(__NR_write, STDERR_FILENO, message, sizeof(message) - 1);
      _exit(99);
   }
   const int saved_errno = errno;
   long result = syscall(__NR_sched_setscheduler, 0, policy, param);
   if (result == -1)
      result = -errno;
   errno = saved_errno;
   rewrites.fetch_add(1, std::memory_order_relaxed);
#if defined(__x86_64__)
   context->uc_mcontext.gregs[REG_RAX] = result;
#else
   context->uc_mcontext.regs[0] = result;
#endif
}

static void install_scheduling_filter()
{
   struct sigaction action{};
   action.sa_sigaction = scheduling_trap;
   action.sa_flags = SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   check(sigaction(SIGSYS, &action, nullptr) == 0, "install SIGSYS handler");
   // Other GPU restrictions are outside this fixture; no browser runs here.
   sock_filter instructions[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, audit_arch, 1, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_sched_setscheduler, 3, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_setpriority, 7, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_getpriority, 6, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, args[0])),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, 2, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, static_cast<uint32_t>(getpid()), 1, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, args[0])),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, PRIO_PROCESS, 1, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP),
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, args[1])),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0, 2, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, static_cast<uint32_t>(getpid()), 1, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
   };
   sock_fprog program{static_cast<unsigned short>(std::size(instructions)), instructions};
   check(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "set NoNewPrivs");
   check(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "install seccomp filter");
}

struct Job {
   unsigned index = 0;
   unsigned result = 0;
   int policy = -1, nice = -100, seccomp = -1;
   pid_t tid = 0;
   int explicit_nice = -100, explicit_errno = 0;
};

static void execute(void *opaque, void *global, int)
{
   auto &job = *static_cast<Job *>(opaque);
   job.policy = sched_getscheduler(0);
   errno = 0;
   job.nice = getpriority(PRIO_PROCESS, 0);
   job.seccomp = prctl(PR_GET_SECCOMP, 0, 0, 0, 0);
   job.tid = syscall(__NR_gettid);
   errno = 0;
   job.explicit_nice = getpriority(PRIO_PROCESS, job.tid);
   job.explicit_errno = errno;
   job.result = job.index + 1;
   static_cast<std::atomic<unsigned> *>(global)->fetch_add(1, std::memory_order_relaxed);
}

static void exercise(bool minimum, bool sandboxed)
{
   alarm(10);
   const int parent_policy = sched_getscheduler(0);
   const int parent_nice = getpriority(PRIO_PROCESS, 0);
   const int parent_seccomp = prctl(PR_GET_SECCOMP, 0, 0, 0, 0);
   if (sandboxed)
      install_scheduling_filter();
   std::atomic<unsigned> completed{0};
   util_queue queue{};
   const unsigned flags = minimum ? UTIL_QUEUE_INIT_USE_MINIMUM_PRIORITY : 0;
   check(util_queue_init(&queue, "sandbox", 8, 4, flags, &completed), "create Mesa queue");
   util_queue_adjust_num_threads(&queue, 4);
   check(queue.num_threads == 4, "create all requested workers");
   for (unsigned round = 0; round < 2; ++round) {
      std::array<Job, 64> jobs;
      std::array<util_queue_fence, 64> fences;
      for (unsigned i = 0; i < jobs.size(); ++i) {
         jobs[i].index = i;
         util_queue_fence_init(&fences[i]);
         util_queue_add_job(&queue, &jobs[i], &fences[i], execute, nullptr, sizeof(Job));
      }
      for (unsigned i = 0; i < jobs.size(); ++i) {
         util_queue_fence_wait(&fences[i]);
         check(jobs[i].result == i + 1, "job completed with the expected result");
         check(jobs[i].policy == (minimum ? SCHED_BATCH : parent_policy), "worker policy retained");
         check(jobs[i].nice == (minimum ? 19 : parent_nice), "worker niceness retained");
         check(jobs[i].seccomp == (sandboxed ? 2 : parent_seccomp), "worker inherited the expected filter");
         check(jobs[i].tid != getpid(), "job ran on a worker");
         check(sandboxed ? jobs[i].explicit_nice == -1 && jobs[i].explicit_errno == EPERM
                         : jobs[i].explicit_nice == jobs[i].nice && jobs[i].explicit_errno == 0,
               "explicit worker priority obeys the target restriction");
         util_queue_fence_destroy(&fences[i]);
      }
      util_queue_finish(&queue);
      if (round == 0) {
         util_queue_adjust_num_threads(&queue, 2);
         util_queue_adjust_num_threads(&queue, 4);
         check(queue.num_threads == 4, "recreate workers after shrink");
      }
   }
   util_queue_destroy(&queue);
   check(completed.load() == 128, "every job executed exactly once");
   check(sched_getscheduler(0) == parent_policy && getpriority(PRIO_PROCESS, 0) == parent_nice,
         "parent scheduling unchanged");
   std::printf("PASS: minimum=%u filter=%u jobs=128 self-rewrites=%u\n",
               minimum, sandboxed, rewrites.load());
   std::fflush(stdout);
   _exit(0);
}

int main()
{
   unsigned failures = 0;
   for (bool sandboxed : {false, true}) {
      for (bool minimum : {false, true}) {
         std::fflush(nullptr);
         const pid_t child = fork();
         check(child >= 0, "fork scheduling case");
         if (!child)
            exercise(minimum, sandboxed);
         int status = 0;
         pid_t result;
         do { result = waitpid(child, &status, 0); } while (result < 0 && errno == EINTR);
         check(result == child, "wait for scheduling case");
         if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            std::fprintf(stderr, "FAIL: minimum=%u filter=%u child-status=%d\n", minimum, sandboxed, status);
            ++failures;
         }
      }
   }
   return failures ? 1 : 0;
}
