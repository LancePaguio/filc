#ifndef DISASM_H
#define DISASM_H

#include "opcode.h"

#include <cstddef>
#include <iosfwd>
#include <string>

namespace filcompiler {

void disassembleChunk(const Chunk& chunk, const std::string&name,
    std::ostream& out);

void disassembleInstruction(const Chunk& chunk, std::size_t offset,
    std::ostream& out);

std::string chunkOps(const Chunk& chunk);

}
#endif // !DISASM_H
