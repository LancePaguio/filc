#include "opcode.h"
#include <cstddef>
#include <string>
#include <vector>

namespace filcompiler{

const char* opCodeName(OpCode op) {
    switch (op) {
	    case OpCode::OP_ADD: return "ADD";
	    case OpCode::OP_SUB: return "SUB";
	    case OpCode::OP_MUL: return "MUL";
	    case OpCode::OP_DIV: return "DIV";
	    case OpCode::OP_MOD: return "MOD";
	    case OpCode::OP_NEG: return "NEG";
	    case OpCode::OP_GT: return "GT";
		case OpCode::OP_GE: return "GE";
		case OpCode::OP_LT: return "LT";
		case OpCode::OP_LE: return "LE";
		case OpCode::OP_EQ: return "EQ";
		case OpCode::OP_NEQ: return "NEQ";
		case OpCode::OP_NOT: return "NOT";
		case OpCode::OP_TRUE: return "TRUE";
		case OpCode::OP_FALSE: return "FALSE";
		case OpCode::OP_DEFINE_GLOBAL: return "DEFINE_GLOBAL";
		case OpCode::OP_GET_GLOBAL: return "GET_GLOBAL";
		case OpCode::OP_SET_GLOBAL: return "SET_GLOBAL";
		case OpCode::OP_PRINT_VAL: return "PRINT_VAL";
		case OpCode::OP_PRINT_STR: return "PRINT_STR";
		case OpCode::OP_READ: return "READ";
		case OpCode::OP_CONST_INT: return "CONST_INT";
		case OpCode::OP_POP: return "POP";
		case OpCode::OP_DUP: return "DUP";
		case OpCode::OP_JUMP: return "JUMP";
		case OpCode::OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
		case OpCode::OP_JUMP_IF_TRUE: return "JUMP_IF_TRUE";
		case OpCode::OP_HALT: return "HALT";
	}
	return "?";

}

// 5.2
// these are the constant pool
// and global stack
// it holds the integers in the intconstat
// strings to stringconstant pool
// defined variables to to globalslots
int Chunk::addIntConstant(int v) {
    const auto it = intConstIndex.find(v);
	if (it != intConstIndex.end())
	    return it->second;
	intConstants.push_back(v);
	const int idx = static_cast<int>(intConstants.size()) - 1;
	intConstIndex.emplace(v, idx);
	return idx;
}

int Chunk::addStringConstant(const std::string& s) {
    const auto it = strConstIndex.find(s);
	if (it != strConstIndex.end())
	    return it->second;
	stringConstants.push_back(s);
	const int idx = static_cast<int>(stringConstants.size()) - 1;
	strConstIndex.emplace(s, idx);
	return idx;
}

int Chunk::globalSlot(const std::string& name) {
    const auto it = globalIndex.find(name);
	if (it != globalIndex.end())
	    return it->second;
	globalNames.push_back(name);
	const int idx = static_cast<int>(globalNames.size()) - 1;
	globalIndex.emplace(name, idx);
	return idx;
}

int Chunk::emit(OpCode op, int operand, int line, int column) {
    code.push_back({op, operand, line, column});
	return static_cast<int>(code.size()) - 1;
}

void Chunk::patchJump(int instrIndex, int target) {
    code [static_cast<std::size_t>(instrIndex)].operand = target;
}

// this checks the instructions, the jump, the operand if
// they are in range, and check for any unpatched jumps
bool Chunk::verify(std::string& whyNot)  const {
    if (code.empty() || code.back().op != OpCode::OP_HALT) {
	    whyNot = "ang chunk ay dapat nagtatapos sa OP_HALT";
		return false;
	}
	const auto inRange = [] (int idx, std::size_t size) {
        return idx >= 0 && static_cast<std::size_t>(idx) < size;
    };
    const auto n = code.size();
	for (std::size_t i = 0; i < n; ++i)  {
	    const Instr& in = code[i];
        const std::string at = std::to_string(i);
		switch (in.op) {
		    case OpCode::OP_JUMP:
		    case OpCode::OP_JUMP_IF_FALSE:
		    case OpCode::OP_JUMP_IF_TRUE:
			    if (!inRange(in.operand, n)) {
				    whyNot = "nawala sa saklaw ng jump target ang instruction" +
					    at;
					return false;
				}
				break;
			case OpCode::OP_CONST_INT:
			    if (!inRange(in.operand, intConstants.size())) {
				    whyNot = "mali ang index ng int constant sa " + at;
					return false;
				}
			break;
			case OpCode::OP_PRINT_STR:
			    if (!inRange(in.operand, stringConstants.size())) {
				    whyNot = "mali ang index ng string constant sa " + at;
					return false;
				}
			break;
			case OpCode::OP_DEFINE_GLOBAL:
			case OpCode::OP_GET_GLOBAL:
			case OpCode::OP_SET_GLOBAL:
			    if (!inRange(in.operand, globalNames.size())) {
				    whyNot = "hindi tama ang global slot sa "
					+ at;
					return false;
				}
			break;
		default:
		    break;
		}
	}
	return verifyStack(whyNot);
}

bool Chunk::verifyStack(std::string& whyNot) const {
    const int n = static_cast<int>(code.size());
	std::vector<int> depth(static_cast<std::size_t>(n), -1);
	std::vector<int> work{0};
	depth[0] = 0;

	const auto visit = [&] (int target, int d) {
	    if (target < 0 || target >= n)
		    return true;
		int& slot = depth[static_cast<std::size_t>(target)];
		if (slot == -1) {
		    slot = d; work.push_back(target);
		    return true;
		}
		return slot == d;
	};

	while (!work.empty()) {
	    const int i = work.back();
		work.pop_back();
		const Instr& in = code[static_cast<std::size_t>(i)];
		int d = depth[static_cast<std::size_t>(i)];

        int pops = 0;
		int pushes = 0;
		switch (in.op) {
			case OpCode::OP_ADD:
			case OpCode::OP_SUB:
			case OpCode::OP_MUL:
			case OpCode::OP_DIV:
			case OpCode::OP_MOD:
			case OpCode::OP_GT:
			case OpCode::OP_GE:
			case OpCode::OP_LT:
			case OpCode::OP_LE:
			case OpCode::OP_EQ:
			case OpCode::OP_NEQ:
                pops = 2;
                pushes = 1;
                break;
			case OpCode::OP_NEG:
			case OpCode::OP_NOT:
                pops = 1;
                pushes = 1;
                break;
			case OpCode::OP_TRUE:
			case OpCode::OP_FALSE:
			case OpCode::OP_GET_GLOBAL:
			case OpCode::OP_CONST_INT:
			case OpCode::OP_READ:
                pushes = 1;
                break;
			case OpCode::OP_DEFINE_GLOBAL:
			case OpCode::OP_SET_GLOBAL:
			case OpCode::OP_PRINT_VAL:
			case OpCode::OP_POP:
                pops = 1;
				break;
			case OpCode::OP_DUP:
                pops = 1;
                pushes = 2;
				break;
			case OpCode::OP_JUMP_IF_FALSE:
			case OpCode::OP_JUMP_IF_TRUE:
                pops = 1;
                pushes = 1;
				break;
			case OpCode::OP_PRINT_STR:
			case OpCode::OP_JUMP:
			case OpCode::OP_HALT:
			    break;
		}

		if (d < pops)  {
			 whyNot = "kulang ang operand stack sa instruction " +
			     std::to_string(i);
			 return false;
		}

		d = d - pops + pushes;
		if (in.op == OpCode::OP_HALT)  {
		    if (d != 0)  {
			    whyNot =  "may natitira pa sa operand stack sa OP_HALT " +
				    std::to_string(d) + ")";
			    return false;
			}
		    continue;
		}
		if (in.op == OpCode::OP_JUMP) {
		    if(!visit(in.operand,d)) {
			    whyNot = "magkakaiba ang stack depth sa jump target " +
				    std::to_string(in.operand);
				return false;
			}
			continue;
		}
		if (in.op == OpCode::OP_JUMP_IF_FALSE || in.op ==
		    OpCode::OP_JUMP_IF_TRUE)  {

		    if(!visit(in.operand,d)) {
			    whyNot = "magkakaiba ang stack depth sa branch target " +
					std::to_string(in.operand);
				return false;
			}
		}
		if (!visit(i + 1, d) ) {
		    whyNot = "hindi magkatugma ang stack depth sa instruction " +
			    std::to_string(i + 1);
			return false;
		}
	}
	return true;
}

}
