#ifndef OPCODE_H
#define OPCODE_H

#include "common.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace filcompiler {

//instruction set
// for arithmetic, comparison, global access, control flow,
// and input/output
// for 5.1
enum class OpCode {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG, OP_GT,
    OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ, OP_NOT, OP_TRUE,
    OP_FALSE, OP_DEFINE_GLOBAL, OP_GET_GLOBAL, OP_SET_GLOBAL,
	OP_PRINT_VAL, OP_PRINT_STR, OP_READ, OP_CONST_INT, OP_POP,
    OP_DUP, OP_JUMP, OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE, OP_HALT,
};

// decoded instruction
struct Instr {
    OpCode op{};
    int operand{};
    int line{};
    int column{};
};

const char* opCodeName(OpCode op);

// container for the chunk
// for 5.2 and virtual machine
struct Chunk {
    std::vector<Instr> code;
    std::vector<int> intConstants;
    std::vector<std::string> stringConstants;
    std::vector<std::string> globalNames;

    int addStringConstant(const std::string& s);
    int addIntConstant(int v);
    int globalSlot(const std::string& name);
    int emit(OpCode op, int operand, int line, int column = 0);
    void patchJump(int instrIndex, int target);

    bool verify(std::string& whyNot) const;
    bool verifyStack(std::string& whyNot) const;

private:
    std:: unordered_map<std::string, int> globalIndex;
    std:: unordered_map<std::string, int> strConstIndex;
    std:: unordered_map<int, int> intConstIndex;

};

}
#endif // !OPCODE_H
