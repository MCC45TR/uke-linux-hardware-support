// SPDX-License-Identifier: GPL-2.0-or-later
// COPR-only comparison of checkpolicy's binary-policy CIL representation.
// Preserve Boolean branches, names, label order and every unrecognized statement.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

struct Node {
    bool list = false;
    std::string atom;
    std::vector<Node> items;
};

class Parser {
    std::istream &in;
    std::size_t nodes = 0;
    void space() {
        while (in) {
            if (std::isspace(static_cast<unsigned char>(in.peek()))) in.get();
            else if (in.peek() == ';') {
                std::string ignored;
                std::getline(in, ignored);
            } else break;
        }
    }
    Node read(unsigned depth) {
        if (depth > 512 || ++nodes > 100000) throw std::runtime_error("expression limit");
        space();
        Node n;
        if (in.peek() == '(') {
            in.get(); n.list = true;
            for (;;) {
                space();
                if (in.peek() == ')') { in.get(); return n; }
                if (!in || in.peek() == EOF) throw std::runtime_error("unclosed list");
                n.items.push_back(read(depth + 1));
            }
        }
        if (in.peek() == '"') {
            n.atom += static_cast<char>(in.get());
            bool escaped = false;
            for (;;) {
                const int c = in.get();
                if (c == EOF) throw std::runtime_error("unclosed string");
                n.atom += static_cast<char>(c);
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') return n;
            }
        }
        while (in && in.peek() != EOF && in.peek() != '(' && in.peek() != ')' &&
               !std::isspace(static_cast<unsigned char>(in.peek())))
            n.atom += static_cast<char>(in.get());
        if (n.atom.empty()) throw std::runtime_error("empty atom");
        return n;
    }
public:
    explicit Parser(std::istream &source) : in(source) {}
    bool next(Node &n) {
        space();
        if (in.peek() == EOF) return false;
        nodes = 0; n = read(0); return true;
    }
};

std::string render(const Node &n) {
    if (!n.list) return n.atom;
    std::string result = "(";
    for (const auto &item : n.items) {
        if (result.size() != 1) result += ' ';
        result += render(item);
    }
    return result + ")";
}

std::string atom(const Node &n) {
    if (n.list || n.atom.empty()) throw std::runtime_error("expected atom");
    return n.atom;
}

void sortSet(Node &n) {
    if (!n.list) throw std::runtime_error("expected set");
    for (const auto &item : n.items) (void)atom(item);
    std::sort(n.items.begin(), n.items.end(), [](const auto &a, const auto &b) { return a.atom < b.atom; });
    n.items.erase(std::unique(n.items.begin(), n.items.end(), [](const auto &a, const auto &b) {
        return a.atom == b.atom;
    }), n.items.end());
}

class Canonical {
    const bool old;
    std::map<std::string, Node> classes, commons;
    std::map<std::string, std::string> associations;
    std::map<std::string, unsigned> labelSequence;
    const std::set<std::string> labels{"genfscon", "portcon", "netifcon", "nodecon", "fsuse",
        "ibpkeycon", "ibendportcon", "sidcontext"};
    const std::map<std::string, std::set<std::string>> added{
        {"system", {"firmware_load", "kexec_image_load", "kexec_initramfs_load", "policy_load", "x509_certificate_load"}},
        {"bpf", {"map_create_as", "prog_load_as"}}, {"io_uring", {"allowed"}}};
    void permissions(const std::string &name, Node &n) const {
        sortSet(n);
        if (old && added.contains(name)) {
            const auto &remove = added.at(name);
            std::erase_if(n.items, [&](const auto &p) { return remove.contains(p.atom); });
        }
    }
public:
    explicit Canonical(bool original) : old(original) {}
    void emit(Node n, const std::string &condition = "") {
        if (!n.list || n.items.empty()) throw std::runtime_error("expected statement");
        const auto tag = atom(n.items[0]);
        if (tag == "class" || tag == "common") {
            if (!condition.empty() || n.items.size() != 3) throw std::runtime_error("class shape");
            auto &table = tag == "class" ? classes : commons;
            if (!table.emplace(atom(n.items[1]), n.items[2]).second) throw std::runtime_error("duplicate declaration");
            return;
        }
        if (tag == "classcommon") {
            if (!condition.empty() || n.items.size() != 3 ||
                !associations.emplace(atom(n.items[1]), atom(n.items[2])).second)
                throw std::runtime_error("common association shape");
            return;
        }
        if (tag == "booleanif") {
            if (!condition.empty() || n.items.size() < 3) throw std::runtime_error("Boolean shape");
            for (std::size_t i = 2; i < n.items.size(); ++i) {
                const auto &branch = n.items[i];
                if (!branch.list || branch.items.empty()) throw std::runtime_error("Boolean branch");
                const auto value = atom(branch.items[0]);
                if (value != "true" && value != "false") throw std::runtime_error("Boolean value");
                for (std::size_t j = 1; j < branch.items.size(); ++j)
                    emit(branch.items[j], "when " + render(n.items[1]) + " " + value + " ");
            }
            return;
        }
        if (tag == "allow" || tag == "auditallow" || tag == "dontaudit") {
            if (n.items.size() != 4 || !n.items[3].list || n.items[3].items.size() != 2)
                throw std::runtime_error("AV rule shape");
            auto &access = n.items[3];
            const auto name = atom(access.items[0]);
            if (old && name == "memfd_file") throw std::runtime_error("unexpected new-class AV rule");
            permissions(name, access.items[1]);
            if (access.items[1].items.empty()) return;
        } else if (tag == "typeattributeset" && n.items.size() == 3 && n.items[2].list) {
            if (std::all_of(n.items[2].items.begin(), n.items[2].items.end(), [](const auto &x) { return !x.list; }))
                sortSet(n.items[2]);
        } else if (old && tag == "classorder") {
            if (n.items.size() != 2 || !n.items[1].list) throw std::runtime_error("class order shape");
            std::erase_if(n.items[1].items, [](const auto &x) { return atom(x) == "memfd_file"; });
        }
        if (labels.contains(tag))
            std::cout << "label " << tag << " " << ++labelSequence[tag] << " ";
        std::cout << condition << render(n) << '\n';
    }
    void finish() {
        for (auto &[name, set] : classes) {
            if (old && name == "memfd_file") continue;
            if (associations.contains(name)) {
                const auto &extra = commons.at(associations.at(name));
                if (!extra.list || !set.list) throw std::runtime_error("permission set shape");
                set.items.insert(set.items.end(), extra.items.begin(), extra.items.end());
            }
            permissions(name, set);
            std::cout << "(class " << name << " " << render(set) << ")\n";
        }
        for (const auto &[name, unused] : associations) {
            (void)unused;
            if (!classes.contains(name)) throw std::runtime_error("unknown class association");
        }
    }
};

int main(int argc, char **argv) {
    try {
        if (argc != 2 || (std::string(argv[1]) != "full" && std::string(argv[1]) != "old"))
            throw std::runtime_error("usage: policy-canonical full|old < binary-policy.cil");
        Parser parser(std::cin);
        Canonical canonical(std::string(argv[1]) == "old");
        Node n;
        while (parser.next(n)) canonical.emit(std::move(n));
        canonical.finish();
        if (!std::cout) throw std::runtime_error("output failure");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
