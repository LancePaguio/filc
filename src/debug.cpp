#include "debug.h"
#include "common.h"
#include <ostream>
#include <string>

namespace filcompiler{
const char* tokenTypeName(TokenType t) {
    switch (t) {
	    case TokenType::ITAKDA: return "ITAKDA";
	    case TokenType::KUNG: return "KUNG";
	    case TokenType::KUNG_HINDI: return "KUNG_HINDI";
	    case TokenType::HABANG: return "HABANG";
	    case TokenType::GAWIN: return "GAWIN";
	    case TokenType::PARA: return "PARA";
	    case TokenType::PILI: return "PILI";
	    case TokenType::KASO: return "KASO";
	    case TokenType::TIGIL: return "TIGIL";
	    case TokenType::SUNOD: return "SUNOD";
	    case TokenType::KUNIN: return "KUNIN";
	    case TokenType::COMMA: return "COMMA";
	    case TokenType::BASAHIN: return "BASAHIN";
	    case TokenType::ISULAT: return "ISULAT";
	    case TokenType::IPAKITA: return "IPAKITA";
	    case TokenType::TOTOO: return "TOTOO";
	    case TokenType::MALI: return "MALI";
	    case TokenType::AT: return "AT";
	    case TokenType::O: return "O";
	    case TokenType::HINDI: return "HINDI";
	    case TokenType::NUMBER: return "NUMBER";
	    case TokenType::IDENTIFIER: return "IDENTIFIER";
	    case TokenType::STRING: return "STRING";
	    case TokenType::PLUS: return "PLUS";
	    case TokenType::MINUS: return "MINUS";
	    case TokenType::STAR: return "STAR";
	    case TokenType::SLASH: return "SLASH";
	    case TokenType::PERCENT: return "PERCENT";
	    case TokenType::GREATER: return "GREATER";
	    case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
	    case TokenType::LESS: return "LESS";
	    case TokenType::LESS_EQUAL: return "LESS_EQUAL";
	    case TokenType::EQUAL: return "EQUAL";
	    case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
	    case TokenType::NOT_EQUAL: return "NOT_EQUAL";
	    case TokenType::LEFT_PAREN: return "LEFT_PAREN";
	    case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
	    case TokenType::LEFT_BRACE: return "LEFT_BRACE";
	    case TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
	    case TokenType::COLON: return "COLON";
	    case TokenType::SEMICOLON: return "SEMICOLON";
	    case TokenType::END_OF_FILE: return "END_OF_FILE";
	}
	return "?";
}

void dumpTokens(const std::vector<Token>& tokens, std::ostream& out) {
    for (const Token& t : tokens) {
	    out << t.line << ":" << t.column << "\t" << tokenTypeName(t.type);
		if (!t.lexeme.empty()) out << "\t'" << t.lexeme << "'";
		out << "\n";
	}
}
}
