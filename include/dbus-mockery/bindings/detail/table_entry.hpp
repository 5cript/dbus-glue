#pragma once

#include <variant>
#include "../exposables/basic_exposable_method.hpp"
#include "../exposables/basic_exposable_property.hpp"

namespace DBusMock::detail
{
    struct table_entry
	{
		std::variant <
		    std::monostate,
		    basic_exposable_method*,
		    basic_exposable_property*
		> entry;

		table_entry(basic_exposable_method* method)
		    : entry{method}
		{}

		table_entry(basic_exposable_property* property)
		    : entry{property}
		{}
	};
}
