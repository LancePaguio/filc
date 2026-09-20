#ifndef TREEWALK_H
#define TREEWALK_H


#include "common.h"
#include "ast.h"
#include "value.h"

#include <cerrno>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace filcompiler {
class TreeWalker {
public:
    enum class Storage { Map, Slots };

    TreeWalker(std::istream& input, std::ostream& output, Storage mode)
	    : in(input), out(output), storage(mode) {}

	bool run(std::vector<StmtPtr>& program);

	void setPrompt(std::ostream* promptOut){
	    prompt = promptOut;
	}

	long long nodesVisited() const {
	    return nodes;
	}

	double readWaitMs() const {
	    return readMs;
	}

	const std::vector<Diagnostic>& diagnostics() const {
	    return diagnosticsVar;
	}

private:

	enum class Flow { Normal, Break, Continue };
    struct Fault {
	    std::string message;
		ErrorLoc loc;
	};

	std::istream& in;
	std::ostream& out;
	Storage storage;
	long long nodes = 0;
	double readMs = 0.0;
	std::ostream* prompt = nullptr;
	std::vector<Diagnostic> diagnosticsVar;
	std::unordered_map<std::string, int> slotOf;
	std::unordered_map<std::string, Value> byName;
	std::vector<Value>  bySlot;
	std::vector<bool>  definedSlot;

	int resolve(const std::string& name);
	void resolveStmt(Stmt& s);
	void resolveExpr(Expr& e);

	Value load(const std::string& name, int slot, ErrorLoc loc);
	Flow exec(Stmt& s);
	Value eval(Expr& e);
	void store(const std::string& name, int slot, const Value& v);
	static void requireNumbers(const Value& l, const Value& r, ErrorLoc loc);
	static void requireNumber(const Value& v, ErrorLoc loc);

};

}

#endif // !TREEWALK_H
