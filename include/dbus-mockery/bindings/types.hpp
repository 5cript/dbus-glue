#pragma once

#include "sdbus_core.hpp"
#include "object_path.hpp"
#include "signature.hpp"
#include "file_descriptor.hpp"
#include "struct_adapter.hpp"
#include "msg_fwd.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <any>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>

// https://dbus.freedesktop.org/doc/dbus-specification.html#type-system

/*
	types ::= complete_type*
	complete_type ::= basic_type | variant | structure | array | dictionary
	basic_type ::= "y" | "n" | "q" | "u" | "i" | "x" | "t" | "d" |
				   "b" | "h" |
				   "s" | "o" | "g"
	variant ::= "v"
	structure ::= "(" complete_type+ ")"
	array ::= "a" complete_type
	dictionary ::= "a" "{" basic_type complete_type "}"
*/

namespace DBusMock
{
    /**
	 *	Converts a C++ type int a dbus char.
	 */
    template <typename T, typename SFINAE = void>
    struct type_detect{};

	template <typename T, typename SFINAE = void>
	struct type_converter
	{
	};

	struct type_descriptor
	{
		char type{'\0'};
		std::string_view contained{};

		std::string string() const
		{
			std::string s;
			bool noContain = false;
			if (contained.size() == 0 || (contained.size() == 1 && contained.front() == '\0'))
			{
				noContain = true;
			}
			if (type > 32)
			{
				s.resize(1);
				s[0] = type;
			}
			else
			{
				auto tval = static_cast <int> (type);
				if (tval < 0)
					tval += 255;
				std::stringstream sstr;
				sstr << "\\x" << std::hex << std::setw(2) << std::setfill('0') << tval;
				s = sstr.str();
			}
			if (noContain)
				s += "\\x00";
			else
				s += contained.data();
			return s;
		}
	};

	constexpr bool is_possible_key(char c)
	{
		return c == 'y' || c == 'n' || c == 'q' || c == 'i' || c == 'u' || c == 'x' || c == 't'	||
		       c == 's' || c == 'o';
	}

	template <typename FunctionT>
	bool for_signature_do_noexcept (type_descriptor descr, FunctionT func)
	{
		using namespace	std::string_literals;

		switch (descr.type)
		{
		    default: return false;
		    case('y'): func(uint8_t{}); break;
		    case('b'): func(bool{}); break;
		    case('n'): func(int16_t{}); break;
		    case('q'): func(uint16_t{}); break;
		    case('i'): func(int32_t{}); break;
		    case('u'): func(uint32_t{}); break;
		    case('x'): func(int64_t{}); break;
		    case('t'): func(uint64_t{}); break;
		    case('d'): func(double{}); break;
		    case('s'): func(std::string{}); break;
		    case('o'): func(object_path{}); break;
		    case('h'): func(file_descriptor{}); break;
		    case('g'): func(signature{}); break;
		}
		return true;
	}

	template <typename FunctionT>
	void for_signature_do (type_descriptor descr, FunctionT func)
	{
		using namespace	std::string_literals;

		bool suc = for_signature_do_noexcept(descr, [](auto){});
		if (suc)
			for_signature_do_noexcept(descr, func);
		else
		{
			throw std::domain_error(
			    "resolvable_variants cannot contain arrays or other complex types"s + descr.string()
			);
		}
	}

	// can only be used for simple fundamentals (integers, string, etc). not containers, dicts or structs
	struct resolvable_variant
	{
		type_descriptor descriptor;
		std::any value;

		/**
		 *  Use if you know the type without looking at the descriptor
		 */
		template <typename T>
		T resolve() const
		{
			return std::any_cast <T> (value);
		}

		/**
		 *  Use if you know the type without looking at the descriptor
		 */
		template <typename T>
		void resolve(T& value) const
		{
			value = std::any_cast <T> (value);
		}

		/**
		 *  Use if you dont know the type.
		 */
		template <typename FunctionT>
		void resolve(FunctionT func) const
		{
			for_signature_do(descriptor, [this, &func](auto dummy){
				using value_type = std::decay_t<decltype(dummy)>;
				func(std::any_cast <value_type> (value));
			});
		}
	};

	/**
	 * @brief The message_variant struct is a variant that can only be read by hand and cannot be autoresolved like
	 *		  the resolvable variant, which automagically reads the contained type into a variable.
	 *		  This class was neccessary after I realized that the autoresolve mechanic will instantiate every possible
	 *		  type in the universe for non trivial types, so this class was made.
	 */
	struct message_variant
	{
	public:
		friend class message_variant_resolver;

	public:
		explicit message_variant(class message& msg);
		explicit message_variant(class message&& msg);
		message_variant() = default;

		/**
		 * @brief message_variant Takes ownership of an owning message pointer.
		 * @param msg An owning message pointer.
		 */
		explicit message_variant(class message* msg);

		message_variant(message_variant&&) = default;
		message_variant& operator=(message_variant&&) = default;

		/**
		 * @brief message_variant Copies a message_variant. WARNING will rewind source, so dont copy on incomplete read.
		 *						  I dont really have a solution for this.
		 */
		message_variant(message_variant const&);

		/**
		 * @brief message_variant Copies a message_variant. WARNING will rewind source, so dont copy on incomplete read.
		 *						  I dont really have a solution for this.
		 */
		message_variant& operator=(message_variant const&);

		/**
		 * @brief assign this is not meant to be used directly by the user. It takes a variant off of msg and assings
		 *				 it to *this.
		 * @param msg A message that has a variant to read next.
		 * @return A sdbus result value.
		 */
		int assign(message& msg);

		/**
		 * @brief clear	Destroys the variants value.
		 */
		void clear();

		/**
		 * @brief release Releases ownership of the held message.
		 * @return
		 */
		message* release();

		/**
		 * @brief rewind Rewinds read pointer of variant to front.
		 */
		void rewind(bool complete = false);

		/**
		 * @brief append_to Append this variant to another message.
		 * @param other Another message.
		 * @return
		 */
		int append_to(message& other) const;

		type_descriptor type() const;

	private:
		/// Contains the variant data. Can be anything
		std::unique_ptr <message> message_;
	};

	using variant = message_variant;

	template <template <typename...> typename MapT, typename... Remain>
	using variant_dictionary = MapT <std::string, variant, Remain...>;

	/**
	 *	Creates a type string for the given type list
	 */
	template <typename... Types>
	struct type_constructor
	{
		static void insert(std::string& s, char const* v)
		{
			s += v;
		}

		static std::string make_type()
		{
			std::string result;
			(insert(result, type_detect <Types>::value),...);
			return result;
		}

		static type_descriptor make_descriptor()
		{
			auto res = make_type();
			if (res.size() == 1)
				return {res.front(), ""};
			if (res.front() == 'a')
				return {'a', res.substr(1, res.size() - 1)};

			// maybe wrong?
			return {res.front(), res.substr(1, res.size() - 1)};
		}
	};

	template <typename T>
	resolvable_variant make_resolvable_variant(T const& val)
	{
		resolvable_variant vari;
		vari.descriptor.contained = type_detect <T>::value;
		vari.descriptor.type = type_detect <T>::value[0];
		vari.value = val;
		return vari;
	}

	/**
	 * @brief Converts a sdbus type char into a readable type name
	 * @param type A sdbus type char
	 * @return A comprehensible name
	 */
	std::string typeToComprehensible(char type);

	/**
	 * @brief Converts a sdbus type into the expected type in C
	 * @param type A sdbus type char
	 * @return A C type name
	 */
	std::string typeToCpp(char type);

	template <>
	struct type_detect <uint8_t>
	{
		constexpr static char const* value = "y";
	};

	template <>
	struct type_detect <bool>
	{
		constexpr static char const* value = "b";
	};

	template <>
	struct type_detect <int16_t>
	{
		constexpr static char const* value = "n";
	};

	template <>
	struct type_detect <uint16_t>
	{
		constexpr static char const* value = "q";
	};

	template <>
	struct type_detect <int32_t>
	{
		constexpr static char const* value = "i";
	};

	template <>
	struct type_detect <uint32_t>
	{
		constexpr static char const* value = "u";
	};

	template <>
	struct type_detect <int64_t>
	{
		constexpr static char const* value = "x";
	};

	template <>
	struct type_detect <uint64_t>
	{
		constexpr static char const* value = "t";
	};

	template <>
	struct type_detect <double>
	{
		constexpr static char const* value = "d";
	};

	template <>
	struct type_detect <object_path>
	{
		constexpr static char const* value = "o";
	};

	template <>
	struct type_detect <signature>
	{
		constexpr static char const* value = "g";
	};

	template <>
	struct type_detect <file_descriptor>
	{
		constexpr static char const* value = "h";
	};

	template <>
	struct type_detect <std::string>
	{
		constexpr static char const* value = "s";
	};

	template <>
	struct type_detect <char const*>
	{
		constexpr static char const* value = "s";
	};

	template <typename T>
	struct type_detect <T, std::enable_if_t <std::is_class_v <T> && AdaptedStructs::struct_as_tuple <T>::is_adapted>>
	{
		constexpr static char const* value = "r";
	};

	template <typename T>
	struct type_converter <T, std::enable_if_t <std::is_fundamental_v <T>>>
	{
		template <typename U>
		static T convert(U orig)
		{
			return orig;
		}
	};

	template <>
	struct type_converter <std::string, void>
	{
		static char const* convert(std::string const& orig)
		{
			return orig.c_str();
		}
	};

	template <>
	struct type_converter <char const*, void>
	{
		static char const* convert(char const* orig)
		{
			return orig;
		}
	};
}
