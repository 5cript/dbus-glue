#pragma once

#include <tuple>

namespace DBusGlue::detail
{
    template <typename Tuple>
    struct tuple_parameter_decay
	{ };

	template <typename... TupleParams>
	struct tuple_parameter_decay <std::tuple <TupleParams...>>
	{
		using type = std::tuple <std::decay_t <TupleParams>...>;
	};
}
