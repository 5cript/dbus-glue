#pragma once

#include <variant>
#include "../exposables/basic_exposable_method.hpp"
#include "../exposables/basic_exposable_property.hpp"
#include "../exposables/basic_exposable_signal.hpp"

namespace DBusMock::detail
{
    struct table_entry
	{
		std::variant <
		    std::monostate,
		    basic_exposable_method*,
		    basic_exposable_property*,
		    basic_exposable_signal*
		> entry;

		table_entry(basic_exposable_method* method)
		    : entry{method}
		{}

		table_entry(basic_exposable_property* property)
		    : entry{property}
		{}

		table_entry(basic_exposable_signal* signal)
		    : entry{signal}
		{}
	};
}
