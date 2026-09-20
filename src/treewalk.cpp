#include "treewalk.h"
#include "ast.h"
#include "common.h"
#include "value.h"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <chrono>
#include <vector>

namespace filcompiler {
namespace {

constexpr std::int64_t kIntMin = std::numeric_limits<int>::min();
constexpr std::int64_t kIntMax = std::numeric_limits<int>::max();
bool fit(std::int64_t v) {
    return v >= kIntMin && v  <= kIntMax;
}

}

void TreeWalker::requireNumbers(const Value& l, const Value& r, ErrorLoc loc) {
    if (l.type != Value::Type::VT_INT || r.type != Value::Type::VT_INT)
	    throw Fault{"Ang pwede ilagay sa operand ay mga numero lamang", loc};
}

void TreeWalker::requireNumber(const Value& v, ErrorLoc loc){
    if (v.type != Value::Type::VT_INT)
	    throw Fault{"Ang pwede ilagay sa operand ay mga numero lamang", loc};
}

Value TreeWalker::load(const std::string& name, int slot, ErrorLoc loc) {
    if (storage == Storage::Slots) {
	    const auto i = static_cast<std::size_t>(slot);
		if (i >= definedSlot.size() || !definedSlot[i])
		    throw Fault{"Hindi pa naitakda ang variable '" + name + "'.", loc};
		return bySlot[i];
	}

	const auto it = byName.find(name);
	if (it == byName.end())
	    throw Fault{"Hindi pa naitakda ang variable '" + name + "'.", loc};
	return it->second;
}

void TreeWalker::store(const std::string& name, int slot, const Value& v) {
    if (storage == Storage::Slots) {
	    const auto i = static_cast<std::size_t>(slot);
		bySlot[i] = v;
		definedSlot[i] = true;
	} else {
	    byName[name]  = v;
	}
}

Value TreeWalker::eval(Expr& e)  {
    ++nodes;
	const ErrorLoc loc {e.line, e.column};

	switch(e.kind) {
	    case ExprKind::LITERAL_INT:
		    return Value::makeInt(static_cast<LiteralIntExpr&>(e).value);
	    case ExprKind::LITERAL_BOOL:
		    return Value::makeBool(static_cast<LiteralBoolExpr&>(e).value);
	    case ExprKind::VARIABLE:{
			auto& v = static_cast<VariableExpr&>(e);
		    return load(v.name, v.slot, loc);
		}
		case ExprKind::READ: {
                int x = 0;
                if (prompt)
                    //making the prompt appear before the program waits
                    *prompt << "input: " <<std::flush;
				const auto r0 = std::chrono::steady_clock::now();
				const bool got = static_cast<bool>(in >> x);
				readMs += std::chrono::duration<double, std::milli>
				    (std::chrono::steady_clock::now() - r0).count();
                if (!got)
                    throw Fault{"Hindi tama ang nilagay na integer.", loc};
                return Value::makeInt(x);
		}
		case ExprKind::UNARY_NEG: {
		    const Value v = eval(*static_cast<UnaryNegExpr&>(e).operand);
			requireNumber(v, loc);
			if (v.i == std::numeric_limits<int>::min())
			    throw Fault{"Lumagpas sa saklaw ng 32 bit integer", loc};
			return Value::makeInt(-v.i);
		}
		case ExprKind::UNARY_NOT:
		    return Value::makeBool(!eval(*static_cast<UnaryNotExpr&>(e).
			    operand).truth());
		case ExprKind::LOGICAL: {
		    auto& l = static_cast<LogicalExpr&>(e);
			const Value left = eval(*l.left);
			if (l.op == LogOp::AND && !left.truth())
			    return left;
			if (l.op == LogOp::OR && left.truth())
			    return left;
			return eval(*l.right);
		}
		case ExprKind::BINARY: {
		    auto& b = static_cast<BinaryExpr&>(e);
			const Value l = eval(*b.left);
			const Value r = eval(*b.right);

			if (b.op == BinOp::EQ)
			    return Value::makeBool(rawEq(l,r));
			if (b.op == BinOp::NEQ)
			    return Value::makeBool(!rawEq(l,r));

			requireNumbers(l, r, loc);
			const std::int64_t a = l.i;
			const std::int64_t c = r.i;

			switch (b.op) {
			    case BinOp::GT:
				    return Value::makeBool(a>c);
			    case BinOp::LT:
				    return Value::makeBool(a<c);
			    case BinOp::GE:
				    return Value::makeBool(a>=c);
			    case BinOp::LE:
				    return Value::makeBool(a<=c);
				case BinOp::DIV:
				    if (c==0)
					    throw Fault{"Hindi ito pwede hatiin sa zero.", loc};
				    if (a==kIntMin && c == -1)
					    throw Fault{"Lumagpas sa saklwa ng 32 bit integer",
							loc};
					return Value::makeInt(static_cast<int>(a/c));
				case BinOp::MOD:
				    if (c==0)
					    throw Fault{"Hindi ito pwede hatiin sa zero.", loc};
				    if (a==kIntMin && c == -1)
					    throw Fault{"Lumagpas sa saklwa ng 32 bit integer",
						    loc};
					return Value::makeInt(static_cast<int>(a%c));
				default:
				    break;
			}

			std::int64_t r64 = 0;
			const char* definition = "";
			if (b.op == BinOp::ADD) {
			    r64 = a + c;
				definition = "pagdaragdag";
			}
			if (b.op == BinOp::SUB) {
			    r64 = a - c;
				definition = "pagbabawas";
			}
			if (b.op == BinOp::MUL) {
			    r64 = a * c;
				definition = "pagpaparami";
			}
			if (!fit(r64))
			    throw Fault{std::string(
				    "Lumagpas sa saklaw ng 32 bit integer ang ")
					+ definition, loc};
			return Value::makeInt(static_cast<int>(r64));

		}

    }
	return Value::makeInt(0);
}

TreeWalker::Flow TreeWalker::exec(Stmt& s)  {
    ++nodes;
	switch (s.kind) {
	    case StmtKind::FOR: {
		    auto& f = static_cast<ForStmt&>(s);
			exec(*f.init);
			while (eval(*f.condition).truth()){
			    const Flow fl = exec(*f.body);
				if (fl == Flow::Break)
				    break;
				exec(*f.update);
			}
			break;
		}
	    case StmtKind::IF: {
		    auto& i = static_cast<IfStmt&>(s);
			if (eval(*i.condition).truth())
			    return exec(*i.thenBranch);
			if(i.elseBranch)
			    return exec(*i.elseBranch);

			break;
		}
		case StmtKind::WHILE:{
		    auto& w = static_cast<WhileStmt&>(s);
			while (eval(*w.condition).truth()){
			    const Flow f = exec(*w.body);
				if (f == Flow::Break)
				    break;

			}
			break;
		}
		case StmtKind::DO_WHILE:{
		    auto& d = static_cast<DoWhileStmt&>(s);
			do {
			    const Flow f = exec(*d.body);
				if (f == Flow::Break)
				    break;
			} while (eval(*d.condition).truth());
			break;
		}
		case StmtKind::SWITCH: {
		    auto& sw = static_cast<SwitchStmt&>(s);
			const Value subject = eval(*sw.subject);
			for (auto& c : sw.cases) {
			    if(!rawEq(subject, Value::makeInt(c.value)))
				    continue;
				for (auto& st : c.body)  {
				    const Flow f = exec(*st);
					if (f == Flow::Break)
					    return Flow::Normal;
					if (f == Flow::Continue)
					    return f;
				}
				break;
			}
			break;
		}
		case StmtKind::BREAK:
		    return Flow::Break;
		case StmtKind::CONTINUE:
		    return Flow::Continue;
	    case StmtKind::BLOCK: {
		    auto& b = static_cast<BlockStmt&>(s);
			for (auto& st : b.statements) {
			    const Flow f = exec(*st);
				if (f != Flow::Normal)
				    return f;
			}
			break;
		}
	    case StmtKind::VAR_DECL: {
		    auto& v = static_cast<VarDeclStmt&>(s);
			store(v.name, v.slot, eval(*v.initializer));
			break;
		}
	    case StmtKind::ASSIGN: {
		    auto& a = static_cast<AssignStmt&>(s);
			store(a.name, a.slot, eval(*a.value));
			break;
		}
	    case StmtKind::PRINT: {
		    auto& p = static_cast<PrintStmt&>(s);
			if (p.isString)
			    out << p.stringLiteral << '\n';
			else {
			    printValue(out, eval(*p.expr));
				out << '\n';
			}
			break;
		}

	}
	return Flow::Normal;
}

int TreeWalker::resolve(const std::string& name)  {
    const auto it = slotOf.find(name);
    if(it != slotOf.end())
        return it->second;
    const int idx = static_cast<int>(slotOf.size());
    slotOf.emplace(name, idx);
    return idx;
}

void TreeWalker::resolveExpr(Expr& e)  {
    switch(e.kind)  {
        case ExprKind::VARIABLE: {
            auto& v = static_cast<VariableExpr&>(e);
            v.slot = resolve(v.name);
            break;
        }
        case ExprKind::LOGICAL: {
            auto& l = static_cast<LogicalExpr&>(e);
            resolveExpr(*l.left);
            resolveExpr(*l.right);
            break;
        }
        case ExprKind::UNARY_NOT: {
            resolveExpr(*static_cast<UnaryNotExpr&>(e).operand);
            break;
        }
        case ExprKind::UNARY_NEG: {
            resolveExpr(*static_cast<UnaryNegExpr&>(e).operand);
            break;
        }
        case ExprKind::BINARY: {
            auto& b = static_cast<BinaryExpr&>(e);
            resolveExpr(*b.left);
            resolveExpr(*b.right);
            break;
        }
        default:
            break;
    }
}

void TreeWalker::resolveStmt(Stmt& s)  {
    switch (s.kind)  {
        case StmtKind::IF: {
            auto& i = static_cast<IfStmt&>(s);
            resolveExpr(*i.condition);
            resolveStmt(*i.thenBranch);
            if (i.elseBranch)
                resolveStmt(*i.elseBranch);
            break;
        }
        case StmtKind::FOR: {
            auto& f = static_cast<ForStmt&>(s);
            resolveStmt(*f.init);
            resolveExpr(*f.condition);
            resolveStmt(*f.update);
            resolveStmt(*f.body);
            break;
        }
        case StmtKind::WHILE: {
            auto& w = static_cast<WhileStmt&>(s);
            resolveExpr(*w.condition);
            resolveStmt(*w.body);
            break;
        }
        case StmtKind::DO_WHILE: {
            auto& d = static_cast<DoWhileStmt&>(s);
            resolveStmt(*d.body);
            resolveExpr(*d.condition);
            break;
        }
        case StmtKind::SWITCH: {
            auto& sw = static_cast<SwitchStmt&>(s);
            resolveExpr(*sw.subject);
            for (auto& c : sw.cases)
                for (auto& st : c.body)
                    resolveStmt(*st);
            break;
        }
        case StmtKind::BLOCK:
            for (auto& st : static_cast<BlockStmt&>(s).statements)
                resolveStmt(*st);
            break;
        case StmtKind::VAR_DECL: {
            auto& v = static_cast<VarDeclStmt&>(s);
            resolveExpr(*v.initializer);
            v.slot = resolve(v.name);
            break;
        }
        case StmtKind::ASSIGN: {
            auto& a = static_cast<AssignStmt&>(s);
            resolveExpr(*a.value);
            a.slot = resolve(a.name);
            break;
        }
        case StmtKind::PRINT: {
            auto& p = static_cast<PrintStmt&>(s);
            if (!p.isString)
                resolveExpr(*p.expr);
            break;
        }
        default:
            break;
    }
}

bool TreeWalker::run(std::vector<StmtPtr>& program) {

	diagnosticsVar.clear();
	nodes = 0;
	byName.clear();
	slotOf.clear();

    if (storage == Storage::Slots)  {
	    for (auto& s : program)  resolveStmt(*s);
		bySlot.assign(slotOf.size(), Value::makeInt(0));
		definedSlot.assign(slotOf.size(), false);
	}
	try {
	    for(auto& s : program)
		    exec(*s);
	} catch (const Fault& f)  {
	    diagnosticsVar.push_back({Stage::Runtime, f.message, f.loc, 1});
		return false;
	}
	return true;

}

}

