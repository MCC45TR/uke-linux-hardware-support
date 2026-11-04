// SPDX-License-Identifier: MIT
#include "../src/rollback.hpp"
#include <cassert>
int main() {
    assert(uke::rollback::plan({"rollback","status"}).back()=="status");
    for(const std::string mode:{"enforced","permissive","off"}) {
        auto plan=uke::rollback::plan({"set","rollback-"+mode});
        assert(plan==std::vector<std::string>({"/usr/bin/uke-kernel-update","rollback-policy","set",mode}));
    }
    for(const std::vector<std::string> words:std::vector<std::vector<std::string>>{
            {"set"},{"set","rollback-on"},{"set","rollback-off","extra"},{"rollback","off"}}) {
        bool refused=false;try{uke::rollback::plan(words);}catch(const uke::cli::UsageError &){refused=true;}assert(refused);
    }
}
