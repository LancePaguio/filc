#ifndef COMPILER_H
#define COMPILER_H

#include "common.h"
#include "ast.h"
#include "opcode.h"

#include <vector>
#include <cstddef>

namespace filcompiler {

class BytecodeCompiler {
public:
    Chunk compile(std::vector<StmtPtr>& program);

private:
    struct Scope {
	    enum class Kind {Loop, Switch};
		Kind kind;
		int slots;
		std::vector<int> breakJumps;
		std::vector<int> continueJumps;
	};

	Chunk chunk;
	std::vector<Scope> scopes;
	void compileStmt(Stmt& s);
	void compileExpr(Expr& e);

	int emitJump(std::size_t targetScopeIndex, ErrorLoc l);

	int here() const {
	    return static_cast<int>(chunk.code.size());
	}


};

}

#endif // !COMPILER_H
