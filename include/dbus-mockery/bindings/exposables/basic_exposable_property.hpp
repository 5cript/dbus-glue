#pragma once

#include "../message.hpp"

namespace DBusMock
{
    enum class property_change_behaviour
	{
		always_constant, // property NEVER changes for the entire lifetime of the object.
		emits_change, // emits a change signal whenever changed
		emits_invalidation // emits a change signal, but without the new value
	};

	class basic_exposable_property
	{
	public:
		virtual sd_bus_vtable make_vtable_entry(std::size_t offset) const = 0;
		virtual int read(message& msg) = 0;
		virtual int write(message& msg) = 0;
		virtual ~basic_exposable_property() = default;
	};

}
