#ifndef COMMON_H
#define COMMON_H

#include <stdexcept>
#include <string>
#include <unordered_map>

// we use a our own namespace to prevent name collision
namespace filcompiler {

enum class TokenType {
	// single character tokens
	LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
	COMMA, MINUS, PLUS, COLON, SEMICOLON, SLASH, STAR,
	PERCENT,

    // two character tokens
	EQUAL, EQUAL_EQUAL, NOT_EQUAL,
	GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,

    // literals
    IDENTIFIER, STRING, NUMBER,

    // filipino keywords
    ITAKDA, KUNG, KUNG_HINDI, HABANG, GAWIN, PARA, PILI, KASO, TIGIL,
	SUNOD, BASAHIN, KUNIN, ISULAT, IPAKITA, TOTOO, MALI, AT, O, HINDI,

	END_OF_FILE
};

struct ErrorLoc {
    int line = 0;
	int column = 0;
};

// all of these are information stored in the token
// this checks 2.1 of the action plan
struct Token {
    ErrorLoc loc() const {
		return {line, column};
	}
    TokenType type{};
	std::string lexeme;
	int line{};
	int column{};
};

// Diagnostics for lexical errors. 2.4
// we are using an exception to signal that
// a function can't perform it's assigned task
struct CompileError : public std::runtime_error {
	ErrorLoc loc;
    int len;
	CompileError(std::string msg, ErrorLoc loc_, int len_ = 1)
	    : std::runtime_error(std::move(msg)), loc(loc_), len(len_) {}
};

// show which stage produced the diagnostics
enum class Stage{Lexer, Syntax, Semantic, Runtime, Internal};
struct Diagnostic {
    Stage stage = Stage::Internal;
	std::string message;
	ErrorLoc loc;
	int len = 1;
};

// using unordered_map to create a
// lookup table which matches the keyword
// with teh token type
// this checks 2.2
inline const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"itakda", TokenType::ITAKDA},
	{"kung", TokenType::KUNG},
	{"kung_hindi", TokenType::KUNG_HINDI},
	{"gawin", TokenType::GAWIN},
	{"pili", TokenType::PILI},
	{"tigil", TokenType::TIGIL},
	{"basahin", TokenType::BASAHIN},
	{"isulat", TokenType::ISULAT},
	{"totoo", TokenType::TOTOO},
	{"at", TokenType::AT},
	{"o", TokenType::O},
	{"habang", TokenType::HABANG},
	{"para", TokenType::PARA},
	{"kaso", TokenType::KASO},
	{"sunod", TokenType::SUNOD},
	{"kunin", TokenType::KUNIN},
	{"ipakita", TokenType::IPAKITA},
	{"mali", TokenType::MALI},
	{"hindi", TokenType::HINDI},
};


}
#endif
