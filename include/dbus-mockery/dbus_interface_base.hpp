#pragma once

#include <string>
#include <string_view>

#include "bindings/bus.hpp"

namespace DBusMock::Mocks
{
    class interface_mock_base
    {
    private:
        Bindings::Bus& bus;
        std::string service;
        std::string path;
        std::string interface;

    public:
        interface_mock_base
        (
            Bindings::Bus& bus,
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
    private:
        Bindings::Bus& bus;
        std::string service;
        std::string path;
        std::string interface;

    public:
        interface_method_base
        (
            Bindings::Bus& bus,
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

        virtual ~interface_method_base() = default;
    };
}
