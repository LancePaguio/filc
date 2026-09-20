#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H
#include "common.h"

#include <iosfwd>
#include <string>
#include <vector>
namespace filcompiler{

class DiagnosticReporter{
public:
    DiagnosticReporter(const std::string& source, std::string filename,
	    std::ostream& out);

	void report(const Diagnostic& d) const;
	void reportAll(const std::vector<Diagnostic>& ds) const;
	static const char* stageName(Stage s);

private:
    const std::string& sourceVar;
	std::string filenameVar;
	std::ostream& outVar;
	std::string sourceLine(int n) const;
};
}

#endif // !DIAGNOSTIC_H
