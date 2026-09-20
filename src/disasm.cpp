#include "disasm.h"
#include "opcode.h"

#include <cstddef>
#include <string>
#include <ostream>

namespace filcompiler {
namespace  {
std::string padLeft(std::string s, std::size_t width, char fill = ' ')  {
    if (s.size() < width) s.insert(0, width -  s.size(), fill);
	return s;
}
std::string padRight(std::string s, std::size_t width)  {
    if (s.size() < width) s.append(width - s.size(), ' ');
	return s;
}

std::string opName(OpCode op) {
    return padRight(std::string("OP_") + opCodeName(op), 16);
}

void simpleInstruction(OpCode op, std::ostream& out) {
    out << "OP_" << opCodeName(op) << '\n';
}

void operandInstruction(OpCode op, int operand,
    const std::string& what, std::ostream& out) {
    out << opName(op) << ' ' << padLeft(std::to_string(operand), 4)
	    << " '" << what << "'\n";
}

void jumpInstruction(OpCode op, std::size_t offset, int target,
    std::ostream& out) {
    out << opName(op) << ' ' << padLeft(std::to_string(offset), 4)
	<< " -> " << target << "'\n";
}

}

void disassembleChunk(const Chunk& chunk, const std::string &name,
    std::ostream &out) {
    out << "== " << name << " ==\n";
	for (std::size_t offset = 0; offset < chunk.code.size(); ++offset)
	    disassembleInstruction(chunk, offset, out);
}

void disassembleInstruction(const Chunk& chunk, const std::size_t offset,
    std::ostream &out) {
	const Instr& in = chunk.code[offset];

	out << padLeft(std::to_string(offset), 4, '0') << ' ';
	if (offset > 0 && in.line == chunk.code[offset - 1].line)
	    out << "   | ";
	else
	    out << padLeft(std::to_string(in.line), 4) << ' ';

    const auto index = static_cast<std::size_t>(in.operand);
	switch (in.op) {
		case OpCode::OP_CONST_INT:
			operandInstruction(in.op, in.operand,
			    std::to_string(chunk.intConstants[index]), out);
			break;
		case OpCode::OP_PRINT_STR:
			operandInstruction(in.op, in.operand,
			    chunk.stringConstants[index], out);
			break;
		case OpCode::OP_DEFINE_GLOBAL:
		case OpCode::OP_GET_GLOBAL:
		case OpCode::OP_SET_GLOBAL:
			operandInstruction(in.op, in.operand, chunk.globalNames[index], out);
			break;
		case OpCode::OP_JUMP:
		case OpCode::OP_JUMP_IF_FALSE:
		case OpCode::OP_JUMP_IF_TRUE:
            jumpInstruction(in.op, offset, in.operand,out);
			break;
		default:
			simpleInstruction(in.op, out);
			break;
	}
}

std::string chunkOps(const Chunk& chunk)  {
    std::string out;
	for (const Instr& in : chunk.code) {
	    if (!out.empty()) out += " ";
		out += opCodeName(in.op);
	}
    return out;
}

}

