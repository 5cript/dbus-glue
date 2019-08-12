#pragma once

#include "methods.hpp"
#include "properties.hpp"
#include "signals.hpp"

#include <iostream>
#include <boost/type_index.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/pop_front.hpp>
#include <boost/preprocessor/seq/push_front.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/control/if.hpp>

#include <tuple>
#include <type_traits>

namespace DBusMock::Mocks
{
    namespace detail
    {
        template <typename>
        struct method_dissect
        {
        };

        template <typename R, typename IFace, typename... Parameters>
        struct method_dissect <R(IFace::*)(Parameters...)>
        {
            using interface_type = IFace;
            using return_type = R;
            using parameters = std::tuple <Parameters...>;
        };
    }

    struct interface_mock_n_dummy
    {
        virtual ~interface_mock_n_dummy() = default;
    };

    template <typename InterfaceT, std::size_t Index>
    struct interface_mock_n{};

    template <typename InterfaceT>
    struct interface_mock{};
}

/// Namespace Utility
#define DBUS_MOCK_NAMESPACE_COLON_SEQ_EACH(r, data, i, elem) \
    BOOST_PP_IF(i, ::, BOOST_PP_EMPTY()) elem

#define DBUS_MOCK_NAMESPACE_COLON_DASH_SEQ_EACH(r, data, i, elem) \
    BOOST_PP_IF(i, ::, BOOST_PP_EMPTY()) BOOST_PP_CAT(elem, _)

#define DBUS_MOCK_NAMESPACE_COLON_SEQ(nspace) \
    BOOST_PP_SEQ_FOR_EACH_I(DBUS_MOCK_NAMESPACE_COLON_SEQ_EACH, ::, nspace)

#define DBUS_MOCK_NAMESPACE_COLON_SEQ_DASH(nspace) \
    BOOST_PP_SEQ_FOR_EACH_I(DBUS_MOCK_NAMESPACE_COLON_DASH_SEQ_EACH, ::, nspace)

#define DBUS_MOCK_EXPAND_NSPACE_RIGHT(nspace) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(nspace),, DBUS_MOCK_NAMESPACE_COLON_SEQ(BOOST_PP_SEQ_POP_FRONT(nspace))::)

#define DBUS_MOCK_EXPAND_NSPACE_LEFT(nspace) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(nspace),, ::DBUS_MOCK_NAMESPACE_COLON_SEQ(BOOST_PP_SEQ_POP_FRONT(nspace)))

#define DBUS_MOCK_EXPAND_NSPACE_LEFT_DASH(nspace) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(nspace),, ::DBUS_MOCK_NAMESPACE_COLON_SEQ_DASH(BOOST_PP_SEQ_POP_FRONT(nspace)))

#define DBUS_MOCK_EXPAND_NSPACE_RIGHT_DASH(nspace) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(nspace),, DBUS_MOCK_NAMESPACE_COLON_SEQ_DASH(BOOST_PP_SEQ_POP_FRONT(nspace))::)

#define DBUS_MOCK_EXPAND_NSPACE_INTERMEDIARY(nspace) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(nspace),, ::DBUS_MOCK_NAMESPACE_COLON_SEQ(BOOST_PP_SEQ_POP_FRONT(nspace))::)

#define DBUS_MOCK_EXPAND_NSPACE_INTERMEDIARY_DASH(nspace) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(nspace),::, ::DBUS_MOCK_NAMESPACE_COLON_SEQ_DASH(BOOST_PP_SEQ_POP_FRONT(nspace))::)

#define DBUS_MOCK_SEQUENCE_FACTORY_0(...) \
     ((__VA_ARGS__)) DBUS_MOCK_SEQUENCE_FACTORY_1
#define DBUS_MOCK_SEQUENCE_FACTORY_1(...) \
     ((__VA_ARGS__)) DBUS_MOCK_SEQUENCE_FACTORY_0
#define DBUS_MOCK_SEQUENCE_FACTORY_0_END
#define DBUS_MOCK_SEQUENCE_FACTORY_1_END

#define DBUS_MOCK_METHOD_DISSECT(IFace, Method) method_dissect<decltype(&IFace::Method)>

#define DBUS_MOCK_METHOD_RETURN(IFace, Method) DBUS_MOCK_METHOD_DISSECT(IFace, Method)::return_type

#define DBUS_MOCK_METHODS(...) BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), 1)

#define DBUS_MOCK_PROPERTIES(...) BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), 1)

#define DBUS_MOCK_SIGNALS(...) BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), 1)

#define DBUS_MOCK_NO_METHODS BOOST_PP_SEQ_PUSH_FRONT((X), 0)
#define DBUS_MOCK_NO_PROPERTIES BOOST_PP_SEQ_PUSH_FRONT((X), 0)
#define DBUS_MOCK_NO_SIGNALS BOOST_PP_SEQ_PUSH_FRONT((X), 0)

#define DBUS_MOCK_METHOD_HELPER_FORGE(IFace, Method) \
    BOOST_PP_CAT(IFace ## _mock_help_, Method)

#define DBUS_MOCK_METHOD_SINGLE_IMPL(NSpace, IFace, Method) \
    namespace DBusMock::Mocks::detail DBUS_MOCK_EXPAND_NSPACE_LEFT_DASH(NSpace) \
    { \
        template <typename Owner, typename> \
        struct DBUS_MOCK_METHOD_HELPER_FORGE(IFace, Method) \
        { \
        };\
        \
        template <typename Owner, typename IFace, typename... Parameters> \
        struct DBUS_MOCK_METHOD_HELPER_FORGE(IFace, Method) <Owner, void(IFace::*)(Parameters...)> \
            : public virtual interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base;\
            \
            auto Method(Parameters const&... params) -> void \
            { \
                call_method_no_reply(BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(DBusMock::async_flag_t, ParametersDeduced&&... params) \
            { \
                interface_async_proxy <void(Parameters...)> prox{*this, BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward <ParametersDeduced&&> (params)...); \
                return prox; \
            } \
        };\
        \
        template <typename Owner, typename R, typename IFace, typename... Parameters> \
        struct DBUS_MOCK_METHOD_HELPER_FORGE(IFace, Method) <Owner, R(IFace::*)(Parameters...)> \
            : public virtual interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base;\
            \
            auto Method(Parameters const&... params) -> R \
            { \
                return call_method <R, Parameters...> (BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(DBusMock::async_flag_t, ParametersDeduced&&... params) \
            { \
                interface_async_proxy <R(Parameters...)> prox{*this, BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward <ParametersDeduced&&> (params)...); \
                return prox; \
            } \
        };\
        \
        template <typename Owner, typename R, typename IFace, typename... Parameters> \
        struct DBUS_MOCK_METHOD_HELPER_FORGE(IFace, Method) <Owner, R(IFace::*)(Parameters...) const> \
            : public virtual interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base;\
            \
            auto Method(Parameters const&... params) const -> R \
            { \
                return call_method <R, Parameters...> (BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(DBusMock::async_flag_t, ParametersDeduced&&... params) const \
            { \
                interface_async_proxy <R(Parameters...)> prox{*this, BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward <ParametersDeduced&&> (params)...); \
                return prox; \
            } \
        };\
        \
        template <typename Owner, typename IFace, typename... Parameters> \
        struct DBUS_MOCK_METHOD_HELPER_FORGE(IFace, Method) <Owner, void(IFace::*)(Parameters...) const> \
            : public virtual interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base;\
            \
            auto Method(Parameters const&... params) const -> void \
            { \
                call_method_no_reply(BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(DBusMock::async_flag_t, ParametersDeduced&&... params) const \
            { \
                interface_async_proxy <void(Parameters...)> prox{*this, BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward <ParametersDeduced&&> (params)...); \
                return prox; \
            } \
        };\
    }

// Data = (NSpace, IFace)
#define DBUS_MOCK_METHOD_SINGLE(r, Data, Method) \
    DBUS_MOCK_METHOD_SINGLE_IMPL( \
        BOOST_PP_TUPLE_ELEM(0, Data), \
        BOOST_PP_TUPLE_ELEM(1, Data), \
        Method \
    )

// Data = (NSpace, IFace, OwnerPart1, OwnerPart2)
#define DBUS_MOCK_METHOD_TYPE_FORGE(Data, Method) \
    detail DBUS_MOCK_EXPAND_NSPACE_INTERMEDIARY_DASH(BOOST_PP_TUPLE_ELEM(0, Data)) \
    BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(1, Data), BOOST_PP_CAT(_mock_help_, Method)) \
    <BOOST_PP_TUPLE_ELEM(2, Data), BOOST_PP_TUPLE_ELEM(3, Data), \
        decltype(&DBUS_MOCK_EXPAND_NSPACE_RIGHT(BOOST_PP_TUPLE_ELEM(0, Data)) BOOST_PP_TUPLE_ELEM(1, Data)::Method)>

#define DBUS_MOCK_METHOD_HELPER(NSpace, IFace, Methods) \
    BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_METHOD_SINGLE, (NSpace, IFace), Methods)

#define DBUS_MOCK_METHOD_DERIVE(r, Data, Method) \
    , public DBUS_MOCK_METHOD_TYPE_FORGE(Data, Method)

#define DBUS_MOCK_PROPERTY_ROLL(r, IFace, Property) \
    std::decay_t <decltype(IFace::Property)> Property;

#define DBUS_MOCK_PROPERTY_CTOR(r, IFace, Property) \
    , Property{this, BOOST_PP_STRINGIZE(Property)}

#define DBUS_MOCK_METHOD_CTOR(r, Data, Method) \
    , DBUS_MOCK_METHOD_TYPE_FORGE(Data, Method){bus, service, path, interface}

#define DBUS_MOCK_DO_NOTHING(...)

// Numerator => some interfaces are very large, and i will hit boost preprocessor limits there :/
// This is why there is this numeration workaround.
#define DBUS_MOCK_IMPL(NUMERATOR, NSpace, IFace, Methods, Properties, Signals) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Methods), DBUS_MOCK_METHOD_HELPER, DBUS_MOCK_DO_NOTHING)(NSpace, IFace, BOOST_PP_SEQ_POP_FRONT(Methods)) \
    \
    namespace DBusMock::Mocks \
    { \
        template <> \
        struct interface_mock_n <DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace, NUMERATOR> \
            : virtual interface_mock_base \
              BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Methods), BOOST_PP_SEQ_FOR_EACH, DBUS_MOCK_DO_NOTHING)(DBUS_MOCK_METHOD_DERIVE, (NSpace, IFace, \
                interface_mock_n <DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace, NUMERATOR>), BOOST_PP_SEQ_POP_FRONT(Methods)) \
        { \
        public: \
            BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Properties), \
                BOOST_PP_SEQ_FOR_EACH, DBUS_MOCK_DO_NOTHING \
            )(DBUS_MOCK_PROPERTY_ROLL, DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace, BOOST_PP_SEQ_POP_FRONT(Properties))\
        \
        public: \
            interface_mock_n( \
                Bindings::dbus& bus, \
                std::string const& service, \
                std::string const& path, \
                std::string const& interface \
            ) \
                : interface_mock_base{bus, service, path, interface} \
                  BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Methods), BOOST_PP_SEQ_FOR_EACH, DBUS_MOCK_DO_NOTHING)( \
                    DBUS_MOCK_METHOD_CTOR, \
                    (NSpace, IFace, interface_mock_n <DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace, NUMERATOR>), \
                    BOOST_PP_SEQ_POP_FRONT(Methods) \
                  ) \
                  BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Properties), \
                    BOOST_PP_SEQ_FOR_EACH, DBUS_MOCK_DO_NOTHING \
                  )( \
                    DBUS_MOCK_PROPERTY_CTOR, \
                    DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                    BOOST_PP_SEQ_POP_FRONT(Properties) \
                  ) \
            { \
            } \
            virtual ~interface_mock_n() = default; \
        }; \
    }

#define DBUS_MOCK_DERIVE_ZIP_SEQ_EACH(r, data, elem) \
    interface_mock_n <data, elem>,

#define DBUS_MOCK_CTOR_ZIP_SEQ_EACH(r, data, elem) \
    interface_mock_n <data, elem>{bus, service, path, interface},

/**
 * Used to merge all numerated mocks
 * @param IFace the Interface to mock
 * @param ... All numbers that were used for DBUS_MOCK_N
 */
#define DBUS_MOCK_ZIP_IMPL(NSpace, IFace, SEQ) \
namespace DBusMock::Mocks \
{ \
    template <> \
    struct interface_mock <DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace> \
        : BOOST_PP_SEQ_FOR_EACH( \
            DBUS_MOCK_DERIVE_ZIP_SEQ_EACH, \
            DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
            SEQ \
        ) interface_mock_n_dummy \
    { \
        interface_mock( \
            Bindings::dbus& bus, \
            std::string const& service, \
            std::string const& path, \
            std::string const& interface \
        ) \
            : interface_mock_base{bus, service, path, interface} \
            , BOOST_PP_SEQ_FOR_EACH( \
                DBUS_MOCK_CTOR_ZIP_SEQ_EACH, \
                DBUS_MOCK_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                SEQ \
            ) interface_mock_n_dummy{} \
        { \
        } \
    }; \
}

#define DBUS_MOCK_ZIP_IMPL_2(NSpace, IFace, SEQ) \
    DBUS_MOCK_ZIP_IMPL(NSpace, IFace, SEQ)

#define DBUS_MOCK_ZIP(NSpace, IFace, ...) \
    DBUS_MOCK_ZIP_IMPL_2(NSpace, IFace, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define DBUS_MOCK(IFace, Methods, Properties, Signals) \
    DBUS_MOCK_IMPL(0, BOOST_PP_SEQ_PUSH_FRONT((X), 1), IFace, Methods, Properties, Signals) \
    DBUS_MOCK_ZIP(BOOST_PP_SEQ_PUSH_FRONT((X), 1), IFace, 0)

#define DBUS_MOCK_N(N, IFace, Methods, Properties, Signals) \
    DBUS_MOCK_IMPL(N, BOOST_PP_SEQ_PUSH_FRONT((X), 1), IFace, Methods, Properties, Signals)

#define DBUS_MOCK_NAMESPACE(NSpace, IFace, Methods, Properties, Signals) \
    DBUS_MOCK_IMPL(0, BOOST_PP_SEQ_PUSH_FRONT(NSpace, 0), IFace, Methods, Properties, Signals) \
    DBUS_MOCK_ZIP(BOOST_PP_SEQ_PUSH_FRONT(NSpace, 0), IFace, 0)

#define DBUS_MOCK_NAMESPACE_N(N, NSpace, IFace, Methods, Properties, Signals) \
    DBUS_MOCK_IMPL(N, BOOST_PP_SEQ_PUSH_FRONT(NSpace, 0), IFace, Methods, Properties, Signals)

namespace DBusMock
{
    template <typename T>
    auto create_interface
    (
        Bindings::dbus& bus,
        std::string const& service,
        std::string const& path,
        std::string const& interface
    )
    {
        return Mocks::interface_mock <T>{bus, service, path, interface};
    }
}
