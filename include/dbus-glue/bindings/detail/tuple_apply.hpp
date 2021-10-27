#pragma once

#include <tuple>

namespace DBusGlue::detail
{
    template <typename Tuple, template <typename...> class Function>
    struct tuple_apply
	{ };

	template <template <typename...> class Function, typename... List>
	struct tuple_apply <std::tuple <List...>, Function>
	    : Function <List...>
	{
	};
}
