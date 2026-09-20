#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "common.h"
#include "ast.h"

#include <string>
#include <vector>

namespace filcompiler {
std::string printStmt(const Stmt& s);

std::string printProgram(const std::vector<StmtPtr>& program);

}

#endif // !AST_PRINTER_H
