#pragma once

#include <vector>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <string>

#include "token.hpp"

using std::runtime_error;
using std::unordered_map;
using std::vector;
using std::string;

inline const unordered_map<std::string, TokenType> keywords = {
    {"load", TokenType::LOAD},
    {"class", TokenType::CLASS},

    {"this", TokenType::THIS},
    {"public", TokenType::PUBLIC},
    {"private", TokenType::PRIVATE},
    {"protected", TokenType::PROTECTED},

    {"let", TokenType::LET},
    {"const", TokenType::CONST},

    {"func", TokenType::FUNC},
    {"ret", TokenType::RET},
    
    {"if", TokenType::IF}, 
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},

    {"print", TokenType::PRINT},

    {"int32", TokenType::INT32},
    {"int64", TokenType::INT64},
    {"int", TokenType::INT64},

    {"float32", TokenType::FLOAT32},
    {"float64", TokenType::FLOAT64},
    {"float", TokenType::FLOAT64},
    
    {"str", TokenType::STR},
    {"char", TokenType::CHAR},
    {"bool", TokenType::BOOL},
    {"void", TokenType::VOID},

    {"not", TokenType::NOT},
    {"false", TokenType::BOOL_LITERAL},
    {"true", TokenType::BOOL_LITERAL},
    {"null", TokenType::NIL},
};

class Lexer {
    const string source;
    vector<Token> tokens;

    size_t start = 0;
    size_t current = 0;

    // Line/column of the next character to be consumed (both 1-based)
    size_t line = 1;
    size_t column = 1;

    // Line/column of the start of the token currently being scanned
    size_t start_line = 1;
    size_t start_column = 1;

    // Helpers
    bool is_at_end() const { return current >= source.size(); }

    void skip_whitespace() {
        while(!is_at_end()) {
            unsigned char c = static_cast<unsigned char>(peek());
            
            if (std::isspace(c) || c < 32 || c > 126) {
                next();
            } else {
                break;
            }
        }
    }

    void identifier() {
        while(!is_at_end()) {
            const unsigned char c = static_cast<unsigned char>(peek());
            if (!std::isalnum(c) && c != '_') break;
            next();
        }
    }

    void number() {
        while(!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
            next();
        }
        if (!is_at_end() && peek() == '.') {
            if (current + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[current + 1]))) {
                next();
                while(!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
                    next();
                }
            }
        }
    }

    Token make_token(TokenType type) const {
        return Token{
            .type = type,
            .text = source.substr(start, current - start),
            .location = SourceLocation{.offset = start, .line = start_line, .column = start_column}
        };
    }

    // Throws with the location of the token currently being scanned
    [[noreturn]] void error(const string& message) const {
        throw runtime_error("[Line " + std::to_string(start_line) +
                            ", Col " + std::to_string(start_column) + "] " + message);
    }

    char decode_escape(char c) {
        switch (c) {
            case 'n':  return '\n'; case 't':  return '\t';
            case 'r':  return '\r'; case '\\': return '\\';
            case '"':  return '"';  case '\'': return '\'';
            case '0':  return '\0'; default:   return c;
        }
    }

    string scan_quoted(char quote) {
        string value;
        while (!is_at_end() && peek() != quote) {
            if (peek() == '\\') {
                next();
                if (is_at_end()) error("Unterminated escape sequence");
                value += decode_escape(next());
            } else {
                value += next();
            }
        }
        if (is_at_end()) error("Unterminated literal");
        next();
        return value;
    }

    Token scan_char(char quote) {
        string v = scan_quoted(quote);
        if (v.size() != 1) error("Char literal must contain exactly one character");
        Token token = make_token(TokenType::CHAR_LITERAL);
        token.text = std::move(v);
        return token;
    }

    Token scan_string(char quote) {
        Token token = make_token(TokenType::STR_LITERAL);
        token.text = scan_quoted(quote);
        return token;
    }

    char peek() const {
        if(is_at_end()) { return '\0'; }
        return source[current];
    }

    char next() {
        if(is_at_end()) { return '\0'; }
        const char c = source[current++];
        if (c == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        return c;
    }
public:
    explicit Lexer(const string& src) : source(src) {}

    vector<Token> lex() {
        while(!is_at_end()) {
            skip_whitespace();
            if(is_at_end()) break;

            start = current;
            start_line = line;
            start_column = column;
            const char c = next();

            switch(c) {
                case '"': tokens.push_back(scan_string(c)); break;
                case '\'': tokens.push_back(scan_char(c)); break;
                case ',': tokens.push_back(make_token(TokenType::COMMA)); break;
                case '.': tokens.push_back(make_token(TokenType::DOT)); break;
                case '=': 
                    if (peek() == '=') {
                        next();
                        tokens.push_back(make_token(TokenType::EQUALS_EQUALS));
                    } else {
                        tokens.push_back(make_token(TokenType::EQUALS));
                    }
                    break;

                case '&':
                    if (peek() == '&') {
                        next();
                        tokens.push_back(make_token(TokenType::AND));
                    } else {
                        error("Unexpected character '&'");
                    }
                    break;
                case '|':
                    if (peek() == '|') {
                        next();
                        tokens.push_back(make_token(TokenType::OR));
                    } else {
                        error("Unexpected character '|'");
                    }
                    break;

                case ';': tokens.push_back(make_token(TokenType::SEMICOLON)); break;
                case '{': tokens.push_back(make_token(TokenType::LBRACE)); break;
                case '}': tokens.push_back(make_token(TokenType::RBRACE)); break;
                case '[': tokens.push_back(make_token(TokenType::LBRACKET)); break;
                case ']': tokens.push_back(make_token(TokenType::RBRACKET)); break;
                    
                case ':':
                    if (peek() == ':') { 
                        next(); 
                        tokens.push_back(make_token(TokenType::COLON_COLON)); 
                    } else { 
                        error("Unexpected character ':'"); 
                    }
                    break;
                
                case '+':
                    if (peek() == '=') {
                        next();
                        tokens.push_back(make_token(TokenType::PLUS_EQUALS));
                    } else {
                        tokens.push_back(make_token(TokenType::PLUS));
                    }
                    break;
                    
                case '-':                     
                    if (peek() == '=') {
                        next();
                        tokens.push_back(make_token(TokenType::MINUS_EQUALS));
                    } else {
                        tokens.push_back(make_token(TokenType::MINUS));
                    }
                    break;
                    
                case '*':                     
                    if (peek() == '=') {
                        next();
                        tokens.push_back(make_token(TokenType::STAR_EQUALS));
                    } else {
                        tokens.push_back(make_token(TokenType::STAR));
                    }
                    break;
                    
                case '/':
                    if (peek() == '/') { 
                        // Comment
                        while(peek() != '\n' && !is_at_end()) next(); 
                    } 
                    else if (peek() == '=') { 
                        next(); 
                        tokens.push_back(make_token(TokenType::SLASH_EQUALS)); 
                    } 
                    else { 
                        tokens.push_back(make_token(TokenType::SLASH)); 
                    }
                    break;

                case '#':
                    while(peek() != '\n' && !is_at_end()) next();
                    break;

                case '(': tokens.push_back(make_token(TokenType::LPAREN)); break;
                case ')': tokens.push_back(make_token(TokenType::RPAREN)); break;
                case '!':
                    if (peek() == '=') {
                        next();
                        tokens.push_back(make_token(TokenType::NOT_EQUALS));
                    } else {
                        tokens.push_back(make_token(TokenType::NOT));
                    }
                break;
                
                case '>':
                    if (peek() == '=') { next(); tokens.push_back(make_token(TokenType::GREATER_EQUAL)); }
                    else { tokens.push_back(make_token(TokenType::GREATER_THAN)); }
                    break;
                case '<':
                    if (peek() == '=') { next(); tokens.push_back(make_token(TokenType::LESS_EQUAL)); }
                    else { tokens.push_back(make_token(TokenType::LESS_THAN)); }
                    break;

                default:
                    if(std::isalpha(c) || c == '_') {
                        identifier();
                        Token tok = make_token(TokenType::IDENTIFIER);
                        string lower = tok.text;
                        for (auto& ch : lower) ch = static_cast<char>(std::tolower((unsigned char)ch));
                        auto it = keywords.find(lower);
                        if (it != keywords.end()) tok.type = it->second;
                        tokens.push_back(tok);
                    } else if(std::isdigit(c)) {
                        number();
                        
                        bool is_f32 = false;
                        bool is_f64 = false;

                        // d or f, for explicit type too
                        if (peek() == 'f' || peek() == 'F') {
                            next();
                            is_f32 = true;
                        } else if (peek() == 'd' || peek() == 'D') {
                            next();
                            is_f64 = true;
                        }

                        if (is_f32) {
                            tokens.push_back(make_token(TokenType::FLOAT32_LITERAL));
                        } else if (is_f64) {
                            tokens.push_back(make_token(TokenType::FLOAT64_LITERAL));
                        } else if (source.substr(start, current - start).find('.') != string::npos) {
                            tokens.push_back(make_token(TokenType::FLOAT64_LITERAL));
                        } else {
                            tokens.push_back(make_token(TokenType::INT64_LITERAL));
                        }
                    } else { 
                        error("Unknown character: '" + string(1, c) +
                              "' (ASCII: " + std::to_string(static_cast<int>(static_cast<unsigned char>(c))) + ")");
                    }
                    break;
            }
        }
        return tokens;
    }
};