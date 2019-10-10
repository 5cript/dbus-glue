#pragma once

#include "bindings/bus.hpp"
#include "bindings/exposable_interface.hpp"
#include "bindings/exposables/exposable_method.hpp"
#include "bindings/exposables/exposable_property.hpp"
#include "bindings/exposables/exposable_signal.hpp"
#include "bindings/emitable.hpp"

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <string_view>

namespace DBusMock
{
    struct factory_for_parameterized_members
	{
		std::string name;
		std::map <int, std::string> in_names;
		uint64_t flags;

		factory_for_parameterized_members() = default;

		int nextParm()
		{
			int max = 0;
			for (auto const& i : in_names)
			{
				max = std::max(i.first, max);
			}
			return max + 1;
		}
	};

	struct exposable_method_factory
	    : factory_for_parameterized_members
	{
		std::string result_name;

		exposable_method_factory() = default;
	};

	struct exposable_signal_factory
	    : factory_for_parameterized_members
	{
		exposable_signal_factory() = default;
	};

	struct exposable_property_factory
	{
		std::string name = "";
		bool writeable = false;
		property_change_behaviour change_behaviour = property_change_behaviour::emits_change;

		exposable_property_factory() = default;
	};

	namespace ExposeHelpers
	{
	    struct member_name_t
		{
			std::string_view name;
		};
		member_name_t name(std::string_view view)
		{
			return member_name_t{view};
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
			T mem;
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
		flags_t flags(property_change_behaviour prop_change)
		{
			return flags_t{static_cast <uint64_t> (prop_change)};
		}
		struct writeable_t
		{
			bool writeable;
		};
		writeable_t writeable(bool writeable)
		{
			return writeable_t{writeable};
		}
	}

	namespace detail
	{
	    template <typename T>
	    struct decide_add_method
		{
		};

		template <typename T>
		struct decide_add_method <std::unique_ptr <exposable_method<T>>>
		{
			template <typename InterfaceT>
			static void add(InterfaceT* iface, std::unique_ptr <exposable_method<T>>&& method)
			{
				method->set_owner(iface);
				iface->add_method(std::move(method));
			}
		};

		template <typename T>
		struct decide_add_method <std::unique_ptr <exposable_property<T>>>
		{
			template <typename InterfaceT>
			static void add(InterfaceT* iface, std::unique_ptr <exposable_property<T>>&& property)
			{
				property->set_owner(iface);
				iface->add_property(std::move(property));
			}
		};

		template <typename T>
		struct decide_add_method <std::unique_ptr <exposable_signal<T>>>
		{
			template <typename InterfaceT>
			static void add(InterfaceT* iface, std::unique_ptr <exposable_signal<T>>&& signal)
			{
				signal->signal_name = (iface->*(signal->ptr)).name();
				//signal->set_owner(iface);
				iface->add_signal(std::move(signal));
			}
		};
	}

	exposable_property_factory& operator<<(exposable_property_factory&& lhs, ExposeHelpers::member_name_t&& name)
	{
		lhs.name = name.name;
		return lhs;
	}

	exposable_property_factory& operator<<(exposable_property_factory& lhs, ExposeHelpers::member_name_t&& name)
	{
		lhs.name = name.name;
		return lhs;
	}

	exposable_property_factory& operator<<(exposable_property_factory& lhs, ExposeHelpers::flags_t&& flags)
	{
		lhs.change_behaviour = static_cast <property_change_behaviour> (flags.flags);
		return lhs;
	}

	exposable_property_factory& operator<<(exposable_property_factory&& lhs, ExposeHelpers::flags_t&& flags)
	{
		lhs.change_behaviour = static_cast <property_change_behaviour> (flags.flags);
		return lhs;
	}

	exposable_property_factory& operator<<(exposable_property_factory& lhs, ExposeHelpers::writeable_t&& writeable)
	{
		lhs.writeable = writeable.writeable;
		return lhs;
	}

	exposable_property_factory& operator<<(exposable_property_factory&& lhs, ExposeHelpers::writeable_t&& writeable)
	{
		lhs.writeable = writeable.writeable;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory&& lhs, ExposeHelpers::member_name_t&& name)
	{
		lhs.name = name.name;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory& lhs, ExposeHelpers::member_name_t&& name)
	{
		lhs.name = name.name;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory& lhs, ExposeHelpers::result_name_t&& name)
	{
		lhs.result_name = name.name;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory&& lhs, ExposeHelpers::result_name_t&& name)
	{
		lhs.result_name = name.name;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory& lhs, ExposeHelpers::flags_t&& flags)
	{
		lhs.flags = flags.flags;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory&& lhs, ExposeHelpers::flags_t&& flags)
	{
		lhs.flags = flags.flags;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory& lhs, ExposeHelpers::parameter_name_t&& name)
	{
		if (name.which == -1)
			lhs.in_names[name.which] = name.name;
		else
			lhs.in_names[lhs.nextParm()] = name.name;
		return lhs;
	}

	exposable_method_factory& operator<<(exposable_method_factory&& lhs, ExposeHelpers::parameter_name_t&& name)
	{
		if (name.which == -1)
			lhs.in_names[name.which] = name.name;
		else
			lhs.in_names[lhs.nextParm()] = name.name;
		return lhs;
	}

	exposable_signal_factory& operator<<(exposable_signal_factory& lhs, ExposeHelpers::flags_t&& flags)
	{
		lhs.flags = flags.flags;
		return lhs;
	}

	exposable_signal_factory& operator<<(exposable_signal_factory&& lhs, ExposeHelpers::flags_t&& flags)
	{
		lhs.flags = flags.flags;
		return lhs;
	}

	exposable_signal_factory& operator<<(exposable_signal_factory& lhs, ExposeHelpers::parameter_name_t&& param)
	{
		if (param.which == -1)
			lhs.in_names[param.which] = param.name;
		else
			lhs.in_names[lhs.nextParm()] = param.name;
		return lhs;
	}

	exposable_signal_factory& operator<<(exposable_signal_factory&& lhs, ExposeHelpers::parameter_name_t&& param)
	{
		if (param.which == -1)
			lhs.in_names[param.which] = param.name;
		else
			lhs.in_names[lhs.nextParm()] = param.name;
		return lhs;
	}

	template <typename T>
	std::unique_ptr <exposable_method<T>> operator<<(exposable_method_factory& lhs, ExposeHelpers::as_t<T>&& as)
	{
		auto method = std::make_unique <exposable_method <T>>();
		method->method_name = std::move(lhs.name);
		method->out_name = std::move(lhs.result_name);
		for (auto const& [k, v] : lhs.in_names)
			method->in_names.emplace_back(std::move(v));
		method->func = as.mem;
		method->flags = std::move(lhs.flags);
		return method;
	}

	template <typename T>
	std::unique_ptr <exposable_property<T>> operator<<(exposable_property_factory& lhs, ExposeHelpers::as_t<T>&& as)
	{
		auto prop = std::make_unique <exposable_property <T>>();
		prop->name = std::move(lhs.name);
		prop->property = as.mem;
		prop->change_behaviour = std::move(lhs.change_behaviour);
		prop->writeable = lhs.writeable;
		return prop;
	}

	template <typename T>
	std::unique_ptr <exposable_signal<T>> operator<<(exposable_signal_factory& lhs, ExposeHelpers::as_t<T>&& as)
	{
		auto signal = std::make_unique <exposable_signal <T>>();
		//signal->signal_name = std::move(lhs.name);
		signal->ptr = as.mem;
		for (auto const& [k, v] : lhs.in_names)
			signal->in_names.emplace_back(std::move(v));
		signal->flags = std::move(lhs.flags);
		return signal;
	}

	template <typename InterfaceT, typename... List>
	void construct_interface(InterfaceT* iface, List&&... list)
	{
		(detail::decide_add_method <List>::add(iface, std::move(list)), ...);
	}

	template <typename InterfaceT, typename... List>
	std::shared_ptr <InterfaceT> make_interface(List&&... list)
	{
		auto shared = std::make_shared <InterfaceT>();
		construct_interface(shared.get(), std::forward <List&&>(list)...);
		return shared;
	}
}
