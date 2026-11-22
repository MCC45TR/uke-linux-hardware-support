// SPDX-License-Identifier: MIT
#include "torch.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>
int main(int argc,char** argv) {
    try {
        if(argc!=1 && !(argc==2 && std::string_view(argv[1])=="--remove"))
            throw std::runtime_error("Unexpected torch permission argument");
        return uke::torch::prepare_access(argc==2);
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
