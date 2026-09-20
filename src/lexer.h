#ifndef LEXER_H
#define LEXER_H

#include "common.h"

#include <string>
#include <vector>
#include <cstddef>


namespace filcompiler {

class Lexer {
public:
    explicit Lexer(std::string source)
	    : src(std::move(source)) {}

	std::vector<Token> scanTokens();

private:
    //member variables
	const std::string src;
	std::size_t start = 0;
	std::size_t current = 0;
	int line = 1;
    int lineStart = 0;
    int tokenLine = 1;
    int tokenColumn = 1;
	std::vector<Token> tokens;

    //functions
	char advance();
	char peek() const;
	char peekNext() const;
	bool isAtEnd() const;
	bool match(char expected);
	int columnOf(std::size_t offset) const;
	void addToken(TokenType type);
	void skipping();
	void scanToken();
	void identifier();
	void number();
	void stringLit();

};

}

#endif // !LEXER_H
