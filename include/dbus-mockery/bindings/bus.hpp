#pragma once

#include "sdbus_core.hpp"
#include "message.hpp"
#include "slot.hpp"
#include "property.hpp"

#include <string_view>
#include <string>
#include <iostream>
#include <functional>
#include <memory>
#include <atomic>

namespace DBusMock::Bindings
{
    extern "C" {
        static int generic_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
        {
            std::cout << "hello from callback";
            auto* base = reinterpret_cast<slot_base*>(userdata);
            message msg{m};
            //base->pass_message_to_slot<>(msg, base, base->signature());
            base->unpack_message(msg);

            // dont aquire ownership, the message is lend.
            msg.release();
            return 0;
        }
    }

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
         * Enter a busy, blocking event loop, as long as "running is true".
         * Does not set running to true by itself.
         */
        void busy_loop(std::atomic <bool>& running);

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
            using namespace std::string_literals;

            sd_bus_message* raw_handle{};
            auto r = sd_bus_message_new_method_call(
                bus,
                &raw_handle,
                service.data(),
                path.data(),
                interface.data(),
                method_name.data()
            );
            if (r < 0)
                throw std::runtime_error("could not create message call on bus: "s + strerror(-r));

            message sendable{raw_handle};

            // Append parameters
            (sendable.append(parameters), ...);

            // sd_bus_call passables
            auto error = SD_BUS_ERROR_NULL;
            sd_bus_message* reply_handle;

            // set expect reply (always, since it can error out)
            r = sd_bus_message_set_expect_reply(sendable.handle(), 1);
            if (r < 0)
                throw std::runtime_error("could not set message reply expectation: "s + strerror(-r));

            // call method
            r = sd_bus_call(bus, sendable.handle(), 0, &error, &reply_handle);

            // convert error into throwable if set.
            if (sd_bus_error_is_set(&error))
                throw std::runtime_error(error.message);
             sd_bus_error_free(&error);

            if (r < 0)
                throw std::runtime_error("could not send message on bus: "s + strerror(-r));

            return message{reply_handle};
        }

        /**
         * @brief call_method Calls a specific method
         * @param service The service name.
         * @param path The path in the service.
         * @param interface The interface name under the path.
         * @param method_name The method name of the interface.
         * @return A method call result
         */
        template <typename... ParametersT>
        [[deprecated]] message call_method_simple(
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

        template <typename T>
        void write_property(
            std::string_view service,
            std::string_view path,
            std::string_view interface,
            std::string_view property_name,
            T const& prop
        )
        {
            using namespace std::string_literals;

            sd_bus_message* m{};
            auto r = sd_bus_message_new_method_call(
                bus,
                &m,
                service.data(),
                path.data(),
                "org.freedesktop.DBus.Properties",
                "Set"
            );
            if (r < 0)
                throw std::runtime_error("could not create message call on bus: "s + strerror(-r));

            message msg{m};
            msg.append(interface.data());
            msg.append(property_name.data());
            msg.append(prop);

            auto error = SD_BUS_ERROR_NULL;
            sd_bus_message* reply{};

            // frees handle on scope exit
            make_basic_message_handle(reply);

            r = sd_bus_message_set_expect_reply(m, 1);
            if (r < 0)
                throw std::runtime_error("could not set message reply expectation: "s + strerror(-r));

            r = sd_bus_call(bus, m, 0, &error, &reply);

            if (sd_bus_error_is_set(&error))
                throw std::runtime_error(error.message);

             sd_bus_error_free(&error);

            if (r < 0)
                throw std::runtime_error("could not send message on bus: "s + strerror(-r));
        }

        /**
         * @brief call_method Uses a variadic template paramter list of concrete types for that.
         * @param service The service name.
         * @param path The path in the service.
         * @param interface The interface name under the path.
         * @param prop The name of the property to read
         * @return nothing
         */
        template <template <typename...> typename MapType, typename... Remain>
        void read_properties(
            std::string_view service,
            std::string_view path,
            std::string_view interface,
            variant_dictionary <MapType, Remain...>& dict
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

    public:
        /**
         * @brief install_signal_listener Installs a listener for a signal.
         * @param service The service name.
         * @param path The path in the service.
         * @param interface The interface name under the path.
         * @param signal The name of the signal to listen to.
         * @param release_slot: Gives the connected slot object back to the caller, who then has to manage its lifetime.
         *        It has to be free'd with delete (means: pass it to a unique_ptr please).
         */
        template <typename FunctionT>
        slot <FunctionT>* install_signal_listener
        (
            std::string_view service,
            std::string_view path,
            std::string_view interface,
            std::string_view signal,
            std::function <FunctionT> func,
            bool release_slot = false
        )
        {
            slot <FunctionT>* slo;
            if (!release_slot)
            {
                unnamed_slots.emplace_back(
                    new slot <FunctionT> {func}
                );
                slo = static_cast <slot <FunctionT>*>(unnamed_slots.rbegin()->get());
            }
            else
            {
                slo = new slot <FunctionT>(func);
            }

            sd_bus_slot* s;
            sd_bus_match_signal
            (
                bus,
                &s,
                service.data(),
                path.data(),
                interface.data(),
                signal.data(),
                &generic_callback,
                slo
            );

            slo->reset(s);

            if (release_slot)
                return slo;
            else
                return nullptr;
        }

        ~Bus();

        Bus(Bus const&) = delete;
        Bus& operator=(Bus const&) = delete;
        Bus(Bus&&) = default;
        Bus& operator=(Bus&&) = default;

    private:
        /**
         * @brief Bus Constructor is private, because one is meant to use the factories.
         * @param bus
         */
        Bus(sd_bus* bus);

    private:
        sd_bus* bus;
        std::vector <std::unique_ptr <slot_base>> unnamed_slots;
    };

    Bus open_system_bus();
    Bus open_user_bus();
    Bus open_system_bus(std::string const& host);
    Bus open_system_bus_machine(std::string const& machine);
}
