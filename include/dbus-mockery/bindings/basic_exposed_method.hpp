#pragma once

#include "message.hpp"

namespace DBusMock
{
    class basic_exposed_method
	{
	public:
		virtual sd_bus_vtable make_vtable_entry(std::size_t offset) const	= 0;
		virtual int call(message& msg) = 0;
		virtual ~basic_exposed_method() = default;
	};
}
