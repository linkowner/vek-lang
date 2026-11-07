/*
    Lot's of section headers just for visual 
    0xFF - are mostly placeholders.
*/

#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "ast.hpp"
#include "bytecode.hpp"
#include "token.hpp"

using std::vector;
using std::unique_ptr;
using std::move;
using std::string;
using std::unordered_map;

enum class Type { INT32, INT64, FLOAT32, FLOAT64, BOOL, CHAR, STR, VOID, ARRAY, UNKNOWN };

class Compiler {
private:
    unordered_map<string, FuncDecl*> functions;
    unordered_map<string, Type> function_return_types;
    unordered_map<string, Type> variable_types;
    unordered_map<string, bool> is_constant;

    // Helpers
    Type token_to_type(TokenType t) {
        switch(t) {
            case TokenType::INT32: return Type::INT32;
            case TokenType::INT64: return Type::INT64;
            case TokenType::FLOAT32: return Type::FLOAT32;
            case TokenType::FLOAT64: return Type::FLOAT64;
            case TokenType::BOOL: return Type::BOOL;
            case TokenType::CHAR: return Type::CHAR;
            case TokenType::STR: return Type::STR;
            case TokenType::VOID: return Type::VOID;
            default: return Type::UNKNOWN;
        }
    }
    
    string type_to_string(Type t) { // Going to be useful later
        switch(t) {
            case Type::INT32: return "int32"; 
            case Type::INT64: return "int64";
            case Type::FLOAT32: return "float32"; 
            case Type::FLOAT64: return "float64";
            case Type::BOOL: return "bool"; 
            case Type::CHAR: return "char";
            case Type::STR: return "str"; 
            case Type::VOID: return "void";
            case Type::ARRAY: return "array";
            default: return "unknown";
        }
    }

    bool is_numeric(Type t) {
        return t == Type::INT32 || t == Type::INT64 || t == Type::FLOAT32 || t == Type::FLOAT64;
    }

    bool is_assignable(Type target, Type source) {
        if (target == source) return true;
        if (is_numeric(target) && is_numeric(source)) return true;
        return false;
    }

    // Backpatch jump address
    void patch_jump(Bytecode& bytecode, size_t jump_index, size_t target) {
        bytecode.code[jump_index] = (target >> 8) & 0xFF;
        bytecode.code[jump_index + 1] = target & 0xFF;
    }

    // Compilers
    void compile_if(IfStmt* stmt, Bytecode& bytecode) {
        compile_expr(stmt->condition.get(), bytecode, Type::BOOL);
            
        bytecode.write((uint8_t)OpCode::OP_JUMP_IF_FALSE);
        size_t jump_if_false = bytecode.code.size();
        bytecode.write(0xFF); bytecode.write(0xFF);
        
        compile_stmt(stmt->then_branch.get(), bytecode);
        
        if (stmt->else_branch) {
            bytecode.write((uint8_t)OpCode::OP_JUMP);
            size_t jump_over_else = bytecode.code.size();
            bytecode.write(0xFF); bytecode.write(0xFF);
            
            patch_jump(bytecode, jump_if_false, bytecode.code.size());
            compile_stmt(stmt->else_branch.get(), bytecode);
            patch_jump(bytecode, jump_over_else, bytecode.code.size());
        } else {
            patch_jump(bytecode, jump_if_false, bytecode.code.size());
        }
    }

    void compile_while(WhileStmt* stmt, Bytecode& bytecode) {
        size_t loop_start = bytecode.code.size();
        compile_expr(stmt->condition.get(), bytecode, Type::BOOL);

        bytecode.write((uint8_t)OpCode::OP_JUMP_IF_FALSE);
        size_t exit_jump = bytecode.code.size();
        bytecode.write(0xFF); bytecode.write(0xFF);

        compile_stmt(stmt->body.get(), bytecode);

        // Repeat
        bytecode.write((uint8_t)OpCode::OP_JUMP);
        bytecode.write((loop_start >> 8) & 0xFF); // High byte
        bytecode.write(loop_start & 0xFF); // Low byte

        // After the loop
        patch_jump(bytecode, exit_jump, bytecode.code.size());
    }

    void compile_for(ForStmt* stmt, Bytecode& bytecode) {
        if (stmt->initializer) {
            compile_stmt(stmt->initializer.get(), bytecode);
        }

        size_t loop_start = bytecode.code.size();

        // Condition
        size_t exit_jump = 0;
        if (stmt->condition) {
            compile_expr(stmt->condition.get(), bytecode, Type::BOOL);
            bytecode.write((uint8_t)OpCode::OP_JUMP_IF_FALSE);
            exit_jump = bytecode.code.size();
            bytecode.write(0xFF); bytecode.write(0xFF);
        }

        compile_stmt(stmt->body.get(), bytecode);

        if (stmt->increment) {
            compile_expr(stmt->increment.get(), bytecode, Type::UNKNOWN);
            if (!dynamic_cast<AssignExpr*>(stmt->increment.get())) {
                bytecode.write((uint8_t)OpCode::OP_POP);
            }
        }

        // Repeat
        bytecode.write((uint8_t)OpCode::OP_JUMP);
        bytecode.write((loop_start >> 8) & 0xFF);
        bytecode.write(loop_start & 0xFF);

        // After the loop
        if (stmt->condition) {
            patch_jump(bytecode, exit_jump, bytecode.code.size());
        }
    }

    // Statements compiler
    void compile_stmt(Stmt* stmt, Bytecode& bytecode) {
        // Blocks (braces)
        if (auto* block = dynamic_cast<BlockStmt*>(stmt)) {
            for (const auto& s : block->statements) {
                compile_stmt(s.get(), bytecode);
            }
            return;
        }

        // If/else
        if (auto* if_stmt = dynamic_cast<IfStmt*>(stmt)) {
            compile_if(if_stmt, bytecode);
            return;
        }

        // While loop
        if (auto* while_stmt = dynamic_cast<WhileStmt*>(stmt)) {
            compile_while(while_stmt, bytecode);
            return;
        }

        // For loop
        if (auto* for_stmt = dynamic_cast<ForStmt*>(stmt)) {
            compile_for(for_stmt, bytecode);
            return;
        }

        // Declaration
        if (auto* var_decl = dynamic_cast<VarDecl*>(stmt)) {
            Type declared_type = token_to_type(var_decl->type);
            variable_types[var_decl->name] = var_decl->is_array ? Type::ARRAY : declared_type;
            is_constant[var_decl->name] = var_decl->is_const;

            if (var_decl->is_array) {
                bool handled = false;
                
                // Array
                if (auto* arr_lit = dynamic_cast<ArrayLiteral*>(var_decl->initializer.get())) {
                    int64_t size = arr_lit->elements.size();
                    int idx = bytecode.add_const(size);
                    bytecode.write((uint8_t)OpCode::OP_CONSTANT); 
                    bytecode.write((uint8_t)idx);
                    
                    bytecode.write((uint8_t)OpCode::OP_NEW_ARRAY);
                    int name_index = bytecode.add_const(var_decl->name);
                    bytecode.write((uint8_t)OpCode::OP_SAVE);
                    bytecode.write((uint8_t)name_index);

                    for (size_t i = 0; i < arr_lit->elements.size(); ++i) {
                        int var_idx = bytecode.add_const(var_decl->name);
                        bytecode.write((uint8_t)OpCode::OP_LOAD);
                        bytecode.write((uint8_t)var_idx);

                        int idx_val = bytecode.add_const((int64_t)i);
                        bytecode.write((uint8_t)OpCode::OP_CONSTANT);
                        bytecode.write((uint8_t)idx_val);

                        compile_expr(arr_lit->elements[i].get(), bytecode, declared_type);
                        bytecode.write((uint8_t)OpCode::OP_SET_INDEX);
                    }
                    handled = true;
                } 
                // Explicit size
                else if (var_decl->array_size) {
                    compile_expr(var_decl->array_size.get(), bytecode, Type::INT64);
                    bytecode.write((uint8_t)OpCode::OP_NEW_ARRAY);
                    int name_index = bytecode.add_const(var_decl->name);
                    bytecode.write((uint8_t)OpCode::OP_SAVE);
                    bytecode.write((uint8_t)name_index);
                    handled = true;
                }
                // Function call or array variable
                else if (var_decl->initializer) {
                    compile_expr(var_decl->initializer.get(), bytecode, Type::ARRAY);
                    int name_index = bytecode.add_const(var_decl->name);
                    bytecode.write((uint8_t)OpCode::OP_SAVE);
                    bytecode.write((uint8_t)name_index);
                    handled = true;
                }

                if (!handled) {
                    throw std::runtime_error("Array size error. Provide [N] or an initializer.");
                }
            } else {
                compile_expr(var_decl->initializer.get(), bytecode, declared_type);
                int name_index = bytecode.add_const(var_decl->name);
                bytecode.write((uint8_t)OpCode::OP_SAVE);
                bytecode.write((uint8_t)name_index);
            }
            return;
        }

        if (auto* expr_stmt = dynamic_cast<ExprStmt*>(stmt)) {
            compile_expr(expr_stmt->expression.get(), bytecode, Type::UNKNOWN);
            
            if (!dynamic_cast<AssignExpr*>(expr_stmt->expression.get())) {
                bytecode.write((uint8_t)OpCode::OP_POP);
            }
            return;
        }
        
        // Assigning (Array or Variable)
        if (auto* assign_stmt = dynamic_cast<AssignStmt*>(stmt)) {
            if (auto* var = dynamic_cast<VariableExpr*>(assign_stmt->target.get())) {
                if (is_constant[var->name]) {
                    throw std::runtime_error("Cannot reassign constant variable '" + var->name + "'");
                }

                Type target_type = variable_types.count(var->name) ? variable_types[var->name] : Type::UNKNOWN;
                compile_expr(assign_stmt->value.get(), bytecode, target_type);
                int name_index = bytecode.add_const(var->name);
                bytecode.write((uint8_t)OpCode::OP_SAVE);
                bytecode.write((uint8_t)name_index);
            } else if (auto* arr_access = dynamic_cast<ArrayAccessExpr*>(assign_stmt->target.get())) {
                compile_expr(arr_access->array.get(), bytecode);
                compile_expr(arr_access->index.get(), bytecode);
                compile_expr(assign_stmt->value.get(), bytecode);
                bytecode.write((uint8_t)OpCode::OP_SET_INDEX);
            } else {
                throw std::runtime_error("Invalid assignment target");
            }
            return;
        }

        if (auto* print_stmt = dynamic_cast<PrintStmt*>(stmt)) {
            compile_expr(print_stmt->expression.get(), bytecode, Type::UNKNOWN);
            bytecode.write((uint8_t)OpCode::OP_PRINT);
            return;
        }
        
        if (auto* ret_stmt = dynamic_cast<ReturnStmt*>(stmt)) {
            if (ret_stmt->value) {
                compile_expr(ret_stmt->value.get(), bytecode, Type::UNKNOWN);
            } else {
                int idx = bytecode.add_const((int64_t)0);
                bytecode.write((uint8_t)OpCode::OP_CONSTANT); bytecode.write((uint8_t)idx);
            }
            bytecode.write((uint8_t)OpCode::OP_RETURN);
            return;
        }
        throw std::runtime_error("Unknown statement type");
    }

    Type compile_expr(Expr* expr, Bytecode& bytecode, Type expected_type = Type::UNKNOWN) {
        if (auto* i64 = dynamic_cast<Int64Literal*>(expr)) {
            int idx = bytecode.add_const(static_cast<int64_t>(i64->value));
            bytecode.write((uint8_t)OpCode::OP_CONSTANT); bytecode.write((uint8_t)idx);
            return Type::INT64;
        }
        if (auto* f64 = dynamic_cast<Float64Literal*>(expr)) {
            int idx = bytecode.add_const(static_cast<double>(f64->value));
            bytecode.write((uint8_t)OpCode::OP_CONSTANT); bytecode.write((uint8_t)idx);
            return Type::FLOAT64;
        }
        if (auto* f32 = dynamic_cast<Float32Literal*>(expr)) {
            int idx = bytecode.add_const(static_cast<float>(f32->value));
            bytecode.write((uint8_t)OpCode::OP_CONSTANT); bytecode.write((uint8_t)idx);
            return Type::FLOAT32;
        }
        if (auto* b = dynamic_cast<BoolLiteral*>(expr)) {
            int idx = bytecode.add_const(b->value);
            bytecode.write((uint8_t)OpCode::OP_CONSTANT); bytecode.write((uint8_t)idx);
            return Type::BOOL;
        }
        if (auto* s = dynamic_cast<StringLiteral*>(expr)) {
            int idx = bytecode.add_const(s->value);
            bytecode.write((uint8_t)OpCode::OP_CONSTANT); bytecode.write((uint8_t)idx);
            return Type::STR;
        }
        if (auto* c = dynamic_cast<CharLiteral*>(expr)) {
            int idx = bytecode.add_const(c->value);
            bytecode.write((uint8_t)OpCode::OP_CONSTANT); bytecode.write((uint8_t)idx);
            return Type::CHAR;
        }
        if (auto* var = dynamic_cast<VariableExpr*>(expr)) {
            int idx = bytecode.add_const(var->name);
            bytecode.write((uint8_t)OpCode::OP_LOAD); bytecode.write((uint8_t)idx);
            return variable_types.count(var->name) ? variable_types[var->name] : Type::UNKNOWN;
        }

        if (auto* assign_expr = dynamic_cast<AssignExpr*>(expr)) {
            int load_idx = bytecode.add_const(assign_expr->name);
            bytecode.write((uint8_t)OpCode::OP_LOAD); 
            bytecode.write((uint8_t)load_idx);

            compile_expr(assign_expr->value.get(), bytecode, Type::UNKNOWN);

            switch (assign_expr->op) {
                case TokenType::PLUS:  bytecode.write((uint8_t)OpCode::OP_ADD); break;
                case TokenType::MINUS: bytecode.write((uint8_t)OpCode::OP_SUBTRACT); break;
                case TokenType::STAR:  bytecode.write((uint8_t)OpCode::OP_MULTIPLY); break;
                case TokenType::SLASH: bytecode.write((uint8_t)OpCode::OP_DIVIDE); break;
                default: throw std::runtime_error("Invalid assignment operator");
            }

            bytecode.write((uint8_t)OpCode::OP_SAVE);
            bytecode.write((uint8_t)load_idx);
            return Type::UNKNOWN;
        }

        // Arrays
        if (auto* arr_access = dynamic_cast<ArrayAccessExpr*>(expr)) {
            compile_expr(arr_access->array.get(), bytecode);
            compile_expr(arr_access->index.get(), bytecode, Type::INT64);
            bytecode.write((uint8_t)OpCode::OP_GET_INDEX);
            return Type::UNKNOWN;
        }

        // Functions/Methods
        if (auto* method_call = dynamic_cast<MethodCallExpr*>(expr)) {
            compile_expr(method_call->object.get(), bytecode, Type::ARRAY);
            for (const auto& arg : method_call->arguments) {
                compile_expr(arg.get(), bytecode, Type::UNKNOWN);
            }
            if (method_call->method == "push") {
                if (method_call->arguments.size() != 1) throw std::runtime_error("push expects 1 argument");
                bytecode.write((uint8_t)OpCode::OP_ARRAY_PUSH);
            } 
            else if (method_call->method == "pop") {
                if (!method_call->arguments.empty()) throw std::runtime_error("pop expects 0 arguments");
                bytecode.write((uint8_t)OpCode::OP_ARRAY_POP);
            } 
            else {
                throw std::runtime_error("Unknown array method: " + method_call->method);
            }
            return Type::UNKNOWN;
        }
        if (auto* call = dynamic_cast<CallExpr*>(expr)) {
            for (const auto& arg : call->arguments) compile_expr(arg.get(), bytecode, Type::UNKNOWN);
            int name_idx = bytecode.add_const(call->name);
            bytecode.write((uint8_t)OpCode::OP_CALL); bytecode.write((uint8_t)name_idx); bytecode.write((uint8_t)call->arguments.size());
            return function_return_types.count(call->name) ? function_return_types[call->name] : Type::UNKNOWN;
        }

        if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
            Type left_type = compile_expr(bin->left.get(), bytecode, expected_type);
            Type right_type = compile_expr(bin->right.get(), bytecode, expected_type);
            
            switch (bin->op) {
                case TokenType::PLUS:  bytecode.write((uint8_t)OpCode::OP_ADD); break;
                case TokenType::MINUS: bytecode.write((uint8_t)OpCode::OP_SUBTRACT); break;
                case TokenType::STAR:  bytecode.write((uint8_t)OpCode::OP_MULTIPLY); break;
                case TokenType::SLASH: bytecode.write((uint8_t)OpCode::OP_DIVIDE); break;
                case TokenType::GREATER_THAN: bytecode.write((uint8_t)OpCode::OP_GREATER_THAN); break;
                case TokenType::LESS_THAN: bytecode.write((uint8_t)OpCode::OP_LESS_THAN); break;
                case TokenType::EQUALS: bytecode.write((uint8_t)OpCode::OP_EQUAL); break;
                default: throw std::runtime_error("Unknown binary operator");
            }
            return Type::UNKNOWN;
        }
        throw std::runtime_error("Unknown expression type");
    }

public:
    Bytecode compile(const vector<unique_ptr<Stmt>>& program) {
        Bytecode bytecode;
        for (const auto& stmt : program) {
            if (auto* func = dynamic_cast<FuncDecl*>(stmt.get())) {
                functions[func->name] = func;
                function_return_types[func->name] = token_to_type(func->return_type);
            }
        }

        bytecode.write((uint8_t)OpCode::OP_JUMP);
        size_t jump_placeholder = bytecode.code.size();
        bytecode.write(0); bytecode.write(0);

        for (const auto& stmt : program) {
            if (auto* func = dynamic_cast<FuncDecl*>(stmt.get())) {
                if (func->name == "main") continue;
                bytecode.function_addresses[func->name] = bytecode.code.size();
                for (const auto& param : func->params) {
                    int name_idx = bytecode.add_const(param.name);
                    bytecode.write((uint8_t)OpCode::OP_SAVE); bytecode.write((uint8_t)name_idx);
                    variable_types[param.name] = token_to_type(param.type);
                }
                for (const auto& body_stmt : func->body) compile_stmt(body_stmt.get(), bytecode);
            }
        }

        if (functions.find("main") == functions.end()) throw std::runtime_error("No 'main' function found.");
        uint16_t main_addr = (uint16_t)bytecode.code.size();
        bytecode.function_addresses["main"] = main_addr;
        bytecode.code[jump_placeholder] = (main_addr >> 8) & 0xFF;
        bytecode.code[jump_placeholder + 1] = main_addr & 0xFF;

        for (const auto& stmt : functions["main"]->body) compile_stmt(stmt.get(), bytecode);
        bytecode.write((uint8_t)OpCode::OP_HALT);
        return bytecode;
    }
};