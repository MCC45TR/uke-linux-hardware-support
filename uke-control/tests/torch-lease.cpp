// SPDX-License-Identifier: MIT
#include "torch-lease.hpp"
#include <cassert>
#include <iostream>
int main() {
    uke::torch::Lease lease;
    assert(!lease.expired(100));
    lease.applied(true,100);
    assert(lease.remaining(100)==60000000);
    for(uint64_t now=100;now<60000100;now+=1000) {
        lease.applied(true,now);
        assert(!lease.expired(now));
    }
    assert(lease.expired(60000100));
    lease.applied(true,60000101);
    assert(lease.expired(60000101)); // a flood cannot renew a continuous lease
    lease.applied(false,60000101);
    assert(!lease.expired(60000101));
    lease.applied(true,60000102);
    assert(lease.remaining(60000102)==60000000);
    assert(lease.expired(900000000)); // includes a suspended interval
    std::cout<<"Torch continuous-on lease boundaries and request flood passed\n";
}
