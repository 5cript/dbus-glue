#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <tuple>
#include <memory>

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

    class interface_mock_base : public std::enable_shared_from_this<interface_mock_base>
    {
      public:
        template <typename>
        friend class interface_async_proxy;
        template <typename>
        friend class interface_async_property_proxy;

      private:
        dbus& bus_;
        std::string service_;
        std::string path_;
        std::string interface_;

      public:
        interface_mock_base(dbus& bus, std::string service, std::string path, std::string interface)
            : bus_{bus}
            , service_{std::move(service)}
            , path_{std::move(path)}
            , interface_{std::move(interface)}
        {}

        template <typename Derived>
        std::shared_ptr<Derived> shared_from_base()
        {
            return std::static_pointer_cast<Derived>(shared_from_this());
        }

        template <typename T>
        void read_property(std::string_view property_name, T& prop) const
        {
            bus_.read_property(service_, path_, interface_, property_name, prop);
        }

        template <typename T>
        void write_property(std::string_view property_name, T const& prop)
        {
            bus_.write_property(service_, path_, interface_, property_name, prop);
        }

        template <typename... ParametersT>
        void call_method_no_reply(std::string_view method_name, ParametersT const&... parameters)
        {
            bus_.call_method(service_, path_, interface_, method_name, parameters...);
        }

        template <typename ReturnT, typename... ParametersT>
        ReturnT call_method(std::string_view method_name, ParametersT const&... parameters)
        {
            auto message = bus_.call_method(service_, path_, interface_, method_name, parameters...);
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
            bus_.call_method_async(
                service_, path_, interface_, method_name, cb, err, timeout, std::forward<ParametersT&&>(parameters)...);
        }

        template <typename T>
        void read_property_async(
            std::string_view property_name,
            std::function<void(T const&)> const& cb,
            std::function<void(message&, std::string const&)> const& err,
            std::chrono::microseconds timeout) const
        {
            bus_.read_property_async<T>(service_, path_, interface_, property_name, cb, err, timeout);
        }

        template <typename T>
        void write_property_async(
            std::string_view property_name,
            std::function<void()> const& cb,
            std::function<void(message&, std::string const&)> const& err,
            std::chrono::microseconds timeout,
            T const& prop)
        {
            bus_.write_property_async(service_, path_, interface_, property_name, cb, err, timeout, prop);
        }

        template <typename... Args>
        auto install_signal_listener(
            std::string_view signal_name,
            std::function<void(Args const&...)> const& cb,
            std::function<void(message&, std::string const&)> const& err,
            bool release)
        {
            return bus_.install_signal_listener(service_, path_, interface_, signal_name, cb, err, release);
        }

        virtual ~interface_mock_base() = default;
    };

    template <typename R>
    class interface_async_base
    {
      public:
        virtual ~interface_async_base() = default;

        interface_async_base(
            std::weak_ptr<interface_mock_base> base,
            std::string_view sview // generic meaning
            )
            : base_{std::move(base)}
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
        std::weak_ptr<interface_mock_base> base_;
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
        interface_async_proxy(std::weak_ptr<interface_mock_base> base, std::string_view method_name)
            : interface_async_base<R>{std::move(base), method_name}
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
                    auto base = interface_async_base<R>::base_.lock();
                    if (!base)
                    {
                        // eat the error
                    }
                    base->call_method_async(std::forward<decltype(parms)&&>(parms)...);
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
        interface_async_property_proxy(std::weak_ptr<interface_mock_base> base, std::string_view property_name)
            : interface_async_base<R>{std::move(base), property_name}
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
                        if (auto base = interface_async_base<R>::base_.lock(); base)
                            base->read_property_async(std::forward<decltype(parms)&&>(parms)...);
                    },
                    std::move(interface_async_base<R>::base_params_));
            }
            else
            {
                auto catuple = std::tuple_cat(std::move(interface_async_base<R>::base_params_), std::move(params_));
                std::apply(
                    [this](auto&&... parms) {
                        if (auto base = interface_async_base<R>::base_.lock(); base)
                            base->write_property_async(std::forward<decltype(parms)&&>(parms)...);
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
