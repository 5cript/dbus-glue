#pragma once

#include "dbus_interface_base.hpp"

namespace DBusGlue
{
    template <typename Signature>
    struct signal
	{
	};

	struct release_slot_t
	{
		bool do_release;
	};

	constexpr static auto release_slot = release_slot_t{true};

	/**
	 *	@brief This class is only for external interfaces that get adapted. Should probably called slot.
	 */
	template <typename... Args>
	class signal <void(Args...)>
	{
	public:
		auto* listen
		(
		    std::function <void(Args const&...)> cb,
		    std::function <void(message&, std::string const&)> const& err,
		    release_slot_t release = release_slot_t{false}
		)
		{
			return base_->install_signal_listener(
			    name_,
			    cb,
			    err,
			    release.do_release
			);
		}

		signal(Mocks::interface_mock_base* base, char const* name)
		    : base_{base}
		    , name_{name}
		{
		}

	private:
		Mocks::interface_mock_base* base_;
		char const* name_;
	};
}
