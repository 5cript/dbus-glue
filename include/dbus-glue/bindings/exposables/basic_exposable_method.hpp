#pragma once

#include "../message.hpp"

namespace DBusGlue
{
    class basic_exposable_method
	{
	public:
		virtual sd_bus_vtable make_vtable_entry(std::size_t offset) const	= 0;
		virtual int call(message& msg) = 0;
		virtual ~basic_exposable_method() = default;
	};
}
