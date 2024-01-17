#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <tuple>

#include "bindings/bus.hpp"
#include "bindings/message.hpp"

namespace DBusGlue::Mocks
{
    namespace detail
    {
        template <typename R>
        struct func_param_devoid
        {
            using type = std::function<void(R const&)>;
        };

        template <>
        struct func_param_devoid<void>
        {
            using type = std::function<void()>;
        };
    }

    template <typename CallbackT>
    class interface_async_proxy
    {};
    template <typename CallbackT>
    class interface_async_property_proxy
    {};

    class interface_mock_base
    {
      public:
        template <typename>
        friend class interface_async_proxy;
        template <typename>
        friend class interface_async_property_proxy;

      private:
        dbus& bus;
        std::string service;
        std::string path;
        std::string interface;

      public:
        interface_mock_base(
            dbus& bus,
            std::string const& service,
            std::string const& path,
            std::string const& interface)
            : bus{bus}
            , service{service}
            , path{path}
            , interface {
            interface
        }
        {}

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
        void call_method_async(
            std::string_view method_name,
            std::function<CallbackT> const& cb,
            std::function<void(message&, std::string const&)> const& err,
            std::chrono::microseconds timeout,
            ParametersT&&... parameters)
        {
            bus.call_method_async(
                service, path, interface, method_name, cb, err, timeout, std::forward<ParametersT&&>(parameters)...);
        }

        template <typename T>
        void read_property_async(
            std::string_view property_name,
            std::function<void(T const&)> const& cb,
            std::function<void(message&, std::string const&)> const& err,
            std::chrono::microseconds timeout) const
        {
            bus.read_property_async<T>(service, path, interface, property_name, cb, err, timeout);
        }

        template <typename T>
        void write_property_async(
            std::string_view property_name,
            std::function<void()> const& cb,
            std::function<void(message&, std::string const&)> const& err,
            std::chrono::microseconds timeout,
            T const& prop)
        {
            bus.write_property_async(service, path, interface, property_name, cb, err, timeout, prop);
        }

        template <typename... Args>
        auto install_signal_listener(
            std::string_view signal_name,
            std::function<void(Args const&...)> const& cb,
            std::function<void(message&, std::string const&)> const& err,
            bool release)
        {
            return bus.install_signal_listener(service, path, interface, signal_name, cb, err, release);
        }

        virtual ~interface_mock_base() = default;
    };

    template <typename R>
    class interface_async_base
    {
      public:
        virtual ~interface_async_base() = default;

        interface_async_base(
            interface_mock_base& base,
            std::string_view sview // generic meaning
            )
            : base_{base}
            , base_params_{sview, {}, {}, std::chrono::seconds{10}}
        {}

        interface_async_base& then(typename detail::func_param_devoid<R>::type const& cb)
        {
            std::get<1>(base_params_) = cb;
            return *this;
        }

        interface_async_base& error(std::function<void(message&, std::string const&)> const& fail)
        {
            std::get<2>(base_params_) = fail;
            return *this;
        }

        interface_async_base& timeout(std::chrono::microseconds timeout)
        {
            std::get<3>(base_params_) = timeout;
            return *this;
        }

      protected:
        interface_mock_base& base_;
        std::tuple<
            std::string_view,
            typename detail::func_param_devoid<R>::type,
            std::function<void(message&, std::string const&)>,
            std::chrono::microseconds>
            base_params_;
    };

    template <typename R, typename... ParametersT>
    class interface_async_proxy<R(ParametersT...)> : public interface_async_base<R>
    {
      public:
        interface_async_proxy(interface_mock_base& base, std::string_view method_name)
            : interface_async_base<R>{base, method_name}
            , params_{}
        {}

        interface_async_proxy& operator=(interface_async_proxy const&) = delete;
        interface_async_proxy& operator=(interface_async_proxy&&) = default;
        interface_async_proxy(interface_async_proxy const&) = delete;
        interface_async_proxy(interface_async_proxy&&) = default;

        template <typename... ParametersDeduced>
        interface_async_proxy& bind_parameters(ParametersDeduced&&... params)
        {
            params_ = std::tuple<std::decay_t<ParametersDeduced>...>{std::forward<ParametersDeduced&&>(params)...};
            return *this;
        }

        ~interface_async_proxy()
        {
            auto catuple = std::tuple_cat(std::move(interface_async_base<R>::base_params_), std::move(params_));
            std::apply(
                [this](auto&&... parms) {
                    interface_async_base<R>::base_.call_method_async(std::forward<decltype(parms)&&>(parms)...);
                },
                std::move(catuple));
        }

      private:
        std::tuple<std::decay_t<ParametersT>...> params_;
    };

    template <typename R, typename... ParametersT>
    class interface_async_property_proxy<R(ParametersT...)> : public interface_async_base<R>
    {
      public:
        interface_async_property_proxy(interface_mock_base& base, std::string_view property_name)
            : interface_async_base<R>{base, property_name}
            , params_{}
        {}

        interface_async_property_proxy& operator=(interface_async_property_proxy const&) = delete;
        interface_async_property_proxy& operator=(interface_async_property_proxy&&) = default;
        interface_async_property_proxy(interface_async_property_proxy const&) = delete;
        interface_async_property_proxy(interface_async_property_proxy&&) = default;

        template <typename... ParametersDeduced>
        interface_async_property_proxy& bind_parameters(ParametersDeduced&&... params)
        {
            if constexpr (sizeof...(ParametersT) != 0)
                params_ = {std::forward<ParametersDeduced&&>(params)...};
            return *this;
        }

        ~interface_async_property_proxy()
        {
            if constexpr (sizeof...(ParametersT) == 0)
            {
                std::apply(
                    [this](auto&&... parms) {
                        interface_async_base<R>::base_.read_property_async(std::forward<decltype(parms)&&>(parms)...);
                    },
                    std::move(interface_async_base<R>::base_params_));
            }
            else
            {
                auto catuple = std::tuple_cat(std::move(interface_async_base<R>::base_params_), std::move(params_));
                std::apply(
                    [this](auto&&... parms) {
                        interface_async_base<R>::base_.write_property_async(std::forward<decltype(parms)&&>(parms)...);
                    },
                    std::move(catuple));
            }
        }

      private:
        std::tuple<ParametersT...> params_;
    };
}

namespace DBusGlue
{
    /// Dummy for overloading
    struct async_flag_t
    {};

    constexpr async_flag_t async_flag;
}
