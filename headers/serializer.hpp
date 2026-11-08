#pragma once

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <stdexcept>
#include "bytecode.hpp"

using std::runtime_error;
using std::vector;
using std::string;
using std::ofstream;
using std::ifstream;
using std::holds_alternative;

/*
    .vkc File Format
    ------------------
    [4 bytes] Magic Number ("VEKL")
    [4 bytes] Code size (uint32)
    [N bytes] Code array
    [4 bytes] Constant count (uint32)
    [N bytes] Constants (tag + data)
    [4 bytes] Function count (uint32)
    [N bytes] Functions (name length + name + address)
*/

class Serializer {
private:
    static constexpr uint32_t MAGIC_NUMBER = 0x56454B4C; // VEKL

    // Write helpers
    static void write_u8(ofstream& out, uint8_t val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_u32(ofstream& out, uint32_t val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_i32(ofstream& out, int32_t val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_i64(ofstream& out, int64_t val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_f32(ofstream& out, float val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_f64(ofstream& out, double val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_bool(ofstream& out, bool val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_char(ofstream& out, char val) {
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    static void write_string(ofstream& out, const std::string& str) {
        write_u32(out, static_cast<uint32_t>(str.size()));
        out.write(str.data(), str.size());
    }

    // Read helpers
    static uint8_t read_u8(ifstream& in) {
        uint8_t val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static uint32_t read_u32(ifstream& in) {
        uint32_t val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static int32_t read_i32(ifstream& in) {
        int32_t val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static int64_t read_i64(ifstream& in) {
        int64_t val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static float read_f32(ifstream& in) {
        float val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static double read_f64(ifstream& in) {
        double val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static bool read_bool(ifstream& in) {
        bool val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static char read_char(ifstream& in) {
        char val; in.read(reinterpret_cast<char*>(&val), sizeof(val)); return val;
    }

    static std::string read_string(ifstream& in) {
        uint32_t len = read_u32(in);
        std::string str(len, '\0');
        in.read(str.data(), len);
        return str;
    }
public:
    static void save(const Bytecode& bytecode, const string& path) {
        ofstream file(path, std::ios::binary);
        if(!file) throw runtime_error("Could not open file for writing: " + path);

        // Write magic number
        write_u32(file, MAGIC_NUMBER);

        // Code
        write_u32(file, static_cast<uint32_t>(bytecode.code.size()));
        file.write(reinterpret_cast<const char*>(bytecode.code.data()), bytecode.code.size());

        // Constants
        write_u32(file, static_cast<uint32_t>(bytecode.constants.size()));
        for (const auto& val : bytecode.constants) {
            write_u8(file, static_cast<uint8_t>(val.index()));

            if (std::holds_alternative<int32_t>(val)) {
                write_i32(file, std::get<int32_t>(val));
            } else if (std::holds_alternative<int64_t>(val)) {
                write_i64(file, std::get<int64_t>(val));
            } else if (std::holds_alternative<float>(val)) {
                write_f32(file, std::get<float>(val));
            } else if (std::holds_alternative<double>(val)) {
                write_f64(file, std::get<double>(val));
            } else if (std::holds_alternative<bool>(val)) {
                write_bool(file, std::get<bool>(val));
            } else if (std::holds_alternative<char>(val)) {
                write_char(file, std::get<char>(val));
            } else if (std::holds_alternative<std::string>(val)) {
                write_string(file, std::get<std::string>(val));
            }
        }

        // Functions
        write_u32(file, static_cast<uint32_t>(bytecode.function_addresses.size()));
        for (const auto& [name, addr] : bytecode.function_addresses) {
            write_string(file, name);
            write_u32(file, static_cast<uint32_t>(addr));
        }
    }

    static Bytecode load(const string& path) {
        ifstream file(path, std::ios::binary);
        if(!file) throw runtime_error("Could not open file for reading: " + path);

        Bytecode bytecode;

        // Check magic number
        if (read_u32(file) != MAGIC_NUMBER) {
            throw runtime_error("Invalid VekLang bytecode file.");
        }

        // Code
        uint32_t code_size = read_u32(file);
        bytecode.code.resize(code_size);
        file.read(reinterpret_cast<char*>(bytecode.code.data()), code_size);

        // Constants
        uint32_t const_count = read_u32(file);
        for (uint32_t i = 0; i < const_count; i++) {
            uint8_t tag = read_u8(file);

            if (tag == 0) {
                bytecode.constants.push_back(std::monostate{});
            } else if (tag == 1) {
                bytecode.constants.push_back(read_i32(file));
            } else if (tag == 2) {
                bytecode.constants.push_back(read_i64(file));
            } else if (tag == 3) {
                bytecode.constants.push_back(read_f32(file));
            } else if (tag == 4) {
                bytecode.constants.push_back(read_f64(file));
            } else if (tag == 5) {
                bytecode.constants.push_back(read_bool(file));
            } else if (tag == 6) {
                bytecode.constants.push_back(read_char(file));
            } else if (tag == 7) {
                bytecode.constants.push_back(read_string(file));
            }
        }

        // Functions
        uint32_t func_count = read_u32(file);
        for (uint32_t i = 0; i < func_count; i++) {
            string name = read_string(file);
            uint32_t address = read_u32(file);
            bytecode.function_addresses[name] = address;
        }

        return bytecode;
    }
};