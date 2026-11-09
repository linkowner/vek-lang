#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <variant>
#include <stdexcept>
#include <unordered_map>
#include "./token.hpp"

using std::variant;
using std::string;
using std::vector;
using std::unordered_map;
using std::shared_ptr;

struct ArrayObject;

using Value = std::variant<std::monostate, int32_t, int64_t, float, double, bool, char, string, shared_ptr<ArrayObject>>;

struct ArrayObject {
    vector<Value> elements;
};

enum class OpCode {
    // Stack and Math
    OP_CONSTANT = 0x01,
    OP_ADD = 0x02,
    OP_SUBTRACT = 0x03,
    OP_MULTIPLY = 0x04,
    OP_DIVIDE = 0x05,

    // Logic
    OP_GREATER_THAN = 0x06,
    OP_LESS_THAN = 0x07,
    OP_GREATER_EQUAL = 0x08,
    OP_LESS_EQUAL = 0x09,
    OP_EQUAL = 0x0A,
    OP_NOT = 0x0B,

    // Vars
    OP_SAVE = 0x10,
    OP_LOAD = 0x11,
    OP_DEFINE_VAR = 0x12,
    OP_POP = 0x13,
    OP_ENTER_SCOPE = 0x14,
    OP_EXIT_SCOPE = 0x15,
    OP_CAST = 0x16,
    OP_DUPLICATE = 0x17,

    // Control Flow
    OP_RETURN = 0x20,
    OP_CALL = 0x21,
    OP_JUMP = 0x22,
    OP_JUMP_IF_FALSE = 0x23,
    OP_JUMP_IF_TRUE = 0x24,

    // System Functions
    OP_PRINT = 0x30,

    // Arrays
    OP_NEW_ARRAY = 0x40,
    OP_GET_INDEX = 0x41,
    OP_SET_INDEX = 0x42, 
    OP_ARRAY_PUSH = 0x43,
    OP_ARRAY_POP = 0x44,

    // End of code
    OP_HALT = 0xFF,
};

struct Bytecode
{
    // Operand encoding used by the compiler and the VM (they must agree):
    //   - constant-pool indices: 16-bit big-endian (high byte first)
    //   - jump targets:          16-bit big-endian absolute code offset
    //
    // Constant indices used to be a single byte, which meant that any program
    // with more than 255 constants had its indices silently truncated by
    // `static_cast<uint8_t>`: two different constants ended up sharing one
    // byte and the VM simply executed the wrong value, with no error or crash.
    // The operand is now 16 bits wide, and the remaining ceilings are enforced
    // loudly (see add_const / emit_jump / patch_jump) so a program that is too
    // big gets a clear compile error instead of silently wrong behaviour.
    static constexpr size_t MAX_CONSTANTS = 65535;
    static constexpr size_t MAX_CODE_SIZE = 65536;

    vector<uint8_t> code;
    vector<Value> constants;
    unordered_map<string, size_t> function_addresses;

    void write(uint8_t byte) {
        code.push_back(byte);
    }

    // 16-bit big-endian write, used for constant indices and jump targets.
    void write_u16(uint16_t val) {
        write(static_cast<uint8_t>(val >> 8));
        write(static_cast<uint8_t>(val & 0xFF));
    }

    // 16-bit big-endian read at an absolute code offset.
    uint16_t read_u16(size_t offset) const {
        return static_cast<uint16_t>((code[offset] << 8) | code[offset + 1]);
    }

    int add_const(Value val) {
        if (constants.size() >= MAX_CONSTANTS) {
            throw std::runtime_error(
                "Too many constants (limit " + std::to_string(MAX_CONSTANTS) +
                "). The constant pool is full; try splitting the program into modules.");
        }
        constants.push_back(val);
        return static_cast<int>(constants.size() - 1);
    }

    // Fetch a constant by index, rejecting out-of-range indices. Also guards
    // the VM against corrupt or truncated .vkc files.
    const Value& get_const(size_t index) const {
        if (index >= constants.size()) {
            throw std::runtime_error(
                "Constant index " + std::to_string(index) + " is out of range (pool size " +
                std::to_string(constants.size()) + ")");
        }
        return constants[index];
    }

    void check_jump_target(size_t target) const {
        if (target >= MAX_CODE_SIZE) {
            throw std::runtime_error(
                "Program too large: jump target " + std::to_string(target) +
                " exceeds the 16-bit address space (limit " +
                std::to_string(MAX_CODE_SIZE - 1) + "). Split the code into smaller functions or modules.");
        }
    }

    template <typename... Args> // Function overloading would look a little bit weird there
    void emit(OpCode op, Args... args) {
        write(static_cast<uint8_t>(op));
        (write(static_cast<uint8_t>(args)), ...); // Fold expression (c++17)
    }

    // Emit an opcode whose operand is a 16-bit constant-pool index.
    void emit_const(OpCode op, int index) {
        if (index < 0 || static_cast<size_t>(index) >= MAX_CONSTANTS) {
            throw std::runtime_error("Constant index " + std::to_string(index) + " is out of range");
        }
        write(static_cast<uint8_t>(op));
        write_u16(static_cast<uint16_t>(index));
    }

    // Emit a jump opcode with an absolute 16-bit target.
    void emit_jump(OpCode op, size_t target) {
        check_jump_target(target);
        write(static_cast<uint8_t>(op));
        write_u16(static_cast<uint16_t>(target));
    }

    // Emit a jump opcode with a placeholder target. Returns the offset of the
    // two placeholder bytes, to be filled in later with patch_jump().
    size_t emit_jump(OpCode op) {
        write(static_cast<uint8_t>(op));
        write(0);
        write(0);
        return code.size() - 2;
    }

    // Fill in the target of a placeholder jump emitted by emit_jump().
    void patch_jump(size_t jump_index, size_t target) {
        check_jump_target(target);
        code[jump_index] = static_cast<uint8_t>((target >> 8) & 0xFF);
        code[jump_index + 1] = static_cast<uint8_t>(target & 0xFF);
    }
};
