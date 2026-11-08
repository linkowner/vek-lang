#include <iostream>

#include "./headers/vm.hpp"
#include "./headers/serializer.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: vekvm <file.vekbc>\n";
        return 1;
    }

    std::string path = argv[1];
    Bytecode bytecode = Serializer::load(path);
    
    VM vm;
    vm.run(bytecode);
    return 0;
}