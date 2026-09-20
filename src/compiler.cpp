#include "compiler.h"
#include "ast.h"
#include "opcode.h"

#include <cassert>
#include <cstddef>

namespace filcompiler {

Chunk BytecodeCompiler::compile(std::vector<StmtPtr>& program) {
    chunk = Chunk{};
	scopes.clear();
	for(auto& s : program) compileStmt(*s);

	const ErrorLoc end = program.empty()
	    ? ErrorLoc{1, 1}
		: ErrorLoc{program.back()->line, program.back()->column};
	chunk.emit(OpCode::OP_HALT, 0, end.line, end.column);
	return std::move(chunk);

}

int BytecodeCompiler::emitJump(std::size_t targetScopeIndex, ErrorLoc l){
    for (std::size_t j = scopes.size(); j > targetScopeIndex + 1; --j)  {
	    for (int k = 0; k < scopes[j-1].slots; ++k)
		    chunk.emit(OpCode::OP_POP, 0, l.line, l.column);
	}
	return chunk.emit(OpCode::OP_JUMP, -1, l.line, l.column);
}

void BytecodeCompiler::compileStmt(Stmt& s) {
    switch (s.kind) {
	    case StmtKind::ASSIGN: {
		    auto& a = static_cast<AssignStmt&>(s);
			compileExpr(*a.value);
			chunk.emit(OpCode::OP_SET_GLOBAL, chunk.globalSlot(a.name),
				s.line, s.column);
			break;
		}
	    case StmtKind::BLOCK: {
		    auto& b = static_cast<BlockStmt&>(s);
			for (auto& st : b.statements) compileStmt(*st);
			break;
		}
	    case StmtKind::PRINT: {
		    auto& p = static_cast<PrintStmt&>(s);
			if (p.isString) {
				chunk.emit(OpCode::OP_PRINT_STR,
				chunk.addStringConstant(p.stringLiteral), s.line, s.column);
			} else {
			    compileExpr(*p.expr);
				chunk.emit(OpCode::OP_PRINT_VAL, 0,
					s.line, s.column);

			}
			break;
		}
		// for defining and reaasignment of an identifier
		// pops the value of the expression and pushes it
		// into a slot which will tracked later on
		// for 5.4
	    case StmtKind::VAR_DECL: {
		    auto& v = static_cast<VarDeclStmt&>(s);
			compileExpr(*v.initializer);
			chunk.emit(OpCode::OP_DEFINE_GLOBAL, chunk.globalSlot(v.name),
				s.line, s.column);
			break;
		}

		// the following statements are conditionals jumps
		// for 5.5

		case StmtKind:: BREAK: {
		    assert(!scopes.empty() && "'tigil' sa labas ng loop o 'pili'");
			const std::size_t target = scopes.size() - 1;
			const int j = emitJump(target, {s.line, s.column});
			scopes[target].breakJumps.push_back(j);
			break;
		}
		// here we need it to look for the neares loop
		// when it skips something
		case StmtKind:: CONTINUE:{
			std::size_t n = scopes.size();
			while (n > 0 && scopes[n - 1].kind != Scope::Kind::Loop) --n;
            assert(n > 0 && "'sunod' ay na sa labas ng loop");
			const std::size_t loop = n - 1;
			const int j = emitJump(loop, {s.line, s.column});
			scopes[loop].continueJumps.push_back(j);
			break;
		}
		case StmtKind::IF: {
		    auto& i = static_cast<IfStmt&>(s);
			compileExpr(*i.condition);
			const int toElse = chunk.emit(OpCode::OP_JUMP_IF_FALSE, -1,
			    s.line, s.column);
			chunk.emit(OpCode::OP_POP, 0, s.line, s.column);
			compileStmt(*i.thenBranch);
			const int toEnd = chunk.emit(OpCode::OP_JUMP, -1, s.line, s.column);
			chunk.patchJump(toElse, here());
		    chunk.emit(OpCode::OP_POP, 0, s.line, s.column);
			if (i.elseBranch) compileStmt(*i.elseBranch);
				chunk.patchJump(toEnd, here());
			break;
		}
		case StmtKind::FOR: {
		    auto& f = static_cast<ForStmt&>(s);
			compileStmt(*f.init);
			const int condStart = here();
			compileExpr(*f.condition);
			const int exitJump = chunk.emit(OpCode::OP_JUMP_IF_FALSE, -1,
			    s.line, s.column);
			chunk.emit(OpCode::OP_POP, 0, s.line, s.column);
			const int bodyJump = chunk.emit(OpCode::OP_JUMP, -1,
				s.line, s.column);
			const int updateStart = here();
			scopes.push_back({Scope::Kind::Loop, 0, {}, {}});
			compileStmt(*f.update);
		    chunk.emit(OpCode::OP_JUMP, condStart, s.line, s.column);
			chunk.patchJump(bodyJump, here());
			compileStmt(*f.body);
		    chunk.emit(OpCode::OP_JUMP, updateStart, s.line, s.column);

			for (const int cj : scopes.back().continueJumps)
			    chunk.patchJump(cj, updateStart);
			chunk.patchJump(exitJump, here());
		    chunk.emit(OpCode::OP_POP, 0, s.line, s.column);

			for (const int bj : scopes.back().breakJumps)
			    chunk.patchJump(bj, here());
		    scopes.pop_back();
			break;
		}

		case StmtKind::WHILE: {
		    auto& w = static_cast<WhileStmt&>(s);
			const int loopsStart = here();
			compileExpr(*w.condition);
			const int exitJump = chunk.emit(OpCode::OP_JUMP_IF_FALSE, -1,
			    s.line, s.column);
			chunk.emit(OpCode::OP_POP, 0, s.line, s.column);

			scopes.push_back({Scope::Kind::Loop, 0, {}, {}});
			compileStmt(*w.body);
			for (const int cj : scopes.back().continueJumps)
			    chunk.patchJump(cj, loopsStart);
			chunk.emit(OpCode::OP_JUMP, loopsStart, s.line, s.column);

			chunk.patchJump(exitJump, here());
			chunk.emit(OpCode::OP_POP, 0, s.line, s.column);
			for (const int bj : scopes.back().breakJumps)
			    chunk.patchJump(bj, here());
		    scopes.pop_back();
			break;
		}

		// loop body first, then the condition
		case StmtKind::DO_WHILE: {
		    auto& d = static_cast<DoWhileStmt&>(s);
			const int bodyStart = here();
			scopes.push_back({Scope::Kind::Loop, 0, {}, {}});
			compileStmt(*d.body);

			const int condStart = here();
			for (const int cj : scopes.back().continueJumps)
			    chunk.patchJump(cj, condStart);

			compileExpr(*d.condition);
			const int exitJump = chunk.emit(OpCode::OP_JUMP_IF_FALSE, -1,
			    s.line, s.column);
			chunk.emit(OpCode::OP_POP, 0, s.line, s.column);
			chunk.emit(OpCode::OP_JUMP, bodyStart, s.line, s.column);

			chunk.patchJump(exitJump, here());
			chunk.emit(OpCode::OP_POP, 0, s.line, s.column);
			for (const int bj : scopes.back().breakJumps)
			    chunk.patchJump(bj, here());
		    scopes.pop_back();
			break;
		}
		// handle switch case statement for 5.6
		case StmtKind::SWITCH: {
		    auto& sw = static_cast<SwitchStmt&>(s);
			compileExpr(*sw.subject);
			scopes.push_back({Scope::Kind::Switch, 1, {}, {}});

			std:: vector<int>  endJumps;
			for (auto& c : sw.cases){
			    chunk.emit(OpCode::OP_DUP, 0, s.line, s.column);
			    chunk.emit(OpCode::OP_CONST_INT, chunk.addIntConstant(c.value),
					c.loc.line, c.loc.column);
			    chunk.emit(OpCode::OP_EQ, 0, s.line, s.column);
			    const int skip = chunk.emit(OpCode::OP_JUMP_IF_FALSE, -1,
				    c.loc.line, c.loc.column);
			    chunk.emit(OpCode::OP_POP, 0, c.loc.line, c.loc.column);

				for (auto& st : c.body) compileStmt(*st);
				endJumps.push_back(chunk.emit(OpCode::OP_JUMP, -1, c.loc.line,
				    c.loc.column));
				chunk.patchJump(skip, here());
			    chunk.emit(OpCode::OP_POP, 0, c.loc.line, c.loc.column);
			}
			const int endLabel = here();
			for (const int el : endJumps) chunk.patchJump(el, endLabel);
			for (const int bj : scopes.back().breakJumps)
			    chunk.patchJump(bj, endLabel);
		    scopes.pop_back();
			chunk.emit(OpCode::OP_POP, 0, s.line, s.column);
			break;
		}
	}
}

// must compile post-order following 5.3
// ensure arithmetics works by doing
// operands first before the operators
// this puts the operands in the stack
// then it emits the operators based
// on the order of parser's precedence
// table
void BytecodeCompiler::compileExpr(Expr& e) {
    switch (e.kind) {
	    case ExprKind::LITERAL_INT: {
		    auto& lit = static_cast<LiteralIntExpr&>(e);
			chunk.emit(OpCode::OP_CONST_INT, chunk.addIntConstant(lit.value),
				e.line, e.column);
			break;
		}
	    case ExprKind::LITERAL_BOOL: {
		    const auto& lit = static_cast<const LiteralBoolExpr&>(e);
			chunk.emit(lit.value ? OpCode::OP_TRUE : OpCode::OP_FALSE, 0,
				e.line, e.column);
			break;
		}
	    case ExprKind::VARIABLE: {
		    const auto& v= static_cast<const VariableExpr&>(e);
			chunk.emit(OpCode::OP_GET_GLOBAL, chunk.globalSlot(v.name),
				e.line, e.column);
			break;
		}
	    case ExprKind::UNARY_NEG: {
		    const auto& u = static_cast<const UnaryNegExpr&>(e);
			compileExpr(*u.operand);
			chunk.emit(OpCode::OP_NEG, 0,
				e.line, e.column);
			break;
		}
	    case ExprKind::UNARY_NOT: {
		    const auto& u = static_cast<const UnaryNotExpr&>(e);
			compileExpr(*u.operand);
			chunk.emit(OpCode::OP_NOT, 0,
				e.line, e.column);
			break;
		}
		case ExprKind::BINARY: {
		    auto& b = static_cast<BinaryExpr&>(e);
			compileExpr(*b.left);
			compileExpr(*b.right);

			OpCode op{};
			switch (b.op) {
				case BinOp::ADD: op = OpCode::OP_ADD; break;
				case BinOp::SUB: op = OpCode::OP_SUB; break;
				case BinOp::MUL: op = OpCode::OP_MUL; break;
				case BinOp::DIV: op = OpCode::OP_DIV; break;
				case BinOp::MOD: op = OpCode::OP_MOD; break;
				case BinOp::GT: op = OpCode::OP_GT;  break;
				case BinOp::GE: op = OpCode::OP_GE;  break;
				case BinOp::LT: op = OpCode::OP_LT;  break;
				case BinOp::LE: op = OpCode::OP_LE;  break;
				case BinOp::EQ: op = OpCode::OP_EQ;  break;
				case BinOp::NEQ: op = OpCode::OP_NEQ; break;
			}
			chunk.emit(op, 0, e.line, e.column);
			break;
		}
		case ExprKind::LOGICAL: {
		    auto& l = static_cast<LogicalExpr&>(e);
			compileExpr(*l.left);
			const OpCode shortJump = (l.op == LogOp::AND) ? OpCode::
			    OP_JUMP_IF_FALSE
			    : OpCode::OP_JUMP_IF_TRUE;
			const int jump = chunk.emit(shortJump, -1, e.line, e.column);
			chunk.emit(OpCode::OP_POP, 0, e.line, e.column);
			compileExpr(*l.right);
			chunk.patchJump(jump, here());
            break;
		}
		case ExprKind::READ:
		    chunk.emit(OpCode::OP_READ, 0, e.line, e.column);
			break;
	}
}

}
