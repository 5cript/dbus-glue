#pragma once

#include "bindings/bus.hpp"
#include "bindings/exposed_method.hpp"

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <string_view>

namespace DBusMock
{
    struct exposed_method_factory
	{
		std::string method_name;
		std::string result_name;
		std::map <int, std::string> in_names;
		uint64_t flags;
		int nextParm()
		{
			int max = 0;
			for (auto const& i : in_names)
			{
				max = std::max(i.first, max);
			}
			return max + 1;
		}

		exposed_method_factory() = default;
	};

	namespace ExposeHelpers
	{
	    struct method_name_t
		{
			std::string_view name;
		};
		method_name_t name(std::string_view view)
		{
			return method_name_t{view};
		}
		struct result_name_t
		{
			std::string_view name;
		};
		result_name_t result(std::string_view view)
		{
			return result_name_t{view};
		}
		struct parameter_name_t
		{
			std::string_view name;
			int which;
		};
		parameter_name_t parameter(std::string_view view)
		{
			return parameter_name_t{view, -1};
		}
		parameter_name_t parameter(int which, std::string_view view)
		{
			return parameter_name_t{view, which};
		}
		template <typename T>
		struct as_t
		{
			T fptr;
		};
		template <typename T>
		as_t<T> as(T f)
		{
			return {f};
		}
		struct flags_t
		{
			uint64_t flags;
		};
		flags_t flags(uint64_t flag)
		{
			return flags_t{flag};
		}
	}

	namespace detail
	{
	    template <typename T>
	    struct decide_add_method
		{
		};

		template <typename T>
		struct decide_add_method <std::unique_ptr <exposed_method<T>>>
		{
			template <typename InterfaceT>
			static void add(InterfaceT* iface, std::unique_ptr <exposed_method<T>> method)
			{
				method->set_owner(iface);
				iface->add_method(std::move(method));
			}
		};
	}

	exposed_method_factory& operator<<(exposed_method_factory&& lhs, ExposeHelpers::method_name_t&& name)
	{
		lhs.method_name = name.name;
		return lhs;
	}

	exposed_method_factory& operator<<(exposed_method_factory& lhs, ExposeHelpers::result_name_t&& name)
	{
		lhs.result_name = name.name;
		return lhs;
	}

	exposed_method_factory& operator<<(exposed_method_factory& lhs, ExposeHelpers::flags_t&& flags)
	{
		lhs.flags = flags.flags;
		return lhs;
	}

	exposed_method_factory& operator<<(exposed_method_factory& lhs, ExposeHelpers::parameter_name_t&& name)
	{
		if (name.which == -1)
			lhs.in_names[name.which] = name.name;
		else
			lhs.in_names[lhs.nextParm()] = name.name;
		return lhs;
	}
	template <typename T>
	std::unique_ptr <exposed_method<T>> operator<<(exposed_method_factory& lhs, ExposeHelpers::as_t<T>&& as)
	{
		auto method = std::make_unique <exposed_method <T>>();
		method->method_name = std::move(lhs.method_name);
		method->out_name = std::move(lhs.result_name);
		for (auto const& [k, v] : lhs.in_names)
			method->in_names.emplace_back(std::move(v));
		method->func = as.fptr;
		method->flags = std::move(lhs.flags);
		return method;
	}

	template <typename InterfaceT, typename... List>
	std::shared_ptr <InterfaceT> make_interface(List&&... list)
	{
		auto shared = std::make_shared <InterfaceT>();
		(detail::decide_add_method <List>::add(shared.get(), std::move(list)), ...);

		return shared;
	}
}
