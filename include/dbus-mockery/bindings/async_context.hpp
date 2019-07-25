#pragma once

#include "bus_fwd.hpp"
#include "sdbus_core.hpp"
#include "function_wrap.hpp"

#include <functional>

namespace DBusMock::Bindings
{
    class async_context_base
	{
	public:
		virtual ~async_context_base() = default;

		virtual void slot(sd_bus_slot* slot) = 0;
		virtual sd_bus_slot* slot() = 0;
		virtual void unpack_message(message& msg) = 0;
		virtual void on_fail(message&, std::string const&) = 0;
		virtual dbus* owner() = 0;

	};

	/**
	 * @brief The async_context class is used to manage asynchronous callbacks and their lifetimes.
	 */
	template <typename CallbackSignatureT, typename ErrorCallbackSignatureT>
	class async_context : public async_context_base
	{
	public:
		async_context
		(
		    dbus* owner,
		    sd_bus_message* sent_msg,
		    std::function <CallbackSignatureT> cb,
		    std::function <ErrorCallbackSignatureT> err
		)
		    : owner_{owner}
		    , sent_msg_{sent_msg}
		    , cb_{std::move(cb)}
		    , err_{std::move(err)}
		{
		}
		~async_context()
		{
			sd_bus_message_unref(sent_msg_);
			sd_bus_slot_unref(slot_);
		}
		void slot(sd_bus_slot* slot)
		{
			slot_ = slot;
		}
		sd_bus_slot* slot()
		{
			return slot_;
		}
		dbus* owner()
		{
			return owner_;
		}

		void unpack_message(message& msg) override
		{
			cb_.unpack_message(msg);
		}

		void on_fail(message& msg, std::string const& error_message) override
		{
			err_(msg, error_message);
		}

	private:
		dbus* owner_;
		sd_bus_slot* slot_;
		sd_bus_message* sent_msg_;
		function_wrapper <CallbackSignatureT> cb_;
		std::function <ErrorCallbackSignatureT> err_;
	};
}
