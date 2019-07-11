#pragma once

#include "sdbus_core.hpp"
#include "message.hpp"
#include "property.hpp"

#include <string_view>
#include <string>
#include <iostream>

namespace DBusMock::Bindings
{
    class Bus
    {
    public:
        /**
         * @brief open_system_bus Open local system bus.
         * @return
         */
        friend Bus open_system_bus();

        /**
         * @brief open_user_bus Open local user bus.
         * @return
         */
        friend Bus open_user_bus();

        /**
         * @brief open_system_bus Opens the system bus on a remote ssh host.
         * @param host
         * @return
         */
        friend Bus open_system_bus(std::string const& host);

        /**
         * @brief open_system_bus_machine Opens system bus of machine.
         * @param host
         * @return
         */
        friend Bus open_system_bus_machine(std::string const& machine);

        /**
         * @brief call_method Calls a specific method
         * @param service The service name.
         * @param path The path in the service.
         * @param interface The interface name under the path.
         * @param method_name The method name of the interface.
         * @return A method call result
         */
        template <typename... ParametersT>
        message call_method(
            std::string_view service,
            std::string_view path,
            std::string_view interface,
            std::string_view method_name,
            ParametersT const&... parameters // TODO: improvable for const char* and fundamentals
        )
        {
            sd_bus_message* msg;

            auto error = SD_BUS_ERROR_NULL;
            auto r = sd_bus_call_method(
                bus,
                service.data(),
                path.data(),
                interface.data(),
                method_name.data(),
                &error,
                &msg,
                type_constructor <ParametersT...>::make_type().c_str(),
                parameters...
            );

            if (r < 0)
            {
                std::string msg = error.message;
                sd_bus_error_free(&error);
                using namespace std::string_literals;
                throw std::runtime_error("cannot call method: "s + msg);
            }

            sd_bus_error_free(&error);

            return message{msg};
        }

        /**
         * @brief call_method Retrieves a single property
         * @param service The service name.
         * @param path The path in the service.
         * @param interface The interface name under the path.
         * @param prop The name of the property to read
         * @return nothing
         */
        template <typename T>
        void read_property(
            std::string_view service,
            std::string_view path,
            std::string_view interface,
            std::string_view property_name,
            T& prop
        )
        {
            auto message = call_method(
                service,
                path,
                "org.freedesktop.DBus.Properties",
                "Get",
                interface.data(),
                property_name.data()
            );

            message.read(prop);
        }

        /**
         * @brief call_method Uses a variadic template paramter list of concrete types for that.
         * @param service The service name.
         * @param path The path in the service.
         * @param interface The interface name under the path.
         * @param prop The name of the property to read
         * @return nothing
         */
        template <template <typename...> MapType>
        void read_properties(
            std::string_view service,
            std::string_view path,
            std::string_view interface,
            variant_dictionary <MayType> dict
        )
        {
            auto message = call_method(
                service,
                path,
                "org.freedesktop.DBus.Properties",
                "GetAll",
                interface.data()
            );

            message.read(dict);
        }

        ~Bus();

        Bus(Bus const&) = delete;
        Bus(Bus&&) = delete;
        Bus& operator=(Bus&&) = delete;
        Bus& operator=(Bus const&) = delete;

    private:
        /**
         * @brief Bus Constructor is private, because one is meant to use the factories.
         * @param bus
         */
        Bus(sd_bus* bus);

    private:
        sd_bus* bus;
    };

    Bus open_system_bus();
    Bus open_user_bus();
    Bus open_system_bus(std::string const& host);
    Bus open_system_bus_machine(std::string const& machine);
}
