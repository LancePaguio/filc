#include "semantic.h"
#include "ast.h"
#include "common.h"
#include <set>
#include <vector>

namespace filcompiler{

void SemanticAnalyzer::analyze(std::vector<StmtPtr>& program) {
	for (auto& s : program) analyzeStmt(*s);
}

std::vector<std::string> SemanticAnalyzer::declaredVariables() const {
    std::vector<std::string> out(declared.size());
	for (const auto& ky : declared)
	    out[static_cast<std::size_t>(ky.second.order)] = ky.first;
	return out;
}

int SemanticAnalyzer::declarationLine(const std::string& name) const {
    const auto it = declared.find(name);
	return it == declared.end() ? -1 : it->second.declaredLine;
}

// this is for 4.5
// it simply push a message without throwing so it can
// continue walking
void SemanticAnalyzer::error(std::string msg, ErrorLoc loc, int len) {
    hadErrorVar = true;
	diagnosticsVar.push_back({Stage::Semantic, std::move(msg), loc, len});
}

// this is for 4.4
// report unknown global names
bool SemanticAnalyzer::reportUndeclared(const std::string& name, ErrorLoc loc) {
    hadErrorVar = true;
	if (!reportUndeclaredVar.insert(name).second)
	    return false;
	error("Ginamit and variable '" + name + "' bago ito maideklara.", loc,
	    static_cast<int>(name.size()));
	return true;
}

// 4.2 ast traversal
// it does one walk so it can report multiple
// semantic errors
void SemanticAnalyzer::analyzeStmt(Stmt& s) {
    switch (s.kind) {
	    case StmtKind::VAR_DECL: {
		    auto& v = static_cast<VarDeclStmt&>(s);
			analyzeExpr(*v.initializer);
			const auto it = declared.find(v.name);
			if (it != declared.end()) {
			    error("naideklara na ang variable '" + v.name + "' sa linya " +
				    std ::to_string(it->second.declaredLine) + ".",
					{s.line, s.column});

			}else {
			    declared.emplace(v.name, VarInfo{s.line,
				    static_cast<int>(declared.size())});
			}
			break;
		}
		case StmtKind::ASSIGN: {
		    auto& a = static_cast<AssignStmt&>(s);
			analyzeExpr(*a.value);
			if (!declared.count(a.name)) reportUndeclared(a.name,
			    {s.line, s.column});
			break;
		}
		case StmtKind::PRINT: {
		    auto& p = static_cast<PrintStmt&>(s);
			if (!p.isString) analyzeExpr(*p.expr);
			break;
		}
		case StmtKind::BLOCK: {
		    auto& b = static_cast<BlockStmt&>(s);
			for (auto& st : b.statements) analyzeStmt(*st);
			break;
		}
		case StmtKind::IF: {
		    auto& i = static_cast<IfStmt&>(s);
			analyzeExpr(*i.condition);
			analyzeStmt(*i.thenBranch);
			if (i.elseBranch) analyzeStmt(*i.elseBranch);
			break;
		}
		case StmtKind::WHILE: {
		    auto& w = static_cast<WhileStmt&>(s);
			analyzeExpr(*w.condition);
			++loopDepth; ++breakDepth;
			analyzeStmt(*w.body);
			--loopDepth; --breakDepth;
			break;
		}
		case StmtKind::DO_WHILE: {
		    auto& d = static_cast<DoWhileStmt&>(s);
			++loopDepth; ++breakDepth;
			analyzeStmt(*d.body);
			--loopDepth; --breakDepth;
			analyzeExpr(*d.condition);
			break;
		}
		case StmtKind::FOR: {
		    auto& f = static_cast<ForStmt&>(s);
			analyzeStmt(*f.init);
			analyzeExpr(*f.condition);
			analyzeStmt(*f.update);
			++loopDepth; ++breakDepth;
			analyzeStmt(*f.body);
			--loopDepth; --breakDepth;
			break;
		}
		// ensure that each labels are distinct from each
		// other for 4.3
		case StmtKind::SWITCH: {
		    auto& sw = static_cast<SwitchStmt&>(s);
			analyzeExpr(*sw.subject);
			std::set<int>seen;
			for (const auto& c : sw.cases){
			    if (!seen.insert(c.value).second){
				    error("Paulit-ulit ang halaga ng kaso: " + std::
					    to_string(c.value) + ".", c.loc);
				}
			    if (c.body.empty()) {
				    error("Walang laman ang 'kaso" + std::to_string(c.value) +
					    "'. Kailangang may sariling pahayag ang bawat kaso.",
						c.loc);
				}
			}
			++breakDepth;
			for(auto& c : sw.cases)
			    for (auto& st : c.body) analyzeStmt(*st);
			--breakDepth;
			break;
		}
		case StmtKind::BREAK:
		    if (breakDepth == 0)
			    error("Ang 'tigil' ay dapat lamang sa loob ng loop o 'pili'.",
				{s.line, s.column});
		    break;
		case StmtKind::CONTINUE:
		    if (loopDepth == 0)
			    error("Ang 'sunod' ay dapat lamang sa loob ng loop.",
				{s.line, s.column});
		    break;
	}
}

void SemanticAnalyzer::analyzeExpr(Expr& e) {
    switch (e.kind) {
	    case ExprKind::LITERAL_INT:
	    case ExprKind::LITERAL_BOOL:
	    case ExprKind::READ:
		    break;

        case ExprKind::VARIABLE:{
		    auto& v = static_cast<VariableExpr&>(e);
			if (!declared.count(v.name)) reportUndeclared(v.name,
			{e.line, e.column});
			break;
		}
	    case ExprKind::UNARY_NEG:
			analyzeExpr(*static_cast<UnaryNegExpr&>(e).operand);
			break;
	    case ExprKind::UNARY_NOT:
			analyzeExpr(*static_cast<UnaryNotExpr&>(e).operand);
			break;
	    case ExprKind::BINARY:{
		    auto& b = static_cast<BinaryExpr&>(e);
			analyzeExpr(*b.left);
			analyzeExpr(*b.right);
			break;
		}
		case ExprKind::LOGICAL: {
		    auto& b = static_cast<LogicalExpr&>(e);
			analyzeExpr(*b.left);
			analyzeExpr(*b.right);
			break;
		}

    }
}

}
