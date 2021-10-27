#pragma once

#include "../sdbus_core.hpp"
//#include "../message.hpp"

#include <cstdint>

namespace DBusGlue
{
    class basic_exposable_signal
	{
	public:
		virtual sd_bus_vtable make_vtable_entry(std::size_t offset) const	= 0;
		virtual ~basic_exposable_signal() = default;
	};
}
