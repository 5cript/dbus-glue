#pragma once

#include <tuple>

namespace DBusGlue::detail
{
    template <typename>
    struct function_dissect
	{
	};

	template <typename R, typename IFace, typename... Parameters>
	struct function_dissect <R(IFace::*)(Parameters...)>
	{
		using interface_type = IFace;
		using return_type = R;
		using parameters = std::tuple <Parameters...>;
	};

	template <typename R, typename... Parameters>
	struct function_dissect <R(*)(Parameters...)>
	{
		using return_type = R;
		using parameters = std::tuple <Parameters...>;
	};

	template <typename>
	struct member_dissect
	{
	};

	template <typename IFace, typename T>
	struct member_dissect <T IFace::*>
	{
		using interface_type = IFace;
		using member_type = T;
	};
}
