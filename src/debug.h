#ifndef DEBUG_H
#define DEBUG_H

#include <iosfwd>
#include <string>
#include <vector>
#include "common.h"

namespace filcompiler{
const char* tokenTypeName(TokenType t);
void dumpTokens(const std::vector<Token>& tokens, std::ostream& out);
}

#endif // !DEBUG_H
