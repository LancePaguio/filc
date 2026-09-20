#include "common.h"
#include "ast.h"
#include "parser.h"

#include <climits>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

namespace filcompiler {

// a constructor to limit nesting for blocks and expressions
Parser::DepthCheck::DepthCheck(int& counter, int limit, ErrorLoc loc,
    const char* message)
    : counterVar(counter) {
	++counter;
	if (counter > limit) {
	    --counter;
		throw CompileError(std::string(message) + "hanggang sa (" +
		    std::to_string(limit) + " na antas) lamang ang nesting", loc);
    }
}

// parse a single statement, and if it throws it will still record the fault,
// then continue
std::vector<StmtPtr> Parser::parseProgram() {
    std::vector<StmtPtr> stmts;
    while (!isAtEnd()) {
	    const std:: size_t locationBefore = current;
		try {
		    stmts.push_back(statement());
	    } catch (const CompileError& e) {
			hadErrorVar = true;
			diagnosticsVar.push_back ({Stage::Syntax, e.what(), e.loc, e.len});
			synchronize();
			if (current == locationBefore) advance();
		}
	}
	return stmts;
}

//token helprs
bool Parser::isAtEnd() const {
    return peek(). type == TokenType::END_OF_FILE;
}
const Token& Parser::peek() const {
    return tokens[current];
}
const Token& Parser::previous() const {
    return tokens[current -1];
}
const Token& Parser::advance() {
    if (!isAtEnd()) ++current;
	return previous();
}
bool Parser::check(TokenType t) const {
    return !isAtEnd() && peek().type == t;
}
bool Parser::checkNext(TokenType t)  const {
    if (isAtEnd())
        return false;
    if (current + 1 >= tokens.size())
        return false;
    return tokens[current + 1].type == t;
}
// consumes if it matches
bool Parser::match(TokenType t) {
    if (check(t)) {
	advance();
	return true;
    }
	return false;
}
// this points out if ';' or '}' is missing
// or any token that is not there really
const Token& Parser::consume(TokenType t, const std::string& msg) {
    if (check(t))
	    return advance();
	const bool terminator = (t == TokenType::SEMICOLON || t ==
	    TokenType::RIGHT_BRACE);
	const Token& anchor = (terminator && current > 0) ? previous() : peek();
	const int reportLine = anchor.line;
	const int reportCol = terminator ? anchor.column +
	    static_cast<int>(anchor.lexeme.size()) : anchor.column;
    const int reportLen = terminator ? 1
	    : static_cast<int>(anchor.lexeme.size());
	throw CompileError(msg, {reportLine, reportCol}, reportLen);
}
// handle range error
int Parser::parseIntLiteral(const Token& tkn, bool negative) {
    const std::string text = (negative ? "-" : "") + tkn.lexeme;
    errno = 0;
    char* end  = nullptr;
    const long long v = std::strtoll(text.c_str(), &end, 10);

    if (end == text.c_str() || *end != '\0')
        throw CompileError("Hindi wastong numero: '" + text + "'.", tkn.loc(), static_cast<int>(tkn.lexeme.size()));

    if (errno == ERANGE || v < INT_MIN || v > INT_MAX)
	    throw CompileError("Ang numero '" + text +
		    "' ay lumagpas. Ang 32 bit integer ay maaring lamang (" +
			std::to_string(INT_MIN) + " hanggang " + std::to_string(INT_MAX) +
			").", tkn.loc(), static_cast<int>(tkn.lexeme.size()));

	return static_cast<int>(v);
}
// synchonizing tokens, it can begin a statement which
// stops synchronize() from discarding tokens
bool Parser::startsStatement(TokenType t) {
    switch(t) {
	    case TokenType::ITAKDA:
		case TokenType::KUNG:
		case TokenType::HABANG:
	    case TokenType::PARA:
		case TokenType::PILI:
		case TokenType::ISULAT:
	    case TokenType::IPAKITA:
		case TokenType::TIGIL:
		case TokenType::SUNOD:
	    case TokenType::GAWIN:
		case TokenType::BASAHIN:
		case TokenType::KUNIN:
			return true;
		default:
		    return false;
	}
}
//
void Parser::synchronize() {
    if (!startsStatement(peek().type)) advance();
	while (!isAtEnd()) {
	    if (previous().type == TokenType::SEMICOLON)
		    return;
		if (startsStatement(peek().type))
		    return;
		advance();
	}
};

// the LL(1) grammar rules
// for statements and expressions

//statement dispatcher

StmtPtr Parser::statement() {
    const ErrorLoc loc = peek().loc();
    if (match(TokenType::ITAKDA))
	    return varDecl(loc);
    if (match(TokenType::KUNG))
	    return ifStmt(loc);
    if (match(TokenType::HABANG))
	    return whileStmt(loc);
    if (match(TokenType::GAWIN))
	    return doWhileStmt(loc);
    if (match(TokenType::PARA))
	    return forStmt(loc);
    if (match(TokenType::PILI))
	    return switchStmt(loc);
    if (match(TokenType::TIGIL)) {
	    consume(TokenType::SEMICOLON, "kinakailangan ang ';' bago ng 'tigil'.");
		return std::make_unique<BreakStmt>(loc);
	}
    if (match(TokenType::SUNOD)) {
	    consume(TokenType::SEMICOLON, "kinakailangan ang ';' bago ng 'sunod'.");
		return std::make_unique<ContinueStmt>(loc);
	}
    if (check(TokenType::BASAHIN) || check(TokenType::KUNIN)) {
        return readStmt(loc);
    }
	if (check(TokenType::ISULAT) || check(TokenType::IPAKITA))
	    return printStmt(loc);
	if (check(TokenType::LEFT_BRACE))
	    return block(loc);
	if (check(TokenType::IDENTIFIER))
	    return assignStmt(loc);
	throw CompileError("kinakailangan ng pahayag(statement).", loc);

}

StmtPtr Parser::varDecl(ErrorLoc loc) {
    const Token name = consume(TokenType::IDENTIFIER,
	    "kinakailangan ang pangalan ng variable. ");
	consume(TokenType::EQUAL,
	    "kinakailangan ang '=' sa deklarasyon ng variable.");
	ExprPtr init = expression();
	consume(TokenType::SEMICOLON, "kinakailangan ang ';' bago ng deklarasyon.");
	return std::make_unique<VarDeclStmt>(name.lexeme, std::move(init), loc);

}

StmtPtr Parser::assignStmtNoSemi(ErrorLoc loc) {
    const Token name = consume(TokenType::IDENTIFIER,
	    "kinakailangan ang pangalan ng variable.");
	consume(TokenType::EQUAL, "kinakailangan ang '=' bago ng deklarasyon.");
	ExprPtr val = expression();
	return std::make_unique<AssignStmt>(name.lexeme, std::move(val), loc);
}

StmtPtr Parser::assignStmt(ErrorLoc loc) {
    StmtPtr s = assignStmtNoSemi(loc);
	consume(TokenType::SEMICOLON,
	    "kinakailangan ang ';' bago magbigay ng halaga.");
	return s;
}

StmtPtr Parser::printStmt(ErrorLoc loc) {
	advance();
	consume(TokenType::LEFT_PAREN,
	    "kinakailangan ang '(' bago ng 'isulat' o 'ipakita'.");
	StmtPtr result;
	if (check(TokenType::STRING)) {
	    const std::string raw = advance().lexeme;
		std::string content = raw.substr(1, raw.size() - 2);
		result = std::make_unique<PrintStmt>(nullptr,
		    std::move(content), true, loc);
	} else{
	    ExprPtr e = expression();
		result = std::make_unique<PrintStmt>(std::move(e), "", false, loc);
	}
	consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
	consume(TokenType::SEMICOLON, "kinakailangan ang ';'.");
	return result;

}

StmtPtr Parser::block(ErrorLoc loc) {
    const DepthCheck guard(nestingDepth, MAX_NESTING_DEPTH, loc,
	    "Sumobra ang lalim ng pugad");
	advance();
	std::vector<StmtPtr> stmts;
	while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
	    stmts.push_back(statement());
	}
	consume(TokenType::RIGHT_BRACE, "kinakailangan ang '}'.");
	return std::make_unique<BlockStmt>(std::move(stmts), loc);

}

// those that have a loop body like habang
// will get this error message if they don't a block
StmtPtr Parser::requireBlock(){
    if (!check(TokenType::LEFT_BRACE))
	    throw CompileError("kinakailangan ang '{' upang simulan ang block.",
		    peek().loc());
	return block(peek().loc());

}

// resolve if and if else chains by recursing into ifstmt
// to check for branches
StmtPtr Parser::ifStmt(ErrorLoc loc) {
	consume(TokenType::LEFT_PAREN, "kinakailangan ang '(' bago ng 'kung'.");
	ExprPtr cond = expression();
	consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
	StmtPtr thenB = requireBlock();
	StmtPtr elseB = nullptr;
	if (match(TokenType::KUNG_HINDI)) {
	    if (check(TokenType::KUNG)) {
		    const ErrorLoc l2 = peek().loc();
			advance();
			const DepthCheck guard(elseChainDepth, MAX_ELSE_CHAIN, l2,
			    "Sumobra ang haba ng 'kung_hindi' chain: ");
			elseB = ifStmt(l2);
		} else{
	        elseB= requireBlock();
		}
	}
	return std::make_unique<IfStmt>(std::move(cond), std::move(thenB),
	    std::move(elseB), loc);
}

StmtPtr Parser::whileStmt(ErrorLoc loc) {
	consume(TokenType::LEFT_PAREN, "kinakailangan ang '(' bago ng 'habang'.");
	ExprPtr cond = expression();
	consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
	StmtPtr body = requireBlock();
	return std::make_unique<WhileStmt>(std::move(cond), std::move(body), loc);
}

StmtPtr Parser::doWhileStmt(ErrorLoc loc) {
	StmtPtr body = requireBlock();
	consume(TokenType::HABANG,
	    "kinakailangan ang 'habang' bago ng 'gawin' block.");
	consume(TokenType::LEFT_PAREN, "kinakailangan ang '('.");
	ExprPtr cond = expression();
	consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
	consume(TokenType::SEMICOLON, "kinakailangan ang ';'.");
	return std::make_unique<DoWhileStmt>(std::move(body), std::move(cond), loc);
}

StmtPtr Parser::forStmt(ErrorLoc loc) {
	consume(TokenType::LEFT_PAREN, "kinakailangan ang '(' bago ng 'para'.");
	StmtPtr init;
	if(match(TokenType::ITAKDA)) init = varDecl(previous().loc());
	else init = assignStmt(peek().loc());
	ExprPtr cond = expression();
	consume(TokenType::SEMICOLON, "kinakailangan ang ';' bago ng kondisyon.");
	StmtPtr update = assignStmtNoSemi(peek().loc());
	consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
	StmtPtr body = requireBlock();
	return std::make_unique<ForStmt>(std::move(init), std::move(cond),
	    std::move(update), std::move(body), loc);
}

StmtPtr Parser::switchStmt(ErrorLoc loc) {
	consume(TokenType::LEFT_PAREN, "kinakailangan ang '(' bago ng 'pili'.");
	ExprPtr subj= expression();
	consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
    const DepthCheck guard(nestingDepth, MAX_NESTING_DEPTH, loc,
	    "Sumobra ang lalim ng expression");
	consume(TokenType::LEFT_BRACE, "kinakailangan ang '{'.");
	std::vector<SwitchCase> cases;
	while (match(TokenType::KASO)) {
	    const bool negative = match(TokenType::MINUS);
		const Token numtkn = consume(TokenType::NUMBER,
		    "kinakailangan ang numero bago ng 'kaso'.");
		const int val = parseIntLiteral(numtkn, negative);
		consume(TokenType::COLON, "kinakailangan ang ':' bago ng 'kaso'.");
		std::vector<StmtPtr> body;
		while (!check(TokenType::KASO) && !check(TokenType::RIGHT_BRACE) &&
		    !isAtEnd()) {
		    body.push_back(statement());
		}
		cases.push_back({val, numtkn.loc(), std::move(body)});
		}
	consume(TokenType::RIGHT_BRACE, "kinakailangan ang '}'.");
	return std::make_unique<SwitchStmt>(std::move(subj), std::move(cases), loc);
}

StmtPtr Parser::readStmt(ErrorLoc loc) {
    advance();
	consume(TokenType::LEFT_PAREN,
	    "kinakailangan ang '(' bago ng 'basahin' o 'kunin'.");
    const Token name = consume(TokenType::IDENTIFIER,
	    "kinakailangan ang pangalan ng variable sa 'basahin(...)'. ");
	consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
    consume(TokenType::SEMICOLON, "kinakailangan ang ';' bago ng 'tigil'.");
	return std::make_unique<AssignStmt>(name.lexeme,
	    std::make_unique<ReadExpr>(loc), name.loc());
}

// expressions
// the parsing functions for the expressions
// follows a top to bottom precedence
// checks 3.2

ExprPtr Parser::expression() {
    const DepthCheck guard(exprDepth, MAX_EXPR_DEPTH, peek().loc(),
	    "Masyadong malalim ang pugad ng expression");
	return logicalOr();
}

ExprPtr Parser::logicalOr() {
    ExprPtr e = logicalAnd();
	while (check(TokenType::O)) {
	    const ErrorLoc loc = peek().loc();
		advance();
		ExprPtr r = logicalAnd();
		e = std::make_unique<LogicalExpr>(LogOp::OR, std::move(e),
		    std::move(r), loc);
	}
	return e;
}

ExprPtr Parser::logicalAnd() {
    ExprPtr e = logicalNot();
	while (check(TokenType::AT)) {
	    const ErrorLoc loc = peek().loc();
		advance();
		ExprPtr r = logicalNot();
		e = std::make_unique<LogicalExpr>(LogOp::AND, std::move(e),
		    std::move(r), loc);
	}
	return e;
}

ExprPtr Parser::logicalNot(){
    if (check(TokenType::HINDI)){
	    const ErrorLoc loc = peek().loc();
		advance();
		const DepthCheck guard(exprDepth, MAX_EXPR_DEPTH, loc,
		    "Sumobra ang lalim ng expression");
		ExprPtr operand = logicalNot();
		return std::make_unique<UnaryNotExpr>(std::move(operand), loc);
	}
	return relational();
}

bool Parser::isRelationalOp(TokenType t) {
    switch(t) {
	    case TokenType::GREATER:
		case TokenType::LESS:
		case TokenType::GREATER_EQUAL:
		case TokenType::LESS_EQUAL:
		case TokenType::EQUAL_EQUAL:
		case TokenType::NOT_EQUAL:
		    return true;
        default:
		    return false;
	}
}

BinOp Parser::binOpFor(TokenType t) {
    switch (t) {
	    case TokenType::PLUS: return BinOp::ADD;
	    case TokenType::MINUS: return BinOp::SUB;
	    case TokenType::STAR: return BinOp::MUL;
	    case TokenType::PERCENT: return BinOp::MOD;
	    case TokenType::SLASH: return BinOp::DIV;
	    case TokenType::GREATER: return BinOp::GT;
	    case TokenType::GREATER_EQUAL: return BinOp::GE;
	    case TokenType::LESS: return BinOp::LT;
	    case TokenType::LESS_EQUAL: return BinOp::LE;
	    case TokenType::EQUAL_EQUAL: return BinOp::EQ;
	    case TokenType::NOT_EQUAL: return BinOp::NEQ;
		default: break;
	}
	throw CompileError("Hindi kilalang operator", {0, 0});
}

ExprPtr Parser::relational() {
    ExprPtr e = term();
	for (;;) {
	    if (!isRelationalOp(peek().type)) break;
	    const ErrorLoc loc = peek().loc();
		const BinOp op = binOpFor(advance().type);
		ExprPtr r = term();
		e = std::make_unique<BinaryExpr>(op, std::move(e), std::move(r), loc);
	}
	return e;
}

ExprPtr Parser::term() {
    ExprPtr e = factor();
	for (;;) {
	    if (!check(TokenType::PLUS) && !check(TokenType::MINUS)) break;
	    const ErrorLoc loc = peek().loc();
		const BinOp op = binOpFor(advance().type);
		ExprPtr r = factor();
		e = std::make_unique<BinaryExpr>(op, std::move(e), std::move(r), loc);
	}
	return e;
}

ExprPtr Parser::factor() {
    ExprPtr e = unary();
	for (;;) {
	    if (!check(TokenType::STAR) && !check(TokenType::SLASH) &&
		    !check(TokenType::PERCENT)) break;
	    const ErrorLoc loc = peek().loc();
		const BinOp op = binOpFor(advance().type);
		ExprPtr r = unary();
		e = std::make_unique<BinaryExpr>(op, std::move(e), std::move(r), loc);
	}
	return e;
}

ExprPtr Parser::unary(){
    if (check(TokenType::MINUS) && checkNext(TokenType::NUMBER)) {
	    const ErrorLoc loc = peek().loc();
		advance();
		const Token& tkn = advance();
        return std::make_unique<LiteralIntExpr>(parseIntLiteral(tkn, true),
            loc);
    }

    if (check(TokenType::MINUS)) {
        const ErrorLoc loc = peek().loc();
        advance();
        const DepthCheck guard(exprDepth, MAX_EXPR_DEPTH, loc,
            "Sumobra ang lalim ng expression");
        ExprPtr operand = unary();
		return std::make_unique<UnaryNegExpr>(std::move(operand), loc);
	}
	return primary();
}

ExprPtr Parser::primary() {
	const ErrorLoc loc = peek().loc();
    if (check(TokenType::NUMBER)){
		const Token& tkn = advance();
		return std::make_unique<LiteralIntExpr>(parseIntLiteral(tkn), loc);
	}
    if (match(TokenType::TOTOO))
	    return std::make_unique<LiteralBoolExpr>(true, loc);
    if (match(TokenType::MALI))
	    return std::make_unique<LiteralBoolExpr>(false, loc);
	if (check(TokenType::BASAHIN) || check(TokenType::KUNIN)) {
		advance();
		consume(TokenType::LEFT_PAREN,
		    "kinakailangan ang '(' bago ng 'BASAHIN' o 'KUNIN'.");
		consume(TokenType::RIGHT_PAREN, "kinakailangan ang ')'.");
		return std::make_unique<ReadExpr>(loc);
	}
    if (check(TokenType::IDENTIFIER)){
		return std::make_unique<VariableExpr>(advance().lexeme, loc);
	}
    if (match(TokenType::LEFT_PAREN)){
		ExprPtr e = expression();
		consume(TokenType::RIGHT_PAREN,
		    "kinakailangan ang ')' bago ng expression.");
		return e;
	}
	throw CompileError("Hindi inaasahang simbolo sa expression: '" +
	    peek().lexeme + "'.", loc, static_cast<int>(peek().lexeme.size()));
}

}
