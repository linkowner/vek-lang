#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "token.hpp"

using std::string;
using std::vector;
using std::unique_ptr;
using std::move;

struct Expr { virtual ~Expr() = default; };

struct Int32Literal : Expr { int32_t value; Int32Literal(int32_t v) : value(v) {} };
struct Int64Literal : Expr { int64_t value; Int64Literal(int64_t v) : value(v) {} };
struct Float32Literal : Expr { float value; Float32Literal(float v) : value(v) {} };
struct Float64Literal : Expr { double value; Float64Literal(double v) : value(v) {} };
struct BoolLiteral : Expr { bool value; BoolLiteral(bool v) : value(v) {} };
struct StringLiteral : Expr { string value; StringLiteral(string v) : value(move(v)) {} };
struct CharLiteral : Expr { char value; CharLiteral(char v) : value(v) {} };

struct VariableExpr : Expr {
    string name;
    VariableExpr(string n) : name(move(n)) {}
};

struct AssignExpr : Expr {
    string name;
    TokenType op;
    unique_ptr<Expr> value;

    AssignExpr(string n, TokenType o, unique_ptr<Expr> v)
        : name(move(n)), op(o), value(move(v)) {}
};

struct UnaryExpr : Expr {
    TokenType op;
    unique_ptr<Expr> right;
    UnaryExpr(TokenType o, unique_ptr<Expr> r) : op(o), right(move(r)) {}
};

struct ArrayAccessExpr : Expr {
    unique_ptr<Expr> array;
    unique_ptr<Expr> index;
    ArrayAccessExpr(unique_ptr<Expr> a, unique_ptr<Expr> i) : array(move(a)), index(move(i)) {}
};

struct ArrayLiteral : Expr {
    vector<unique_ptr<Expr>> elements;
    ArrayLiteral(vector<unique_ptr<Expr>> e) : elements(move(e)) {}
};

struct MethodCallExpr : Expr {
    unique_ptr<Expr> object;
    string method;
    vector<unique_ptr<Expr>> arguments;
    
    MethodCallExpr(unique_ptr<Expr> obj, string m, vector<unique_ptr<Expr>> args) 
        : object(move(obj)), method(move(m)), arguments(move(args)) {}
};

struct CallExpr : Expr {
    string name;
    vector<unique_ptr<Expr>> arguments;
    CallExpr(string n, vector<unique_ptr<Expr>> args) : name(move(n)), arguments(move(args)) {}
};

struct BinaryExpr : Expr {
    unique_ptr<Expr> left;
    TokenType op;
    unique_ptr<Expr> right;
    BinaryExpr(unique_ptr<Expr> l, TokenType o, unique_ptr<Expr> r) : left(move(l)), op(o), right(move(r)) {}
};

struct Stmt { virtual ~Stmt() = default; };

struct ExprStmt : Stmt {
    unique_ptr<Expr> expression;
    ExprStmt(unique_ptr<Expr> expr) : expression(move(expr)) {}
};

struct VarDecl : Stmt {
    TokenType type;
    string name;
    bool is_array;
    bool is_const;
    unique_ptr<Expr> array_size;
    unique_ptr<Expr> initializer;
    VarDecl(TokenType t, string n, bool is_arr, bool is_c, unique_ptr<Expr> size, unique_ptr<Expr> init)
            : type(t), name(move(n)), is_array(is_arr), is_const(is_c), array_size(move(size)), initializer(move(init)) {}
};

struct AssignStmt : Stmt {
    unique_ptr<Expr> target;
    unique_ptr<Expr> value;
    AssignStmt(unique_ptr<Expr> t, unique_ptr<Expr> v) : target(move(t)), value(move(v)) {}
};

struct PrintStmt : Stmt {
    unique_ptr<Expr> expression;
    PrintStmt(unique_ptr<Expr> expr) : expression(move(expr)) {}
};

struct BlockStmt : Stmt {
    vector<unique_ptr<Stmt>> statements;
    BlockStmt(vector<unique_ptr<Stmt>> s) : statements(move(s)) {}
};

struct IfStmt : Stmt {
    unique_ptr<Expr> condition;
    unique_ptr<Stmt> then_branch;
    unique_ptr<Stmt> else_branch;
    IfStmt(unique_ptr<Expr> cond, unique_ptr<Stmt> then_b, unique_ptr<Stmt> else_b)
        : condition(move(cond)), then_branch(move(then_b)), else_branch(move(else_b)) {}
};

struct WhileStmt : Stmt {
    unique_ptr<Expr> condition;
    unique_ptr<Stmt> body;

    WhileStmt(unique_ptr<Expr> cond, unique_ptr<Stmt> b)
        : condition(move(cond)), body(move(b)) {}
};

struct ForStmt : Stmt {
    unique_ptr<Stmt> initializer;
    unique_ptr<Expr> condition;
    unique_ptr<Expr> increment;
    unique_ptr<Stmt> body;

    ForStmt(unique_ptr<Stmt> init, unique_ptr<Expr> cond, unique_ptr<Expr> inc, unique_ptr<Stmt> b)
        : initializer(move(init)), condition(move(cond)), increment(move(inc)), body(move(b)) {}
};

struct Param {
    TokenType type;
    string name;
    Param(TokenType t, string n) : type(t), name(move(n)) {}
};

struct FuncDecl : Stmt {
    TokenType return_type;
    string name;
    vector<Param> params;
    vector<unique_ptr<Stmt>> body;
    FuncDecl(TokenType rt, string n, vector<Param> p, vector<unique_ptr<Stmt>> b) 
        : return_type(rt), name(move(n)), params(move(p)), body(move(b)) {}
};

struct ReturnStmt : Stmt {
    unique_ptr<Expr> value;
    ReturnStmt(unique_ptr<Expr> v) : value(move(v)) {}
};