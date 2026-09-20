#include "common.h"
#include "diagnostic.h"
#include <cstddef>
#include <ostream>
#include <string>

namespace filcompiler {
namespace  {

std::string expandTabs(const std::string& s, int width = 4) {
    std::string out;
	for (const char c: s) {
	    if (c=='\t') out.append(static_cast<std::size_t>(width), ' ');
	    else
		out += c;
	}
	return out;
}

int visualColumn(const std::string& line, int column, int width = 4) {
    int visual = 0;
	const int limit = column - 1;
	for (int i = 0; i < limit && i < static_cast<int>(line.size()); ++i) {
	    visual += (line[static_cast<std::size_t>(i)] == '\t') ? width : 1;
	}
	return visual;
}
}

DiagnosticReporter::DiagnosticReporter(const std::string& source,
    std::string filename, std::ostream& out) : sourceVar(source),
	filenameVar(std::move(filename)), outVar(out) {}

const char* DiagnosticReporter::stageName(Stage s) {
    switch(s){
	    case Stage::Lexer: return "lexer";
	    case Stage::Syntax: return "syntax";
	    case Stage::Semantic: return "semantic";
	    case Stage::Runtime: return "runtime";
	    case Stage::Internal: return "internal";
	}
	return "?";
}

std::string DiagnosticReporter::sourceLine(int n) const {
    if (n < 1)
	    return {};
    int current = 1;
	std::size_t start = 0;
	while (current < n) {
	    const std::size_t nl = sourceVar.find('\n', start);
		if (nl == std::string::npos)
		    return {};
		start = nl + 1;
		++current;
	}
	const std::size_t nl = sourceVar.find('\n', start);
	std::string line = (nl == std::string::npos) ? sourceVar.substr(start)
	    : sourceVar.substr(start, nl - start);
	if (!line.empty() && line.back() == '\r') line.pop_back();
		return line;
}

void DiagnosticReporter::report(const Diagnostic& d) const {
    outVar << filenameVar << ':' << d.loc.line << ':' << d.loc.column << ": " <<
	    stageName(d.stage) << " error: " << d.message << '\n';

	if (d.loc.line < 1)
	    return;
	const std::string raw = sourceLine(d.loc.line);
	if (raw.empty())
	    return;
	const std::string num = std::to_string(d.loc.line);
	const std::string gutter(num.size(), ' ');
	outVar << ' ' << num << " | " << expandTabs(raw) << '\n';

	const int visual = visualColumn(raw, d.loc.column);
	outVar << ' ' << gutter << " | " <<
	    std::string(static_cast<std::size_t>(visual), ' ') << '^';
	for (int i = 1; i < d.len; ++i) outVar << '~';
	outVar << '\n';
}

void DiagnosticReporter::reportAll(const std::vector<Diagnostic>& ds) const {
    for (const Diagnostic& d : ds) report(d);
}

}
