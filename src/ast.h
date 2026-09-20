#ifndef AST_H
#define AST_H

#include "common.h"

#include <memory>
#include <string>
#include <vector>

namespace filcompiler {

// we will be doing two syntax trees for expressions and statements
// checks 3.1

// 1. expressions
enum class ExprKind { VARIABLE, LITERAL_BOOL, LITERAL_INT, UNARY_NEG, UNARY_NOT,
    BINARY, LOGICAL, READ };

struct Expr {
    ExprKind kind;
	int line;
	int column;
	virtual ~Expr() = default;

protected:
    Expr(ExprKind k, ErrorLoc l) : kind(k), line(l.line), column(l.column){}
};

// 2. statements
enum class StmtKind {IF, WHILE, DO_WHILE, FOR, SWITCH, BREAK, CONTINUE, BLOCK,
	ASSIGN, PRINT, VAR_DECL};

struct Stmt {
    StmtKind kind;
	int line;
	int column;
	virtual ~Stmt() = default;
protected:
    Stmt(StmtKind k, ErrorLoc l) : kind(k), line(l.line), column(l.column){}
};

// 3. Operators
// this is for the ast printer to render them as text
enum class BinOp {ADD, SUB, MUL, DIV, MOD, GT,GE, EQ, NEQ, LT, LE};
enum class LogOp {AND, OR};

inline const char* binOpLexeme(BinOp op) {
    switch (op) {
	    case BinOp::ADD: return "+";
	    case BinOp::SUB: return "-";
	    case BinOp::MUL: return "*";
	    case BinOp::DIV: return "/";
	    case BinOp::MOD: return "%";
	    case BinOp::LT: return "<";
	    case BinOp::LE: return "<=";
	    case BinOp::GT: return ">";
	    case BinOp::GE: return ">=";
	    case BinOp::EQ: return "==";
	    case BinOp::NEQ: return "!=";
	}
	return "?";
}

inline const char* logOpLexeme(LogOp op) {
    return op == LogOp::AND ? "at" : "o";
}

// going to use unique pointers because
// it makes ownership explicit which works
// well with hierarchies and can destroy itself
//
// 1. Expressions
using ExprPtr = std::unique_ptr<Expr>;

struct LiteralIntExpr : Expr {
    int value;
	LiteralIntExpr(int v, ErrorLoc l)
	    : Expr(ExprKind::LITERAL_INT, l), value(v) {}
};

struct VariableExpr : Expr {
    int slot = -1;
    std::string name;
	VariableExpr(std::string n, ErrorLoc l)
	    : Expr(ExprKind::VARIABLE, l), name(std::move(n)) {}
};

struct LiteralBoolExpr : Expr {
    bool value;
	LiteralBoolExpr(bool v, ErrorLoc l)
	    : Expr(ExprKind::LITERAL_BOOL, l), value(v) {}
};


struct UnaryNegExpr : Expr {
    ExprPtr operand;
    UnaryNegExpr(ExprPtr o, ErrorLoc l)
	    : Expr(ExprKind::UNARY_NEG, l), operand(std::move(o)) {}
};

struct UnaryNotExpr : Expr {
    ExprPtr operand;
    UnaryNotExpr(ExprPtr o, ErrorLoc l)
	    : Expr(ExprKind::UNARY_NOT, l), operand(std::move(o)) {}
};

struct BinaryExpr : Expr {
    BinOp op;   // operator
	ExprPtr left, right;  // operands
	BinaryExpr(BinOp o, ExprPtr l_, ExprPtr r_, ErrorLoc l)
	    : Expr(ExprKind::BINARY, l), op(o),
		  left(std::move(l_)), right(std::move(r_)) {}
};

struct LogicalExpr : Expr {
    LogOp op;
	ExprPtr left, right;
	LogicalExpr(LogOp o, ExprPtr l_, ExprPtr r_, ErrorLoc l)
	    : Expr(ExprKind::LOGICAL, l),
		  op(std::move(o)), left(std::move(l_)),
		  right(std::move(r_)) {}
};

struct ReadExpr : Expr {
    explicit ReadExpr(ErrorLoc l) : Expr(ExprKind::READ, l) {}
};

// 2. Statements
using StmtPtr = std::unique_ptr<Stmt>;


struct IfStmt : Stmt {
    ExprPtr condition;
	StmtPtr thenBranch;
	StmtPtr elseBranch;
	IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, ErrorLoc l)
	    : Stmt(StmtKind::IF, l), condition(std::move(c)),
		  thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};

struct WhileStmt :Stmt {
    ExprPtr condition;
	StmtPtr body;
	WhileStmt(ExprPtr c, StmtPtr b, ErrorLoc l)
	    : Stmt(StmtKind::WHILE, l), condition(std::move(c)),
		  body(std::move(b)) {}
};

struct DoWhileStmt : Stmt {
    StmtPtr body;
	ExprPtr condition;
	DoWhileStmt(StmtPtr b, ExprPtr c, ErrorLoc l)
	    : Stmt(StmtKind::DO_WHILE, l), body(std::move(b)),
		  condition(std::move(c)) {}
};

struct ForStmt : Stmt {
    StmtPtr init;
	ExprPtr condition;
	StmtPtr update;
	StmtPtr body;
	ForStmt(StmtPtr i, ExprPtr c, StmtPtr u, StmtPtr b, ErrorLoc l)
	    : Stmt(StmtKind::FOR, l), init(std::move(i)),
		  condition(std::move(c)), update(std::move(u)),
		  body(std::move(b)) {}
};

struct SwitchCase{
    int value;
	ErrorLoc loc;
	std::vector<StmtPtr> body;
};

struct SwitchStmt : Stmt {
    ExprPtr subject;
	std::vector<SwitchCase> cases;
	SwitchStmt(ExprPtr s,std::vector<SwitchCase> c, ErrorLoc l)
	    : Stmt(StmtKind::SWITCH, l), subject(std::move(s)),
		cases(std::move(c)) {}
};

struct BreakStmt : Stmt {
    explicit BreakStmt(ErrorLoc l)
	    : Stmt(StmtKind::BREAK, l) {}
};

struct ContinueStmt : Stmt {
    explicit ContinueStmt(ErrorLoc l)
		: Stmt(StmtKind::CONTINUE, l) {}
};

struct AssignStmt : Stmt {
    std::string name;
	ExprPtr value;
	int slot = -1;
	AssignStmt(std::string n, ExprPtr v, ErrorLoc l)
	    : Stmt(StmtKind::ASSIGN, l), name(std::move(n)),
		value(std::move(v)) {}
};

struct PrintStmt :Stmt {
    ExprPtr expr;
	std::string stringLiteral;
	bool isString;
	PrintStmt(ExprPtr e, std::string s, bool isStr, ErrorLoc l)
	    : Stmt(StmtKind::PRINT, l), expr(std::move(e)),
		stringLiteral(std::move(s)), isString(isStr) {}
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
	BlockStmt(std::vector<StmtPtr> s, ErrorLoc l)
	    : Stmt(StmtKind::BLOCK, l), statements(std::move(s)){}
};

struct VarDeclStmt : Stmt {
    std::string name;
	ExprPtr initializer;
	int slot = -1;
	VarDeclStmt(std::string n, ExprPtr init, ErrorLoc l)
		: Stmt(StmtKind::VAR_DECL, l), name(std::move(n)),
		initializer(std::move(init)) {}
};


}

#endif // AST_H
