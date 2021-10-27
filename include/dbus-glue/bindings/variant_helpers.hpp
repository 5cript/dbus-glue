#pragma once

#include "types.hpp"
#include "message.hpp"
#include "bus.hpp"

#include <iostream>

namespace DBusGlue
{
    class message_variant_resolver
	{
	public:
		template <typename T>
		static void resolve(message_variant& var, T& val)
		{
			var.message_->read <T>(val);
		}

		static type_descriptor type(message_variant& var)
		{
			return var.message_->type();
		}

		template <typename T>
		static void store(dbus& bus, message_variant& var, T const& val)
		{
			var.message_.reset(new message(static_cast <sd_bus*> (bus), 0));
			var.message_->append_variant(val);
			var.message_->seal();
			var.rewind(true);
		}
	};

	template <typename T>
	message_variant make_variant(DBusGlue::dbus& bus, T&& val)
	{
		message_variant v;
		message_variant_resolver::store(bus, v, val);
		return v;
	}

	template <typename T>
	void resolve(resolvable_variant var, T& val)
	{
		var.resolve(val);
	}

	template <typename T>
	void resolve(message_variant& var, T& val, bool complete_rewind = false)
	{
		message_variant_resolver::resolve(var, val);
		var.rewind(complete_rewind);
	}

	template <typename T>
	void variant_load(message_variant& var, T& val, bool complete_rewind = false)
	{
		resolve(var, val, complete_rewind);
	}

	type_descriptor variant_type(message_variant& var)
	{
		return message_variant_resolver::type(var);
	}

	template <typename T>
	void variant_store(dbus& bus, message_variant& var, T const& value)
	{
		message_variant_resolver::store(bus, var, value);
	}
}
