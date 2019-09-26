#pragma once

#include <tuple>

namespace DBusMock::detail
{
    template <typename>
    struct method_dissect
	{
	};

	template <typename R, typename IFace, typename... Parameters>
	struct method_dissect <R(IFace::*)(Parameters...)>
	{
		using interface_type = IFace;
		using return_type = R;
		using parameters = std::tuple <Parameters...>;
	};
}
