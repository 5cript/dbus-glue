#pragma once

#include <variant>
#include "../exposed_method.hpp"

namespace DBusMock::detail
{
    struct table_entry
	{
		std::variant <
		    std::monostate,
		    basic_exposed_method*
		> entry;

		table_entry(basic_exposed_method* method)
		    : entry{method}
		{}
	};
}
