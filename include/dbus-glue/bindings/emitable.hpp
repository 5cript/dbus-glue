#pragma once

#include "bus.hpp"
#include "exposable_interface.hpp"
#include "message.hpp"

#include "detail/dissect.hpp"

#include <tuple>

namespace DBusMock
{
    /**
	 * @brief The emitable struct
	 */

    template <typename type>
    class emitable { };


	template <typename R, typename... Parameters>
	class emitable <R(*)(Parameters...)>
	{
	public:
		using function_type = R(*)(Parameters...);

		emitable(exposable_interface* owner, std::string name)
		    : owner_{owner}
		    , name_{std::move(name)}
		{

		}

		void emit(Parameters&&... params)
		{
			using namespace	std::string_literals;

			auto* bus = owner_->bus();
			if (bus == nullptr)
				throw std::runtime_error("interface was not exposed");

			sd_bus_message* msg {nullptr};
			auto r =sd_bus_message_new_signal(bus, &msg, owner_->path().c_str(), owner_->service().c_str(), name_.c_str());

			message m{msg};
			(m.append(std::forward <Parameters&&>(params)), ...);

			if (r < 0)
				throw std::runtime_error("cannot create signal message: "s + strerror(-r));

			r = sd_bus_send(bus, msg, nullptr);
			if (r < 0)
				throw std::runtime_error("cannot emit signal: "s + strerror(-r));
		}

		std::string const& name() const
		{
			return name_;
		}

	private:
		exposable_interface* owner_;
		std::string name_;
	};
}
