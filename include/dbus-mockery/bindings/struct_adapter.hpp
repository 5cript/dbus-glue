#pragma once

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/enum.hpp>

#include <tuple>

namespace DBusMock::AdaptedStructs
{
    template <typename T>
    struct struct_as_tuple
    {
        constexpr static bool is_adapted = false;
    };
}

#define MAKE_DBUS_STRUCT_DECLTYPE(r, data, elem) \
    (decltype(data::elem))

#define MAKE_DBUS_STRUCT_RETRIEVER(r, data, i, elem) \
    result.elem = std::get <i> (tup);

#define MAKE_DBUS_STRUCT_PACKER(r, data, i, elem) \
    std::get <i> (tup) = n.elem;

#define MAKE_DBUS_STRUCT_IMPL(Name, SEQ) \
namespace DBusMock::AdaptedStructs \
{ \
    template <> \
    struct struct_as_tuple <Name> \
    { \
        constexpr static bool is_adapted = true;\
        \
        using tuple_type = std::tuple < \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(MAKE_DBUS_STRUCT_DECLTYPE, Name, SEQ)) \
        >; \
        \
        static Name from_tuple(tuple_type const& tup) \
        { \
            Name result; \
            BOOST_PP_SEQ_FOR_EACH_I(MAKE_DBUS_STRUCT_RETRIEVER, tup, SEQ) \
            return result; \
        } \
        \
        static tuple_type to_tuple(Name const& n) \
        { \
            tuple_type tup;\
            BOOST_PP_SEQ_FOR_EACH_I(MAKE_DBUS_STRUCT_PACKER, tup, SEQ) \
            return tup; \
        } \
    }; \
}

#define MAKE_DBUS_STRUCT(Name, ...) \
    MAKE_DBUS_STRUCT_IMPL(Name, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
