#pragma once

#include "sdbus_core.hpp"
#include "message.hpp"
#include "slot.hpp"
#include "property.hpp"
#include "bus_fwd.hpp"
#include "event_loop.hpp"
#include "async_context.hpp"
#include "basic_exposable_interface.hpp"
#include "detail/slot_holder.hpp"

#include <string_view>
#include <string>
#include <iostream>
#include <functional>
#include <memory>
#include <atomic>
#include <chrono>
#include <mutex>

extern "C" {
    int dbus_mock_signal_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
	int dbus_mock_async_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
}

namespace DBusMock
{
    class dbus
	{
	public:
		friend int ::dbus_mock_async_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

	public:
		/**
		 * @brief open_system_bus Open local system bus.
		 * @return
		 */
		friend dbus open_system_bus();

		/**
		 * @brief open_user_bus Open local user bus.
		 * @return
		 */
		friend dbus open_user_bus();

		/**
		 * @brief open_system_bus Opens the system bus on a remote ssh host.
		 * @param host
		 * @return
		 */
		friend dbus open_system_bus(std::string const& host);

		/**
		 * @brief open_system_bus_machine Opens system bus of machine.
		 * @param host
		 * @return
		 */
		friend dbus open_system_bus_machine(std::string const& machine);

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
		 * @brief call_method Calls a specific method
		 * @param service The service name.
		 * @param path The path in the service.
		 * @param interface The interface name under the path.
		 * @param method_name The method name of the interface.
		 * @param params The parameters to pass to the method.
		 * @param cb A callback called when the method call success
		 * @param fail A callback called when the method call fails.
		 * @param timeout A timeout. The call fails, if the timeout is reached
		 * @return A method call result
		 */
		template <typename CallbackSignature, typename... ParametersT>
		void call_method_async(
		    std::string_view service,
		    std::string_view path,
		    std::string_view interface,
		    std::string_view method_name,
		    std::function <CallbackSignature> const& cb,
		    std::function <void(message&, std::string const&)> const& fail,
		    std::chrono::microseconds timeout,
		    ParametersT const&... parameters // TODO: improvable for const char* and fundamentals
		)
		{
			using namespace std::string_literals;

			int r = 0;
			sd_bus_message* raw_handle{};
			{
				std::scoped_lock guard{sdbus_lock_};
				r = sd_bus_message_new_method_call(
				    bus_,
				    &raw_handle,
				    service.data(),
				    path.data(),
				    interface.data(),
				    method_name.data()
				);
				if (r < 0)
					throw std::runtime_error("could not create message call on bus: "s + strerror(-r));
			}

			// buildup message, nothing else will touch this, so locking here is not necessary.
			message sendable{raw_handle};

			// Append parameters
			(sendable.append(parameters), ...);

			sendable.release(); // the async context takes over later

			// set expect reply (always, since it can error out)
			r = sd_bus_message_set_expect_reply(raw_handle, 1);
			if (r < 0)
				throw std::runtime_error("could not set message reply expectation: "s + strerror(-r));


			{
				std::scoped_lock guard{sdbus_lock_};
				// call method
				sd_bus_slot* slot;
				auto* ac = async_slots_.insert(
				    std::unique_ptr <async_context_base> (new async_context <
				        CallbackSignature,
				        void(message&, std::string const&)
				    >(this, raw_handle, cb, fail))
				);
				r = sd_bus_call_async(bus_, &slot, raw_handle, dbus_mock_async_callback, ac, static_cast <uint64_t> (timeout.count()));
				ac->slot(slot);
			}

			if (r < 0)
				throw std::runtime_error("could not send message on bus: "s + strerror(-r));
		}

		/**
		 *  flushes the bus.
		 */
		void flush();

		/**
		 * @brief install_event_loop Install some event loop implementation into the bus connection.
		 *        Its automatically started, if not already so.
		 * @param esys
		 */
		void install_event_loop(std::unique_ptr <event_loop> esys);

		/**
		 * @brief event_loop retrieve the currently installed event loop
		 * @return A handle to the event loop.
		 */
		template <typename LoopType>
		LoopType* loop()
		{
			if (!event_loop_)
				return nullptr;
			else
				return dynamic_cast <LoopType*> (event_loop_.get());
		}

		/**
		 * @brief operator sd_bus * this is castable directly to the handle.
		 */
		explicit operator sd_bus*()
		{
			return bus_;
		}

		/**
		 * @brief Returns the internal held handle.
		 */
		sd_bus* handle()
		{
			return bus_;
		}

		/**
		 * @brief read_property Retrieves a single property
		 * @param service The service name.
		 * @param path The path in the service.
		 * @param interface The interface name under the path.
		 * @param property_name The name of the property to read
		 * @param prop The value to read into.
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

		/**
		 * @brief write_property Retrieves a single property
		 * @param service The service name.
		 * @param path The path in the service.
		 * @param interface The interface name under the path.
		 * @param property_name The name of the property to write.
		 * @param prop The value to write
		 * @return nothing
		 */
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
			msg.append_variant(prop);

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

		/**
		 * @brief call_method Reads a property asynchronously
		 * @param service The service name.
		 * @param path The path in the service.
		 * @param interface The interface name under the path.
		 * @param property_name The method name of the property.
		 * @param cb A callback called when the method call success
		 * @param fail A callback called when the method call fails.
		 * @param timeout A timeout. The call fails, if the timeout is reached
		 * @return A method call result
		 */
		template <typename T>
		void read_property_async(
		    std::string_view service,
		    std::string_view path,
		    std::string_view interface,
		    std::string_view property_name,
		    std::function <void(std::decay_t <T> const&)> const& cb,
		    std::function <void(message&, std::string const&)> const& fail,
		    std::chrono::microseconds timeout
		)
		{
			std::scoped_lock guard{sdbus_lock_};

			call_method_async(
			    service,
			    path,
			    "org.freedesktop.DBus.Properties",
			    "Get",
			    cb,
			    fail,
			    timeout,
			    interface.data(),
			    property_name.data()
			);
		}

		/**
		 * @brief call_method Writes a property asynchronously
		 * @param service The service name.
		 * @param path The path in the service.
		 * @param interface The interface name under the path.
		 * @param property_name The method name of the property.
		 * @param cb A callback called when the method call success
		 * @param fail A callback called when the method call fails.
		 * @param timeout A timeout. The call fails, if the timeout is reached
		 * @return A method call result
		 */
		template <typename T>
		void write_property_async(
		    std::string_view service,
		    std::string_view path,
		    std::string_view interface,
		    std::string_view property_name,
		    std::function <void(void)> const& cb,
		    std::function <void(message&, std::string const&)> const& fail,
		    std::chrono::microseconds timeout,
		    T const& prop
		)
		{
			std::scoped_lock guard{sdbus_lock_};

			call_method_async(
			    service,
			    path,
			    "org.freedesktop.DBus.Properties",
			    "Set",
			    cb,
			    fail,
			    timeout,
			    interface.data(),
			    property_name.data(),
			    make_variant(prop)
			);
		}

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
			    &dbus_mock_signal_callback,
			    slo
			);

			slo->reset(s);

			if (release_slot)
				return slo;
			else
				return nullptr;
		}

		/**
		 * @brief mutex Returns the mutex of the bus, which can be used to synchronize bus operation with
		 *              other sd_bus calls.
		 * @return A reference to the internal mutex.
		 */
		std::recursive_mutex& mutex();

		/**
		 *  Destroys and cleans up the bus connection gracefully.
		 */
		~dbus();

		/**
		 * @brief dbus A dbus connection cannot be copied
		 */
		dbus(dbus const&) = delete;

		/**
		 * @brief dbus A dbus connection cannot be copied
		 */
		dbus& operator=(dbus const&) = delete;

		/**
		 * @brief dbus A dbus connection cannot be moved
		 */
		dbus(dbus&&) = delete;

		/**
		 * @brief dbus A dbus connection cannot be moved
		 */
		dbus& operator=(dbus&&) = delete;

		int expose_interface(std::shared_ptr <basic_exposable_interface> exposable);

	private:
		/**
		 * @brief free_async_concext Removes an async context from the store. Dont use manually.
		 *        Do note, that if a call to this free, for some inexplicable reason, doesn't get made:
		 *        the async_contexts will get cleaned up on bus death.
		 * @param ac
		 */
		void free_async_context(async_context_base* ac);

		/**
		 * @brief Bus Constructor is private, because one is meant to use the factories.
		 * @param bus
		 */
		dbus(sd_bus* bus_);

	private:
		sd_bus* bus_;
		std::vector <std::unique_ptr <slot_base>> unnamed_slots_;
		std::vector <std::shared_ptr <basic_exposable_interface>> exposed_interfaces_;
		std::recursive_mutex sdbus_lock_;
		std::unique_ptr <event_loop> event_loop_;
		detail::slot_holder async_slots_;
	};

	dbus open_system_bus();
	dbus open_user_bus();
	dbus open_system_bus(std::string const& host);
	dbus open_system_bus_machine(std::string const& machine);
}
