#pragma once

#include "sdbus_core.hpp"
#include "object_path.hpp"
#include "signature.hpp"
#include "file_descriptor.hpp"
#include "struct_adapter.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <any>
#include <stdexcept>
#include <utility>
#include <type_traits>

// https://dbus.freedesktop.org/doc/dbus-specification.html#type-system

namespace DBusMock::Bindings
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
        char type;
        std::string_view contained;
    };

    template <typename FunctionT>
    void for_signature_do (type_descriptor descr, FunctionT func)
    {
        switch (descr.type)
        {
            case('y'): return func(uint8_t{});
            case('b'): return func(bool{});
            case('n'): return func(int16_t{});
            case('q'): return func(uint16_t{});
            case('i'): return func(int32_t{});
            case('u'): return func(uint32_t{});
            case('x'): return func(int64_t{});
            case('t'): return func(uint64_t{});
            case('d'): return func(double{});
            case('s'): return func(std::string{});
            default:
                throw std::domain_error("for now unimplemented type");
        }
    }

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

    using variant = resolvable_variant;

    template <template <typename...> typename MapT, typename... Remain>
    using variant_dictionary = MapT <std::string, resolvable_variant, Remain...>;

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
    };

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
