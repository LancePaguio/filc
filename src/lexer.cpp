#include "lexer.h"
#include <cctype>

namespace filcompiler {

namespace {
	// We make use of ctype a c++ wrapper to quickly check
	// if its a digit, an alphabet, or both
    bool isDigitCh(char c) {
		return std::isdigit(static_cast<unsigned char>(c)) != 0;
	}
    bool isAlphaCh(char c) {
	    return std::isalpha(static_cast<unsigned char>(c)) != 0;
		}
    bool isAlphaNum(char c) {
		return std::isalnum(static_cast<unsigned char>(c)) != 0;
	}
}

// the main scanner works its way through the whole
// source code until it runs out of characters
std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
	    skipping();
		if (isAtEnd())
			break;
		start = current;
		tokenLine = line;
		tokenColumn = columnOf(start);
		scanToken();
	}
	tokens.push_back({TokenType::END_OF_FILE, "", line, columnOf(current)});
	return tokens;
}

// we created these helper methods using
// crafting interpreters as reference
bool Lexer::isAtEnd() const {return current >= src.size();}
char Lexer::advance() {return src[current++];}
char Lexer::peek() const { return isAtEnd() ? '\0' : src[current]; }
char Lexer::peekNext() const {
    return (current + 1 >= src.size()) ? '\0' : src[current + 1];

}
bool Lexer::match(char expected) {
    if (isAtEnd() || src[current] != expected)
		return false;
	++current;
	return true;
}
int Lexer::columnOf(std::size_t offset) const {
    return static_cast<int>(offset) - lineStart + 1;
}
void Lexer::addToken(TokenType type) {
    tokens.push_back({type, src.substr(start, current - start), tokenLine,
		tokenColumn});
}

// skips whitespaces, newlines, comments
// for 2.3
void Lexer::skipping() {
    for (;;) {
	    const char c = peek();
		if (c == ' ' || c == '\r' || c == '\t') {
		    advance();
		} else if (c == '\n') {
		    ++line;
			advance();
			lineStart = static_cast<int>(current);
		} else if (c == '/' && peekNext() == '/') {
		    while (peek() != '\n' && !isAtEnd()) advance();
		} else {
		    return;
		}
	}
}

// this is our jump table to match
// single or two characters operators
// with the token type
// checks 2.3
void Lexer::scanToken() {
    const char c = advance();
	switch(c) {
	case '(': addToken(TokenType::LEFT_PAREN); return;
	case ')': addToken(TokenType::RIGHT_PAREN); return;
	case '{': addToken(TokenType::LEFT_BRACE); return;
	case '}': addToken(TokenType::RIGHT_BRACE); return;
	case ';': addToken(TokenType::SEMICOLON); return;
	case ':': addToken(TokenType::COLON); return;
	case '-': addToken(TokenType::MINUS); return;
	case '+': addToken(TokenType::PLUS); return;
	case '*': addToken(TokenType::STAR); return;
	case ',': addToken(TokenType::COMMA); return;
	case '%': addToken(TokenType::PERCENT); return;
	case '/': addToken(TokenType::SLASH); return;
	case '"': stringLit(); return;
	case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL
								  : TokenType::EQUAL); return;
	case '>': addToken(match('=') ? TokenType::GREATER_EQUAL
								  : TokenType::GREATER); return;
	case '<': addToken(match('=') ? TokenType::LESS_EQUAL
	                              : TokenType::LESS); return;
	// gives an error when it sees only a `!` without the `=`
	case '!':
	    if (match('=')) {
		    addToken(TokenType::NOT_EQUAL);
			return;
		}
		throw CompileError("Hindi inaasahang karakter: '!'. Ito ba ay dapat '!='?",
			{tokenLine, tokenColumn});
	default:
		// we do numbers first in case the identifier
		// starts with a digit.
	    if (isDigitCh(c)) {
		    number();
			return;
		}
	    if (isAlphaCh(c) || c == '_') {
		    identifier();
			return;
		}
		throw CompileError(std::string("Hindi inaasahang karakter: '") + c + "'",
		    {tokenLine, tokenColumn});

	}
}

//number literals
void Lexer::number() {
    while (isDigitCh(peek())) advance();
	addToken(TokenType::NUMBER);

}

//string literals
void Lexer::stringLit() {
    const int startLine = tokenLine;
	const int startCol = tokenColumn;
	while (peek() != '"' && !isAtEnd()) {
	    if (peek() == '\n') {
		    ++line;
			advance();
			lineStart = static_cast<int>(current);
			continue;
		}
		advance();
	}
	// for unclosed quotes
	if (isAtEnd())
	    throw CompileError("Hindi natatapos na string literal.",
		    {startLine, startCol});
	advance();
	addToken(TokenType::STRING);

}

// this identifier employs the maximal munch,
// where it will keep consuming until the token
// cannot be extended anymore.
// This is to check two characters tokens like
// '!=', '>=', '<='
void Lexer::identifier() {
    while (isAlphaNum(peek()) || peek() == '_') advance();
	const std::string text = src.substr(start, current - start);
	const auto it = KEYWORDS. find(text);
	addToken(it != KEYWORDS.end() ? it->second : TokenType::IDENTIFIER);

}



}
