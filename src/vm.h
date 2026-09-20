#ifndef VM_H
#define VM_H

#include "common.h"
#include "diagnostic.h"
#include "opcode.h"
#include "value.h"

#include <cmath>
#include <vector>
#include <string>
#include <iosfwd>

namespace filcompiler {

class VM {
public:
    VM();
	VM(std::istream& input, std::ostream& output);
    bool run(const Chunk& chunk);
	void setStepLimit(long long steps) {
	    stepLimit = steps;
	}
	void setTrace(std::ostream* traceOut)  {
	    trace = traceOut;
	}
	// we need print an "input:" message before basahin()
	// so we just add a nullptr here to have it as default
	void setPrompt(std::ostream* promptOut){
	    prompt = promptOut;
	}
	const std::vector<Diagnostic>& diagnostics() const {
	    return diagnosticsVar;
	}

	long long instructionsRetired() const {
	    return retired;
	}
	double readWaitMs() const {
	    return readMs;
	}


private:
    std::istream& in;
    std::ostream& out;
	std::vector<Value> stack;
	std::vector<Diagnostic> diagnosticsVar;
	long long stepLimit = 0;
	long long retired = 0;
	double readMs = 0.0;
	std::ostream* trace = nullptr;
	std::ostream* prompt = nullptr;
	bool fail(std::string msg, const Instr& at);
	void push(const Value& v) {
	    stack.push_back(v);
	}
	Value pop() {
	    Value v = stack.back();
		stack.pop_back();
		return v;
	}

};

}


#endif // !VM_H
