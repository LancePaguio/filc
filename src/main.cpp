#include "common.h"
#include "lexer.h"
#include "diagnostic.h"
#include "parser.h"
#include "ast_printer.h"
#include "semantic.h"
#include "debug.h"
#include "compiler.h"
#include "disasm.h"
#include "wika.h"

// adding this to avoid errors in other systems
#if defined(__linux__)
#include <sys/resource.h>
#endif

#include <ios>
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <exception>

namespace {
void usage(const char* argv0)  {
    std::cerr << "Paggamit: " << argv0 << " [OPSYONS] [FILE]\n\nOpsyons:\n"
	<< "[--tokens]\n\n[--ast]\n\n[--bytecode]\n\n[--trace]\n\n"
	<< "[--treewalk]\n\n[--treewalk-map]\n\n"
	<< "[--time]\n\n[--help/tulong]";

}

const char* backendName(filcompiler::Backend b) {
    switch (b) {
	    case filcompiler::Backend::Bytecode: return "bytecode";
	    case filcompiler::Backend::TreeWalkSlots: return "treewalk";
	    case filcompiler::Backend::TreeWalkMap: return "treewalk-map";

    }
	return "?";
}

long peakRss(){
#if defined (__linux__)
	rusage u{};
	if(getrusage(RUSAGE_SELF, &u) == 0)
		return u.ru_maxrss;
#endif
	return -1;
}
}

bool frontEnd(const std::string& source, const std::string& path,
    std::vector<filcompiler::StmtPtr>& program) {
	using namespace filcompiler;
	const DiagnosticReporter reporter(source, path, std::cerr);
	try {
		Lexer lexer(source);
		Parser parser(lexer.scanTokens());
		program = parser.parseProgram();
		if (parser.hadError()) {
			reporter.reportAll(parser.diagnostics());
			return false;
		}

	} catch (const CompileError& e) {
		reporter.report({Stage::Lexer, e.what(), e.loc, e.len});
		return false;
	}
	SemanticAnalyzer semantic;
	semantic.analyze(program);
	if (semantic.hadError()) {
		reporter.reportAll(semantic.diagnostics());
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
    using namespace filcompiler;

    bool showTokens = false;
    bool showAst = false;
    bool showBytecode = false;
    bool showTrace = false;
    bool showTiming = false;
    bool showTimingCsv = false;
	Backend backend = Backend::Bytecode;

    std::string path;
    for (int i = 1; i < argc; ++i) {
        const std::string input = argv[i];
		if (input == "--tokens") showTokens = true;
		else if (input == "--ast") showAst = true;
		else if (input == "--bytecode") showBytecode = true;
		else if (input == "--trace") showTrace = true;
		else if (input == "--time") showTiming = true;
		else if (input == "--time-csv") showTimingCsv = true;
		else if (input == "--treewalk") backend = Backend::TreeWalkSlots;
		else if (input == "--treewalk-map") backend = Backend::TreeWalkMap;
		else if (input == "--help" || input == "--tulong") {
            usage(argv[0]);
            return 0;
		}
		//we add the usage message anyways if they run filc without any options or files
		else if (!input.empty() && input[0] == '-') {
            usage(argv[0]);
            return 2;
        }
		else path = input;
    }
    if (path.empty()) {
        usage(argv[0]);
        return 2;
    }

    std::string source;
    try{
        source = readFile(path);

    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 2;
    }

    if (showTokens) {
		const DiagnosticReporter reporter(source, path, std::cerr);
        try {
            Lexer lexer(source);
            dumpTokens(lexer.scanTokens(), std::cout);
        } catch (const CompileError& e) {
		    reporter.report({Stage::Lexer, e.what(), e.loc, e.len});
        }
        return 0;
    }
    if (showAst || showBytecode) {
        std::vector<StmtPtr> program;
        if (!frontEnd(source, path, program))
            return 1;
        if (showAst)
            std::cout << printProgram(program) << "\n";
        if (showBytecode) {
            BytecodeCompiler gen;
            disassembleChunk(gen.compile(program), path, std::cout);
        }
        return 0;
    }

    RunOpt opts;
    opts.filename = path;
    StageTime time;
	if (showTrace) opts.trace = &std::cerr;
    opts.time = &time;
	const double trueExecMs = time.execMs - time.readMs;
	opts.backend = backend;

    bool ok = false;
    try {
        ok = caR(source, opts);
    } catch(const std::exception& e) {
        std::cerr << "internal error: " << e.what() << "\n";
        return 3;
    }

    if (showTiming) {
        std::cerr << std:: fixed << std::setprecision(3)
            << "backend: " << backendName(backend) << "\n"
            << "lexer: " << time.lexerMs << "ms\n"
            << "parser: " << time.parserMs << "ms\n"
            << "semantic: " << time.semanticMs << "ms\n";
            if (backend == Backend::Bytecode)
                std::cerr <<  "compiler: " << time.compilerMs << "ms\n";
			std::cerr << "exec: " << time.execMs << "ms\n";
			std::cerr << "read: " << time.readMs << "ms\n";
			std::cerr << "trueexec: " << trueExecMs << "ms\n";
			if (backend == Backend::Bytecode)
				std::cerr << "instructions: " << time.instructions << "\n";
			else
				std::cerr << "nodes: " << time.nodes << "\n";
			std::cerr << "process peak rss: " << peakRss() << "kilobytes\n";
    }
    if (showTimingCsv) {
        std::cerr << std:: fixed << std::setprecision(3)
            << path << ',' << backendName(backend) << ","
            << time.lexerMs << time.lexerMs << "," << time.parserMs << ','
            << time.semanticMs << ".";
        if (backend == Backend::Bytecode)
            std::cerr <<  time.compilerMs;
        std::cerr <<  ',' << trueExecMs <<  ','  << time.readMs << ',';
        if (backend == Backend::Bytecode)
            std::cerr << time.instructions;
        if (backend == Backend::Bytecode)
            std::cerr << time.nodes;
        std::cerr << ',' << peakRss() << '\n';
	}

    return ok ? 0 : 1;

}
