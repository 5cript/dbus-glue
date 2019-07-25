#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <tuple>

#include "bindings/bus.hpp"
#include "bindings/message.hpp"

namespace DBusMock::Mocks
{
    template <typename CallbackT>
    class interface_async_proxy
    {
    };

    class interface_mock_base
    {
    private:
        Bindings::dbus& bus;
        std::string service;
        std::string path;
        std::string interface;

    protected:
        interface_mock_base
        (
            Bindings::dbus& bus,
            std::string const& service,
            std::string const& path,
            std::string const& interface
        )
            : bus{bus}
            , service{service}
            , path{path}
            , interface{interface}
        {
        }

        template <typename T>
        void read_property(std::string_view property_name, T& prop) const
        {
            bus.read_property(service, path, interface, property_name, prop);
        }

        template <typename T>
        void write_property(std::string_view property_name, T const& prop)
        {
            bus.write_property(service, path, interface, property_name, prop);
        }

        virtual ~interface_mock_base() = default;
    };

    class interface_method_base
    {
    public:
        template <typename> friend class interface_async_proxy;

    private:
        Bindings::dbus& bus;
        std::string service;
        std::string path;
        std::string interface;

    protected:
        interface_method_base
        (
            Bindings::dbus& bus,
            std::string const& service,
            std::string const& path,
            std::string const& interface
        )
            : bus{bus}
            , service{service}
            , path{path}
            , interface{interface}
        {
        }

        template <typename... ParametersT>
        void call_method_no_reply(std::string_view method_name, ParametersT const&... parameters)
        {
            bus.call_method(service, path, interface, method_name, parameters...);
        }

        template <typename ReturnT, typename... ParametersT>
        ReturnT call_method(std::string_view method_name, ParametersT const&... parameters)
        {
            auto message = bus.call_method(service, path, interface, method_name, parameters...);
            ReturnT retVal;
            message.read(retVal);
            return retVal;
        }

        template <typename CallbackT, typename... ParametersT>
        void call_method_async
        (
            std::string_view method_name,
            std::function <CallbackT> const& cb,
            std::function <void(Bindings::message&, std::string const&)> const& err,
            std::chrono::microseconds timeout,
            ParametersT&&... parameters
        )
        {
            bus.call_method_async
            (
                service,
                path,
                interface,
                method_name,
                cb,
                err,
                timeout,
                std::forward <ParametersT&&> (parameters)...
            );
        }

        virtual ~interface_method_base() = default;
    };

    template <typename R, typename... ParametersT>
    class interface_async_proxy <R(ParametersT...)>
    {
    public:
        interface_async_proxy
        (
            interface_method_base& base,
            std::string_view method_name
        )
            : base_{base}
            , base_params_{
                method_name,
                {},
                {},
                std::chrono::seconds{10}
            }
            , params_{}
        {
        }
        interface_async_proxy& operator=(interface_async_proxy const&) = delete;
        interface_async_proxy& operator=(interface_async_proxy&&) = default;
        interface_async_proxy(interface_async_proxy const&) = delete;
        interface_async_proxy(interface_async_proxy&&) = default;

        ~interface_async_proxy()
        {
            auto catuple = std::tuple_cat(std::move(base_params_), std::move(params_));
            std::apply
            (
                [this](auto&&... parms){base_.call_method_async(std::forward <decltype(parms)&&> (parms)...);},
                std::move(catuple)
            );
        }

        template <typename... ParametersDeduced>
        interface_async_proxy& bind_parameters
        (
            ParametersDeduced&&... params
        )
        {
            params_ = {std::forward <ParametersDeduced&&> (params)...};
            return *this;
        }

        interface_async_proxy& then(std::function <void(R)> const& cb)
        {
            std::get <1> (base_params_) = cb;
            return *this;
        }

        interface_async_proxy& error(std::function <void(Bindings::message&, std::string const&)> const& fail)
        {
            std::get <2> (base_params_) = fail;
            return *this;
        }

        interface_async_proxy& timeout(std::chrono::microseconds timeout)
        {
            std::get <3> (base_params_)  = timeout;
            return *this;
        }

    private:
        interface_method_base& base_;
        std::tuple <
            std::string_view,
            std::function <void(R)>,
            std::function <void(Bindings::message&, std::string const&)>,
            std::chrono::microseconds
        > base_params_;
        std::tuple <ParametersT...> params_;
    };
}

namespace DBusMock
{
    /// Dummy for overloading
    struct async_flag_t{};

    constexpr async_flag_t async_flag;
}
