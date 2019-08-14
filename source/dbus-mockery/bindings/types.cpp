#include <dbus-mockery/bindings/types.hpp>

using namespace std::string_literals;

#define CASE_CONV(ch, name) \
	case(ch): \
    { \
	    return name;\
	}

namespace DBusMock
{
    std::string typeToComprehensible(char type)
	{
		switch(type)
		{
		    CASE_CONV('y', "unsigned char")
			CASE_CONV('b', "boolean")
			CASE_CONV('n', "short signed integer")
			CASE_CONV('q', "short unsigned integer")
			CASE_CONV('i', "signed integer")
			CASE_CONV('u', "unsigned integer")
			CASE_CONV('x', "signed long integer")
			CASE_CONV('t', "unsigned long integer")
			CASE_CONV('d', "double precision floating point")
			CASE_CONV('s', "string")
			CASE_CONV('o', "object path")
			CASE_CONV('g', "signature")
			CASE_CONV('h', "file descriptor")
			CASE_CONV('a', "array")
			CASE_CONV('v', "variant")
		}
		return "unknown:"s + type;
	}

	std::string typeToCpp(char type)
	{
		switch(type)
		{
		    CASE_CONV('y', "uint8_t")
			CASE_CONV('b', "bool")
			CASE_CONV('n', "int16_t")
			CASE_CONV('q', "uint16_t")
			CASE_CONV('i', "int32_t")
			CASE_CONV('u', "uint32_t")
			CASE_CONV('x', "int64_t")
			CASE_CONV('t', "uint64_t")
			CASE_CONV('d', "double")
			CASE_CONV('s', "std::string")
			CASE_CONV('o', "std::string")
			CASE_CONV('g', "std::string")
			CASE_CONV('h', "int")
			CASE_CONV('a', "[array]")
			CASE_CONV('v', "{variant}")
		}
		return "unknown:"s + type;
	}
}

#undef CASE_CONV
