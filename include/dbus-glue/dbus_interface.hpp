#pragma once

#include "methods.hpp"
#include "properties.hpp"
#include "signals.hpp"
#include "detail/preprocessor.hpp"
#include "bindings/detail/dissect.hpp"

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
#include <memory>

namespace DBusGlue::Mocks
{
    struct interface_mock_n_dummy
    {
        virtual ~interface_mock_n_dummy() = default;
    };

    template <typename InterfaceT, std::size_t Index>
    struct interface_mock_n
    {};

    template <typename InterfaceT>
    struct interface_mock
    {};
}

#define DBUS_DECLARE_METHOD_DISSECT(IFace, Method) method_dissect<decltype(&IFace::Method)>

#define DBUS_DECLARE_METHOD_RETURN(IFace, Method) DBUS_DECLARE_METHOD_DISSECT(IFace, Method)::return_type

#define DBUS_DECLARE_METHODS(...) BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), 1)

#define DBUS_DECLARE_PROPERTIES(...) BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), 1)

#define DBUS_DECLARE_SIGNALS(...) BOOST_PP_SEQ_PUSH_FRONT(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), 1)

#define DBUS_DECLARE_NO_METHODS BOOST_PP_SEQ_PUSH_FRONT((X), 0)
#define DBUS_DECLARE_NO_PROPERTIES BOOST_PP_SEQ_PUSH_FRONT((X), 0)
#define DBUS_DECLARE_NO_SIGNALS BOOST_PP_SEQ_PUSH_FRONT((X), 0)

#define DBUS_DECLARE_METHOD_HELPER_FORGE(IFace, Method) BOOST_PP_CAT(IFace##_mock_help_, Method)

#define DBUS_DECLARE_METHOD_SINGLE_IMPL(NSpace, IFace, Method) \
    namespace DBusGlue::Mocks::detail DBUS_DECLARE_EXPAND_NSPACE_LEFT_DASH(NSpace) \
    { \
        template <typename Owner, typename> \
        struct DBUS_DECLARE_METHOD_HELPER_FORGE(IFace, Method) \
        {}; \
\
        template <typename Owner, typename IFace, typename... Parameters> \
        struct DBUS_DECLARE_METHOD_HELPER_FORGE(IFace, Method)<Owner, void (IFace::*)(Parameters...)> \
            : public virtual ::DBusGlue::Mocks::interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base; \
\
            auto Method(Parameters const&... params) -> void \
            { \
                call_method_no_reply(BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(::DBusGlue::async_flag_t, ParametersDeduced&&... params) \
            { \
                ::DBusGlue::Mocks::interface_async_proxy<void(Parameters...)> prox{ \
                    weak_from_this(), BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward<ParametersDeduced&&>(params)...); \
                return prox; \
            } \
        }; \
\
        template <typename Owner, typename R, typename IFace, typename... Parameters> \
        struct DBUS_DECLARE_METHOD_HELPER_FORGE(IFace, Method)<Owner, R (IFace::*)(Parameters...)> \
            : public virtual ::DBusGlue::Mocks::interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base; \
\
            auto Method(Parameters const&... params) -> R \
            { \
                return call_method<R, Parameters...>(BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(::DBusGlue::async_flag_t, ParametersDeduced&&... params) \
            { \
                ::DBusGlue::Mocks::interface_async_proxy<R(Parameters...)> prox{ \
                    weak_from_this(), BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward<ParametersDeduced&&>(params)...); \
                return prox; \
            } \
        }; \
\
        template <typename Owner, typename R, typename IFace, typename... Parameters> \
        struct DBUS_DECLARE_METHOD_HELPER_FORGE(IFace, Method)<Owner, R (IFace::*)(Parameters...) const> \
            : public virtual ::DBusGlue::Mocks::interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base; \
\
            auto Method(Parameters const&... params) const -> R \
            { \
                return call_method<R, Parameters...>(BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(::DBusGlue::async_flag_t, ParametersDeduced&&... params) const \
            { \
                ::DBusGlue::Mocks::interface_async_proxy<R(Parameters...)> prox{ \
                    weak_from_this(), BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward<ParametersDeduced&&>(params)...); \
                return prox; \
            } \
        }; \
\
        template <typename Owner, typename IFace, typename... Parameters> \
        struct DBUS_DECLARE_METHOD_HELPER_FORGE(IFace, Method)<Owner, void (IFace::*)(Parameters...) const> \
            : public virtual ::DBusGlue::Mocks::interface_mock_base \
        { \
            using interface_mock_base::interface_mock_base; \
\
            auto Method(Parameters const&... params) const -> void \
            { \
                call_method_no_reply(BOOST_PP_STRINGIZE(Method), params...); \
            } \
            template <typename... ParametersDeduced> \
            auto Method(::DBusGlue::async_flag_t, ParametersDeduced&&... params) const \
            { \
                ::DBusGlue::Mocks::interface_async_proxy<void(Parameters...)> prox{ \
                    weak_from_this(), BOOST_PP_STRINGIZE(Method)}; \
                prox.bind_parameters(std::forward<ParametersDeduced&&>(params)...); \
                return prox; \
            } \
        }; \
    }

// Data = (NSpace, IFace)
#define DBUS_DECLARE_METHOD_SINGLE(r, Data, Method) \
    DBUS_DECLARE_METHOD_SINGLE_IMPL(BOOST_PP_TUPLE_ELEM(0, Data), BOOST_PP_TUPLE_ELEM(1, Data), Method)

// Data = (NSpace, IFace, OwnerPart1, OwnerPart2)
#define DBUS_DECLARE_METHOD_TYPE_FORGE(Data, Method) \
    detail DBUS_DECLARE_EXPAND_NSPACE_INTERMEDIARY_DASH(BOOST_PP_TUPLE_ELEM(0, Data)) \
        BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(1, Data), BOOST_PP_CAT(_mock_help_, Method))< \
            BOOST_PP_TUPLE_ELEM(2, Data), \
            BOOST_PP_TUPLE_ELEM(3, Data), \
            decltype(&DBUS_DECLARE_EXPAND_NSPACE_RIGHT(BOOST_PP_TUPLE_ELEM(0, Data)) \
                         BOOST_PP_TUPLE_ELEM(1, Data)::Method)>

#define DBUS_DECLARE_METHOD_HELPER(NSpace, IFace, Methods) \
    BOOST_PP_SEQ_FOR_EACH(DBUS_DECLARE_METHOD_SINGLE, (NSpace, IFace), Methods)

#define DBUS_DECLARE_METHOD_DERIVE(r, Data, Method) , public DBUS_DECLARE_METHOD_TYPE_FORGE(Data, Method)

#define DBUS_DECLARE_PROPERTY_ROLL(r, IFace, Property) std::decay_t<decltype(IFace::Property)> Property;

#define DBUS_INIT_PROPERTY_ROLL(r, IFace, Property) Property.set_base(shared_from_this());

#define DBUS_DECLARE_PROPERTY_CTOR(r, IFace, Property) \
    , Property \
    { \
        BOOST_PP_STRINGIZE(Property) \
    }

#define DBUS_DECLARE_METHOD_CTOR(r, Data, Method) \
    , DBUS_DECLARE_METHOD_TYPE_FORGE(Data, Method) \
    { \
        bus, service, path, interface \
    }

#define DBUS_DECLARE_DO_NOTHING(...)

// Numerator => some interfaces are very large, and i will hit boost preprocessor limits there :/
// This is why there is this numeration workaround.
#define DBUS_DECLARE_IMPL(NUMERATOR, NSpace, IFace, Methods, Properties, Signals) \
    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Methods), DBUS_DECLARE_METHOD_HELPER, DBUS_DECLARE_DO_NOTHING) \
    (NSpace, IFace, BOOST_PP_SEQ_POP_FRONT(Methods)) \
\
        namespace DBusGlue::Mocks \
    { \
        template <> \
        struct interface_mock_n<DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, NUMERATOR> \
            : virtual ::DBusGlue::Mocks::interface_mock_base BOOST_PP_IF( \
                  BOOST_PP_SEQ_HEAD(Methods), BOOST_PP_SEQ_FOR_EACH, DBUS_DECLARE_DO_NOTHING)( \
                  DBUS_DECLARE_METHOD_DERIVE, \
                  (NSpace, \
                   IFace, \
                   ::DBusGlue::Mocks::interface_mock_n<DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, NUMERATOR>), \
                  BOOST_PP_SEQ_POP_FRONT(Methods)) \
        { \
          public: \
            BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Properties), BOOST_PP_SEQ_FOR_EACH, DBUS_DECLARE_DO_NOTHING) \
            (DBUS_DECLARE_PROPERTY_ROLL, \
             DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
             BOOST_PP_SEQ_POP_FRONT(Properties)) \
\
                BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Signals), BOOST_PP_SEQ_FOR_EACH, DBUS_DECLARE_DO_NOTHING)( \
                    DBUS_DECLARE_PROPERTY_ROLL, \
                    DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                    BOOST_PP_SEQ_POP_FRONT(Signals)) \
\
                    public \
                : interface_mock_n(dbus& bus, std::string service, std::string path, std::string interface) \
                : ::DBusGlue::Mocks:: \
                      interface_mock_base{bus, std::move(service), std::move(path), std::move(interface)} BOOST_PP_IF( \
                          BOOST_PP_SEQ_HEAD(Methods), \
                          BOOST_PP_SEQ_FOR_EACH, \
                          DBUS_DECLARE_DO_NOTHING)( \
                          DBUS_DECLARE_METHOD_CTOR, \
                          (NSpace, \
                           IFace, \
                           ::DBusGlue::Mocks:: \
                               interface_mock_n<DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, NUMERATOR>), \
                          BOOST_PP_SEQ_POP_FRONT(Methods)) \
                          BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Properties), BOOST_PP_SEQ_FOR_EACH, DBUS_DECLARE_DO_NOTHING)( \
                              DBUS_DECLARE_PROPERTY_CTOR, \
                              DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                              BOOST_PP_SEQ_POP_FRONT(Properties)) \
                              BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Signals), BOOST_PP_SEQ_FOR_EACH, DBUS_DECLARE_DO_NOTHING)( \
                                  DBUS_DECLARE_PROPERTY_CTOR, \
                                  DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                                  BOOST_PP_SEQ_POP_FRONT(Signals)) \
            {} \
\
            virtual ~interface_mock_n() = default; \
\
            void init() \
            { \
                BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Properties), BOOST_PP_SEQ_FOR_EACH, DBUS_DECLARE_DO_NOTHING) \
                (DBUS_INIT_PROPERTY_ROLL, \
                 DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                 BOOST_PP_SEQ_POP_FRONT(Properties)) \
\
                    BOOST_PP_IF(BOOST_PP_SEQ_HEAD(Signals), BOOST_PP_SEQ_FOR_EACH, DBUS_DECLARE_DO_NOTHING)( \
                        DBUS_INIT_PROPERTY_ROLL, \
                        DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                        BOOST_PP_SEQ_POP_FRONT(Signals)) \
            } \
        }; \
    }

#define DBUS_DECLARE_DERIVE_ZIP_SEQ_EACH(r, data, elem) ::DBusGlue::Mocks::interface_mock_n<data, elem>,

#define DBUS_DECLARE_CTOR_ZIP_SEQ_EACH(r, data, elem) \
    ::DBusGlue::Mocks::interface_mock_n<data, elem>{bus, service, path, interface},

#define DBUS_ZIP_INIT_EACH(r, data, elem) ::DBusGlue::Mocks::interface_mock_n<data, elem>::init();

/**
 * Used to merge all numerated interfaces
 * @param IFace the Interface to mirror.
 * @param ... All numbers that were used for DBUS_DECLARE_N
 */
#define DBUS_DECLARE_ZIP_IMPL(NSpace, IFace, SEQ) \
    namespace DBusGlue::Mocks \
    { \
        template <> \
        struct interface_mock<DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace> \
            : BOOST_PP_SEQ_FOR_EACH( \
                  DBUS_DECLARE_DERIVE_ZIP_SEQ_EACH, \
                  DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                  SEQ) interface_mock_n_dummy \
        { \
            using interface_type = DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace; \
\
            interface_mock(dbus& bus, std::string service, std::string path, std::string interface) \
                : ::DBusGlue::Mocks:: \
                      interface_mock_base{bus, std::move(service), std::move(path), std::move(interface)} \
                , BOOST_PP_SEQ_FOR_EACH( \
                      DBUS_DECLARE_CTOR_ZIP_SEQ_EACH, \
                      DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, \
                      SEQ) interface_mock_n_dummy{} \
            {} \
\
            void init() \
            { \
                BOOST_PP_SEQ_FOR_EACH(DBUS_ZIP_INIT_EACH, DBUS_DECLARE_EXPAND_NSPACE_RIGHT(NSpace) IFace, SEQ) \
            } \
        }; \
    }

#define DBUS_DECLARE_ZIP_IMPL_2(NSpace, IFace, SEQ) DBUS_DECLARE_ZIP_IMPL(NSpace, IFace, SEQ)

#define DBUS_DECLARE_ZIP(NSpace, IFace, ...) \
    DBUS_DECLARE_ZIP_IMPL_2(NSpace, IFace, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define DBUS_DECLARE(IFace, Methods, Properties, Signals) \
    DBUS_DECLARE_IMPL(0, BOOST_PP_SEQ_PUSH_FRONT((X), 1), IFace, Methods, Properties, Signals) \
    DBUS_DECLARE_ZIP(BOOST_PP_SEQ_PUSH_FRONT((X), 1), IFace, 0)

#define DBUS_DECLARE_N(N, IFace, Methods, Properties, Signals) \
    DBUS_DECLARE_IMPL(N, BOOST_PP_SEQ_PUSH_FRONT((X), 1), IFace, Methods, Properties, Signals)

#define DBUS_DECLARE_NAMESPACE(NSpace, IFace, Methods, Properties, Signals) \
    DBUS_DECLARE_IMPL(0, BOOST_PP_SEQ_PUSH_FRONT(NSpace, 0), IFace, Methods, Properties, Signals) \
    DBUS_DECLARE_ZIP(BOOST_PP_SEQ_PUSH_FRONT(NSpace, 0), IFace, 0)

#define DBUS_DECLARE_NAMESPACE_N(N, NSpace, IFace, Methods, Properties, Signals) \
    DBUS_DECLARE_IMPL(N, BOOST_PP_SEQ_PUSH_FRONT(NSpace, 0), IFace, Methods, Properties, Signals)

namespace DBusGlue
{
    template <typename T>
    std::shared_ptr<Mocks::interface_mock<T>>
    create_interface(dbus& bus, std::string const& service, std::string const& path, std::string const& interface)
    {
        auto shared = std::make_shared<Mocks::interface_mock<T>>(bus, service, path, interface);
        shared->init();
        return shared;
    }

    template <typename T>
    auto create_interface(dbus& bus, std::string const& service, object_path const& path, std::string const& interface)
    {
        return create_interface<T>(bus, service, path.string(), interface);
    }
}
