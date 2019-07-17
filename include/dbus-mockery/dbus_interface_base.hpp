#pragma once

#include <string>
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
        interface_mock_base(
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
        virtual ~interface_mock_base() = default;
    };
}
