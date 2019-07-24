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
#include <mutex>

namespace DBusMock::Bindings
{
    extern "C" {
        static int generic_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
        {
            auto* base = reinterpret_cast<slot_base*>(userdata);
            message msg{m};
            try
            {
                if (ret_error != nullptr && sd_bus_error_is_set(ret_error))
                {
                    base->on_fail(msg, ret_error->message);
                    sd_bus_error_free(ret_error);
                }
                else
                    base->unpack_message(msg);
                msg.release();
                return 1;
            }
            catch (std::exception const& exc)
            {
                base->on_fail(msg, exc.what());
                msg.release();
                return -1;
            }
            catch (...)
            {
                msg.release();
                return -1;
            }
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
        void busy_loop(std::atomic <bool>* running);

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
            std::scoped_lock guard{sdbus_lock_};

            using namespace std::string_literals;

            sd_bus_message* raw_handle{};
            auto r = sd_bus_message_new_method_call(
                bus_,
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
            r = sd_bus_call(bus_, sendable.handle(), 0, &error, &reply_handle);

            // convert error into throwable if set.
            if (sd_bus_error_is_set(&error))
                throw std::runtime_error(error.message);
             sd_bus_error_free(&error);

            if (r < 0)
                throw std::runtime_error("could not send message on bus: "s + strerror(-r));

            return message{reply_handle};
        }

        /**
         *  flushes the bus.
         */
        void flush();

        /**
         * @brief attach_event_system Attaches the bus to an event system.
         *        This is used for more complex event loops.
         * @see https://www.freedesktop.org/software/systemd/man/sd-event.html
         * @param e An sd_event. You have to create and provide it yourself, look up docs for sd_bus_attach_event()
         * @param priority. A priority. Valid parameters are:
         *  SD_EVENT_PRIORITY_IMPORTANT
         *  SD_EVENT_PRIORITY_NORMAL
         *  SD_EVENT_PRIORITY_IDLE
         */
        void attach_event_system(sd_event* e, int priority = SD_EVENT_PRIORITY_NORMAL);

        /**
         * @brief detach_event_system Detaches the bus from an event system.
         * Since its not documented in sd_bus, I dont know what happens if you call it without it having an attached
         * event system.
         */
        void detach_event_system();

        /**
         *  Returns the attached event system, if any.
         */
        sd_event* get_event_system();

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
            std::scoped_lock guard{sdbus_lock_};

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
            std::scoped_lock guard{sdbus_lock_};

            using namespace std::string_literals;

            sd_bus_message* m{};
            auto r = sd_bus_message_new_method_call(
                bus_,
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

            r = sd_bus_call(bus_, m, 0, &error, &reply);

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
            std::scoped_lock guard{sdbus_lock_};

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
         * @param func The function that is called on an emitted signal
         * @param fail A function that is called when an error occured
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
            std::function <FunctionT> const& func,
            std::function <void(message&, std::string const&)> const& fail /* called in error case */,
            bool release_slot = false
        )
        {
            std::scoped_lock guard{sdbus_lock_};

            slot <FunctionT>* slo;
            if (!release_slot)
            {
                unnamed_slots_.emplace_back(
                    new slot <FunctionT> {func, fail}
                );
                slo = static_cast <slot <FunctionT>*>(unnamed_slots_.rbegin()->get());
            }
            else
            {
                slo = new slot <FunctionT>(func, fail);
            }

            sd_bus_slot* s;
            sd_bus_match_signal
            (
                bus_,
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
        Bus(sd_bus* bus_);

    private:
        sd_bus* bus_;
        std::vector <std::unique_ptr <slot_base>> unnamed_slots_;
        std::recursive_mutex sdbus_lock_;
    };

    Bus open_system_bus();
    Bus open_user_bus();
    Bus open_system_bus(std::string const& host);
    Bus open_system_bus_machine(std::string const& machine);
}
