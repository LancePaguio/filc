#include "wika.h"
#include "lexer.h"
#include "parser.h"
#include "diagnostic.h"
#include "semantic.h"

//bytecode
#include "vm.h"
#include "compiler.h"

//tree-walk
#include "treewalk.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace filcompiler {
namespace  {

using Clock = std::chrono::steady_clock;
double milSec(Clock::time_point a, Clock::time_point b)  {
    return std::chrono::duration<double, std::milli> (b-a).count();
}
}

std::string readFile(const std::string& path) {
    std::ifstream f(path);
	if (!f) throw std::runtime_error("Ayaw mabukasan yung file: " + path);
	std::ostringstream oss;
	oss << f.rdbuf();
	return oss.str();
}

bool caR(const std::string& source, const RunOpt& opts, std::ostream& err) {
    StageTime local;
	StageTime& t = opts.time ? *opts.time : local;

	const DiagnosticReporter reporter(source, opts.filename, err);
	const auto t0 = Clock::now();
	std::vector<Token> tokens;
	try {
	    Lexer lexer(source);
		tokens = lexer.scanTokens();
	} catch (const CompileError& e) {
	    reporter.report({Stage::Lexer, e.what(), e.loc, e.len});
		return false;
	}
	const auto t1 = Clock::now();
	t.lexerMs = milSec(t0, t1);

	Parser parser(std::move(tokens));
	std::vector<StmtPtr> program = parser.parseProgram();
	const auto t2 = Clock::now();
	t.parserMs = milSec(t1, t2);
	if (parser.hadError()){
	    reporter.reportAll(parser.diagnostics());
		err << "tumigil ang compiler dahil mayroon syntax error.\n";
    return false;
	}

	SemanticAnalyzer semantic;
	semantic.analyze(program);
	const auto t3 = Clock::now();
	t.semanticMs = milSec(t2, t3);
	if (semantic.hadError()) {
	    reporter.reportAll(semantic.diagnostics());
		err << "tumigil ang compiler dahil mayroon semantic error.\n";
		return false;
	}

	if (opts.backend != Backend::Bytecode) {
	    TreeWalker walker(opts.in ? *opts.in : std::cin, opts.out ?
		    *opts.out : std::cout, opts.backend == Backend::TreeWalkSlots
			? TreeWalker::Storage::Slots : TreeWalker::Storage::Map);
        walker.setPrompt(&std::cerr);
		const auto tw0 = Clock::now();
		const bool twOk = walker.run(program);
		t.execMs = milSec(tw0, Clock::now());
		t.readMs = walker.readWaitMs();
		t.nodes = walker.nodesVisited();
        t.compilerMs = 0.0;
		if(!twOk){
            reporter.reportAll(walker.diagnostics());
        }
        return twOk;
	}

	BytecodeCompiler gen;
	const Chunk chunk = gen.compile(program);

	const auto t4 = Clock::now();
	t.compilerMs = milSec(t3, t4);

	VM vm(opts.in ? *opts.in : std::cin, opts.out ? *opts.out : std::cout);
	vm.setStepLimit(opts.maxSteps);
	vm.setPrompt(&std::cerr);
	vm.setTrace(opts.trace);
	const bool ok = vm.run(chunk);
	t.execMs = milSec(t4, Clock::now());
	t.readMs = vm.readWaitMs();
	t.instructions = vm.instructionsRetired();
	if (!ok)
        reporter.reportAll(vm.diagnostics());
    return ok;
}

bool caR(const std::string& source, const RunOpt& opts) {
    return caR(source, opts, std::cerr);
}


}
