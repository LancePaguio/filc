#ifndef VALUE_H
#define VALUE_H

#include <charconv>
#include <ostream>
namespace filcompiler {

// set up the operands that will go to the operand stack
// for 6.1
struct Value {
    enum class Type { VT_INT, VT_BOOL };

	Type type = Type::VT_INT;
	int i = 0;
	bool b = false;

	static Value makeInt(int v) {
	    Value x; x.type = Type::VT_INT; x.i = v;
		return x;

	}
	static Value makeBool(bool v) {
	    Value x; x.type = Type::VT_BOOL; x.b = v;
		return x;

	}

	bool truth() const {
	    return type == Type::VT_BOOL ? b : i != 0;
	}
};

inline void printValue(std::ostream& out, const Value& v) {
    if (v.type == Value::Type::VT_BOOL)
	    out << (v.b ? "totoo" : "mali");
	else
	    out << v.i;
}


inline bool rawEq(const Value& l, const Value& r) {
    if (l.type != r.type)
	    return false;
	return l.type == Value::Type::VT_INT ?
	    l.i == r.i : l.b == r.b;
}

}

#endif // !VALUE_H
