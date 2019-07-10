#pragma once

#include "sdbus_core.hpp"
#include "message.hpp"

#include <string_view>
#include <string>

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
		    ParametersT const&... parameters
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
