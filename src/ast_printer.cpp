#include "ast_printer.h"
#include "ast.h"

#include <ostream>

namespace filcompiler {
namespace  {
std::string expr(const Expr& e);
std::string stmt(const Stmt& s);

// Expressions

// Using Depth-First Search algo
// to write an operator first for each node,
// then recurse to render each child, to its own leaves
std::string expr(const Expr& e) {
    switch (e.kind){
	    case ExprKind::LITERAL_INT:
            return std::to_string(static_cast<const LiteralIntExpr&>(e).value);
	    case ExprKind::LITERAL_BOOL:
            return static_cast<const LiteralBoolExpr&>(e).value ? "totoo" : "mali";
	    case ExprKind::READ:
            return "(basahin)";
	    case ExprKind::VARIABLE:
            return static_cast<const VariableExpr&>(e).name;
	    case ExprKind::UNARY_NEG:
            return "(neg " + expr(*static_cast<const UnaryNegExpr&>(e).operand) + ")";
	    case ExprKind::UNARY_NOT:
            return "(hindi " + expr(*static_cast<const UnaryNotExpr&>(e).operand) + ")";
	    case ExprKind::BINARY: {
			const auto& b = static_cast<const BinaryExpr&>(e);
			return std::string("(") + binOpLexeme(b.op) + " " + expr(*b.left) + " " + expr(*b.right) + ")";
		}
		case ExprKind::LOGICAL: {
		    const auto& l = static_cast<const LogicalExpr&>(e);
			return std::string("(") + logOpLexeme(l.op) + " " + expr(*l.left) + " " + expr(*l.right) + ")";
		}

	}
	return "(?)";
}

std::string quoted(const std::string& s) {
    return "\"" + s + "\"";
}

std::string join(const std::vector<StmtPtr>& stmts) {
    std::string out;
	for (const auto& s : stmts)  {
		out += " " + stmt(*s);
	}
	return out;
}

// Statements
// Also uses DFS

std::string stmt(const Stmt& s) {
	switch (s.kind) {
	    case StmtKind::VAR_DECL : {
		    const auto& v = static_cast<const VarDeclStmt&>(s);
			return "(itakda " + v.name + " " + expr(*v.initializer) + ")";
		}
	    case StmtKind::PRINT : {
		    const auto& p = static_cast<const PrintStmt&>(s);
			return "(isulat " + (p.isString ? quoted(p.stringLiteral)
			    : expr(*p.expr)) + ")";
		}
	    case StmtKind::ASSIGN : {
		    const auto& a = static_cast<const AssignStmt&>(s);
			return "(= " + a.name + " " + expr(*a.value) + ")";
		}
	    case StmtKind::BLOCK : {
		    const auto& b = static_cast<const BlockStmt&>(s);
			if (b.statements.empty()) return "(block)";
            return "(block" + join(b.statements) + ")";
		}
	    case StmtKind::IF : {
		    const auto& i = static_cast<const IfStmt&>(s);
			std::string out = "(kung " + expr(*i.condition) + " " +
			    stmt(*i.thenBranch);
			if (i.elseBranch)
				out += " " + stmt(*i.elseBranch);
			return out + ")";
		}
	    case StmtKind::WHILE : {
		    const auto& w = static_cast<const WhileStmt&>(s);
			return "(habang " + expr(*w.condition) + " "+ stmt(*w.body) + ")";
		}
	    case StmtKind::DO_WHILE: {
		    const auto& d = static_cast<const DoWhileStmt&>(s);
			return "(gawin " + stmt(*d.body) + " " + expr(*d.condition) + ")";
		}
	    case StmtKind::FOR: {
		    const auto& f = static_cast<const ForStmt&>(s);
            return "(para "  + stmt(*f.init) + " " + expr(*f.condition) + " "
            + stmt(*f.update) + " " + stmt(*f.body) + ")";
		}
	    case StmtKind::SWITCH: {
		    const auto& sw = static_cast<const SwitchStmt&>(s);
			std::string out = "(pili " + expr(*sw.subject);
			for (const auto& c : sw.cases)
                out += " (kaso " + std::to_string(c.value)  + join(c.body) + ")";
            return out + ")";
		}
		case StmtKind::BREAK:    return "(tigil)";
		case StmtKind::CONTINUE:    return "(sunod)";
	}
	return "(?)";
}

}

// wrappers that fix the parameters that functions take

std::string printStmt(const Stmt& s) {
    return stmt(s);
}
std::string printProgram(const std::vector<StmtPtr>& program) {
    std::string out;
	for (std::size_t i = 0; i < program.size(); ++i) {
	    if (i) out += "\n";
		out += stmt(*program[i]);
	}
    return out;
}


}
