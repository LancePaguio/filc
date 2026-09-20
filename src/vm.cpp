#include "vm.h"
#include "disasm.h"
#include "opcode.h"
#include "value.h"

#include <cstddef>
#include <limits>
#include <iostream>
#include <cstdint>
#include <ratio>
#include <vector>
#include <chrono>

namespace filcompiler {
namespace  {

// aritmetic is in 64 bits to avoid  signed overflow
constexpr std::int64_t kIntMin = std::numeric_limits<int>::min();
constexpr std::int64_t kIntMax = std::numeric_limits<int>::max();
bool fit(std::int64_t v) {
    return v >= kIntMin && v  <= kIntMax;
}

}

VM::VM() : in(std::cin), out(std::cout) {}
VM::VM(std::istream& input, std::ostream& output)
    : in(input), out(output) {}

//6.2 main execution loop. must verify first, before reading
//the instructions and following
bool VM::run(const Chunk& chunk) {
    diagnosticsVar.clear();
	stack.clear();
    retired = 0;
	readMs = 0.0;
    {
	std::string whyNot;
	if (!chunk.verify(whyNot)){
	    diagnosticsVar.push_back({Stage::Internal,
            "ito ay sirang bytecode: " + whyNot, {}, 1});
	return false;
	}
	}
	std::vector<Value> globals(chunk.globalNames.size(), Value::makeInt(0));
	std::vector<bool> defined(chunk.globalNames.size(), false);
	std::size_t ip = 0;

	const auto requireNumbers = [&](const Value& l, const Value& r,
        const Instr& at) {
	    if (l.type == Value::Type::VT_INT && r.type == Value::Type::VT_INT)
		return true;
			fail("Ang pwede ilagay sa operand ay mga numero lamang", at);
		return false;
	};

	const auto requireNumber = [&](const Value& v, const Instr& at) {
	    if (v.type == Value::Type::VT_INT)
		return true;
			fail("Ang pwede ilagay sa operand ay mga numero lamang", at);
		return false;
	};

	const auto needs = [&](std::size_t n, const Instr& at) {
	    if (stack.size() >= n)
		return true;
			fail("Kulang ang lamang ng operand stack", at);
		return false;

	};

	long long steps = 0;
	for (;;) {
	    if (ip >= chunk.code.size()){
		    diagnosticsVar.push_back({Stage::Internal,
				"Lumabas ang instruction pointer sa byte code",
				{},1});
            return false;
		}

        ++retired;
		if (stepLimit > 0 && ++steps > stepLimit) {
		    diagnosticsVar.push_back({Stage::Internal,
				"Lumagpas ang bilang ng instruction",
				{chunk.code[ip].line, chunk.code[ip].column},1});
            return false;

		}

		const Instr& instr = chunk.code[ip];
		if (trace) {
		    *trace << "        ";
			for (const Value& slot : stack) {
			    *trace << "[";
				printValue(*trace, slot);
				*trace << "]";
			}
		    *trace << '\n';
			disassembleInstruction(chunk, ip, *trace);
		}

        switch (instr.op)  {
            case OpCode::OP_JUMP:
                ip = static_cast<std::size_t>(instr.operand);
                continue;
            case OpCode::OP_JUMP_IF_FALSE:
                if (!needs(1, instr))
                    return false;

                if (!stack.back().truth()){
                    ip = static_cast<std::size_t>(instr.operand);
                    continue;
                }
                break;
            case OpCode::OP_JUMP_IF_TRUE:
                if (!needs(1, instr))
                    return false;

                //when true it must jump
                if (stack.back().truth()){
                    ip = static_cast<std::size_t>(instr.operand);
                    continue;
                }
                break;
            case OpCode::OP_DEFINE_GLOBAL: {
                if (!needs(1, instr))
                    return false;
                const auto slot = static_cast<std::size_t>(instr.operand);
                globals[slot] = pop();
				defined[slot] = true;
                break;
			}
            case OpCode::OP_GET_GLOBAL:{
                const auto slot = static_cast<std::size_t>(instr.operand);
                if (!defined[slot])
                    return fail("Hindi pa naitakda ang variable '" +
                    chunk.globalNames[slot] + "'.", instr);
                push(globals[slot]);
                break;
            }
            case OpCode::OP_SET_GLOBAL:{
                if (!needs(1, instr))
                    return false;
                const auto slot = static_cast<std::size_t>(instr.operand);
                globals[slot] = pop();
                defined[slot] = true;
                break;
            }
            case OpCode::OP_CONST_INT:
                push(Value::makeInt(chunk.intConstants[static_cast
                    <std::size_t> (instr.operand)]));
                break;
            case OpCode::OP_PRINT_STR:
                out << chunk.stringConstants[static_cast<std::size_t>
                    (instr.operand)] << "\n";
                break;
            case OpCode::OP_PRINT_VAL: {
                if (!needs(1, instr))
                    return false;

                printValue(out, pop());
				out << '\n';
                break;
            }
            case OpCode::OP_READ:{
                int x = 0;
                if (prompt)
                    //making the prompt appear before the program waits
                    *prompt << "input: " <<std::flush;
				const auto r0 = std::chrono::steady_clock::now();
				const bool got = static_cast<bool>(in >> x);
				readMs += std::chrono::duration<double, std::milli>
				    (std::chrono::steady_clock::now() - r0).count();
                if (!got)
                    return fail("Hindi tama ang nilagay na integer.", instr);
                push(Value::makeInt(x));
                break;
            }
            case OpCode::OP_TRUE:
                push(Value::makeBool(true));
                break;
            case OpCode::OP_FALSE:
                push(Value::makeBool(false));
                break;
            case OpCode::OP_ADD:
            case OpCode::OP_SUB:
            case OpCode::OP_MUL: {
                if (!needs(2, instr))
                    return false;

                const Value r = pop();
                const Value l = pop();
                if (!requireNumbers(l, r, instr))
                    return false;

                const std::int64_t a = l.i;
                const std::int64_t b = r.i;
                std::int64_t result = 0;
                const char* definition = "";
                if (instr.op == OpCode::OP_ADD){
                    result = a + b;
                    definition = "pagdaragdag";
                }
                if (instr.op == OpCode::OP_SUB){
                    result = a - b;
                    definition = "pagbabawas";
                }
                if (instr.op == OpCode::OP_MUL){
                    result = a * b;
                    definition = "pagpaparami";
                }

                if (!fit(result))
                    return fail(std::string("Lumagpas sa saklaw ng"
                                            " 32 bit integer ang ") + definition, instr);

                push(Value::makeInt(static_cast<int>(result)));
                break;
            }
            case OpCode::OP_DIV: {
                if (!needs(2, instr))
                    return false;

                const Value r = pop();
                const Value l = pop();
                if (!requireNumbers(l, r, instr))
                    return false;

                if (r.i == 0)
                    return fail("Hindi ito pwede hatiin sa zero.", instr);

                if (l.i == std::numeric_limits<int>::min()
                    && r.i == -1)
                    return fail("Lumagpas sa saklaw ng"
                                " 32 bit integer ang ", instr);
                push(Value::makeInt(l.i / r.i));
                break;

            }
            case OpCode::OP_MOD: {
                if (!needs(2, instr))
                    return false;

                const Value r = pop();
                const Value l = pop();
                if (!requireNumbers(l, r, instr))
                    return false;

                if (r.i == 0)
                    return fail("hindi ito pwede hatiin sa zero.", instr);

                if (l.i == std::numeric_limits<int>::min()
                    && r.i == -1) {
                    push(Value::makeInt(0));
                    break;
                }
                push(Value::makeInt(l.i % r.i));
                break;
            }
            case OpCode::OP_EQ: {
                if (!needs(2, instr))
                    return false;
                const Value r = pop();
                const Value l = pop();
                push(Value::makeBool(rawEq(l,r)));
                break;
            }
            case OpCode::OP_NEQ: {
                if (!needs(2, instr))
                    return false;
                const Value r = pop();
                const Value l = pop();
                push(Value::makeBool(!rawEq(l,r)));
                break;
            }
            case OpCode::OP_GT:
            case OpCode::OP_GE:
            case OpCode::OP_LT:
            case OpCode::OP_LE: {
                if (!needs(2, instr))
                    return false;

                const Value r = pop();
                const Value l = pop();
                if(!requireNumbers(l, r, instr))
                    return false;

                bool result = false;
                switch (instr.op) {
                    case OpCode::OP_GT:
                        result = l.i > r.i;
                        break;
                    case OpCode::OP_LT:
                        result = l.i < r.i;
                        break;
                    case OpCode::OP_GE:
                        result = l.i >= r.i;
                        break;
                    default:
                        result = l.i <= r.i;
                        break;
                }
				push(Value::makeBool(result));
				break;
            }
            case OpCode::OP_NEG: {
                if (!needs(1, instr))
                    return false;

                const Value v = pop();
                if (!requireNumber(v, instr))
                    return false;
                if (v.i == std::numeric_limits<int>::min())
                    return fail("Lumagpas sa saklaw ng 32 bit integer ang ",
                        instr);
                push(Value::makeInt(-v.i));
                break;
            }
            case OpCode::OP_NOT: {
                if (!needs(1, instr))
                    return false;
                push(Value::makeBool(!pop().truth()));
                break;
            }
            case OpCode::OP_POP:
                if (!needs(1, instr))
                    return false;
                stack.pop_back();
                break;
            case OpCode::OP_DUP:
                if(!needs(1, instr))
                    return false;
                push(stack.back());
                break;
            case OpCode::OP_HALT:
                return true;
        }
        ++ip;
    }
}
// error handling for 6.4
// shows the message with error location, which instruction it came from
// then stops
bool VM::fail(std::string msg, const Instr& at) {
    diagnosticsVar.push_back({Stage::Runtime, std::move(msg),
        {at.line, at.column}, 1});
    return false;
}


}
