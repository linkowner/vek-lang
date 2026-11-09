#include <iostream>

#include "./headers/vm.hpp"
#include "./headers/serializer.hpp"

static void print_usage() {
    std::cerr << "Usage: vekvm <file.vkc>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string path = argv[1];

    try {
        Bytecode bytecode = Serializer::load(path);

        VM vm;
        vm.run(bytecode);
    } catch (const std::exception& e) {
        std::cerr << "\n[Error] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
