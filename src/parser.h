#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "ast.h"

#include <cstddef>
#include <string>
#include <vector>

namespace filcompiler {

// this is use to track the depth
// for 3.4
inline constexpr int MAX_NESTING_DEPTH = 5;
inline constexpr int MAX_EXPR_DEPTH = 64;
inline constexpr int MAX_ELSE_CHAIN = 256;

class Parser {
public:
    std::vector<StmtPtr> parseProgram();
    explicit Parser(std::vector<Token> tkn) : tokens(std::move(tkn)) {}
	bool hadError() const {
	    return hadErrorVar;
	}

	const std::vector<Diagnostic>& diagnostics() const {
	    return diagnosticsVar;
	}
private:
    const std::vector<Token> tokens;
	std::size_t current = 0;
	int nestingDepth = 0;
    int elseChainDepth = 0;
	int exprDepth = 0;
	bool hadErrorVar = false;
	std::vector<Diagnostic> diagnosticsVar;

	class DepthCheck {
	public:
	    // decrement when in the destructor
	    DepthCheck(int& counter, int limit, ErrorLoc loc, const char* message);
		~DepthCheck() {
		    --counterVar;
		}
		DepthCheck(const DepthCheck&) = delete;
		DepthCheck& operator=(const DepthCheck&) = delete;
	private:
	    int& counterVar;

	};

	//token helpers
	//for 3.3
	bool isAtEnd() const;
	const Token& peek() const;
	const Token& previous() const;
	const Token& advance();
	bool check(TokenType t) const;
	bool checkNext(TokenType t) const;
	bool match(TokenType t);
	const Token& consume(TokenType t, const std::string& msg);
	void synchronize();

	static bool startsStatement(TokenType t);
	static int parseIntLiteral(const Token& tok, bool negative = false);
	static bool isRelationalOp(TokenType t);
	static BinOp binOpFor(TokenType t);

    //statments
	StmtPtr statement();
	StmtPtr ifStmt(ErrorLoc l);
	StmtPtr forStmt(ErrorLoc l);
	StmtPtr switchStmt(ErrorLoc l);
	StmtPtr whileStmt(ErrorLoc l);
	StmtPtr doWhileStmt(ErrorLoc l);
	StmtPtr assignStmt(ErrorLoc l);
	StmtPtr assignStmtNoSemi(ErrorLoc l);
	StmtPtr varDecl(ErrorLoc l);
	StmtPtr block(ErrorLoc l);
	StmtPtr requireBlock();
	StmtPtr printStmt(ErrorLoc l);
	StmtPtr readStmt(ErrorLoc l);

	//expresions
	ExprPtr expression();
	ExprPtr logicalOr();
	ExprPtr logicalAnd();
	ExprPtr logicalNot();
	ExprPtr relational();
	ExprPtr term();
	ExprPtr factor();
	ExprPtr unary();
	ExprPtr primary();


};
}
#endif // !PARSER_H
