#ifndef WIKA_H
#define WIKA_H

#include "common.h"

#include <iosfwd>

namespace filcompiler {
std:: string readFile(const std::string& path);

struct StageTime {
    double lexerMs = 0.0;
    double parserMs = 0.0;
    double semanticMs = 0.0;
    double compilerMs = 0.0;
    double execMs = 0.0;  // this is the vm
    double readMs = 0.0; // when basahin() is used
	long long instructions = 0; // for bytecode only
	long long nodes = 0; // treewalk only
};

enum class Backend {Bytecode, TreeWalkSlots, TreeWalkMap};

struct RunOpt {
	Backend backend = Backend::Bytecode;
    std::string filename;
	StageTime* time = nullptr;
    std::istream* in = nullptr;
    std::ostream* out = nullptr;
    long long maxSteps = 0;
	std::ostream* trace = nullptr;

};

bool caR(const std::string& source, const RunOpt& opts,
    std::ostream& err);
bool caR(const std::string& source, const RunOpt& opts = {});

}

#endif // !WIKA_H
