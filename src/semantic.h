#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "common.h"
#include "ast.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace filcompiler {

class SemanticAnalyzer {
public:
    void analyze(std::vector<StmtPtr>& program);
	bool hadError() const {
	    return hadErrorVar;
	}
	std::vector<std::string> declaredVariables() const;
	int declarationLine(const std::string& name) const;
	const std::vector<Diagnostic>& diagnostics() const {
	    return diagnosticsVar;
	}

private:
    struct VarInfo{
	    int declaredLine;
	    int order;
	};

	// these resolves 4.1
	// reports undeclared variables or those declared multiple times
	// tigil and sunod not inside a loop
	// analyzes the expression if they are identical
	std::map<std::string, VarInfo> declared;
	bool hadErrorVar = false;
	std::vector<Diagnostic> diagnosticsVar;
	std::set<std::string> reportUndeclaredVar;
	bool reportUndeclared(const std::string& name, ErrorLoc loc);
	int loopDepth = 0;
	int breakDepth = 0;

	void error(std::string msg, ErrorLoc loc, int len = 1);

	void analyzeStmt(Stmt& s);
	void analyzeExpr(Expr& e);

};
}

#endif // !SEMANTIC_H
