// The VEK compiler
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <string>
#include <vector>
#include <unordered_map>

#include "./headers/lexer.hpp"
#include "./headers/parser.hpp"
#include "./headers/compiler.hpp"
#include "./headers/paths.hpp"
#include "./headers/vm.hpp"
#include "./headers/serializer.hpp"

namespace fs = std::filesystem;
using std::runtime_error;

using std::cerr;
using std::cout;
using std::cin;
using std::endl;

using std::string;
using std::vector;

using std::ifstream;
using std::stringstream;

string read_file(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Can't open file: " + path);
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "====================================================" << endl;
        cerr << "Usage:" << endl;
        cerr << "vek build <file.vek> - Compile for release" << endl;
        cerr << "vek run <file.vek>   - Run development version" << endl;
        cerr << "====================================================" << endl;

        return 1;
    }

    string command = argv[1];
    string path = argv[2];

    try {
        // Read source
        std::string source = read_file(path);

        // Compile
        Lexer lexer(source);
        auto tokens = lexer.lex();
        Parser parser(tokens);
        auto ast = parser.parse_program();
        Compiler compiler(path);
        Bytecode bytecode = compiler.compile(ast);

        // Handle the command
        if (command == "build") {
            // Save to a .vekbc next to the source file
            fs::path p(path);
            fs::path out_path = p.parent_path() / (p.stem().string() + ".vkc");

            Serializer::save(bytecode, out_path.string());
            std::cout << "Successfully compiled to " << out_path.string() << "\n";
            
        } else if (command == "run") {
            // Execute
            VM vm;
            vm.run(bytecode);
            
        } else {
            std::cerr << "Unknown command: " << command << "\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "\n[Error] " << e.what() << "\n";
        return 1;
    }

    return 0;
}