#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <any>

// https://dbus.freedesktop.org/doc/dbus-specification.html#type-system

namespace DBusMock::Bindings
{
    /**
     *	Converts a C++ type int a dbus char.
     */
    template <typename T>
    struct type_detect{};

    struct type_descriptor
    {
        char type;
        std::string_view contained;
    };

    struct resolvable_variant
    {
        char contained;
        std::any value;

        template <typename T>
        T resolve()
        {
            return std::any_cast <T> (value);
        }

        template <typename T>
        void resolve(T& value)
        {
            value = std::any_cast <T> (value);
        }
    };

    template <template <typename...> MapT>
    using variant_dictionary = MapT <std::string, resolvable_variant>

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
    struct type_detect <std::string>
    {
        constexpr static char const* value = "s";
    };

    template <>
    struct type_detect <char const*>
    {
        constexpr static char const* value = "s";
    };
}
