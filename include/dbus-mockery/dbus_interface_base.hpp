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
}
