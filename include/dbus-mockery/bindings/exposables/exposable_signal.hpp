#pragma once

#include "basic_exposable_signal.hpp"
#include "../sdbus_core.hpp"
#include "../types.hpp"
#include "../message.hpp"


#include "../detail/dissect.hpp"
#include "../detail/tuple_apply.hpp"
#include "../detail/tuple_parameter_decay.hpp"

#include <vector>
#include <string>

namespace DBusMock
{
    template <typename FunctionT>
    class exposable_signal : public basic_exposable_signal
	{
	public:
		using owner_type = typename detail::method_dissect <FunctionT>::interface_type;

	private:
		// will be generated, but must be stored to survive interface registration.
		mutable std::string signature_;
		mutable std::string io_name_combined_;

	public:
		// setable values from related functions
		std::string signal_name;
		std::vector <std::string> in_names;
		uint64_t flags;

	private:
		void prepare_for_expose() const
		{
			io_name_combined_.clear();
			signature_.clear();

			signature_ = detail::vector_flatten(detail::tuple_apply <
			    typename detail::method_dissect <FunctionT>::parameters,
			    detail::argument_signature_factory
			>::build());

			for (auto const& i : in_names)
			{
				io_name_combined_ += i;
				io_name_combined_.push_back('\0');
			}
		}
	public:

		// methods
		exposable_signal() = default;
		~exposable_signal() = default;

		exposable_signal(exposable_signal const&) = default;
		exposable_signal& operator=(exposable_signal const&) = default;
		exposable_signal(exposable_signal&&) = default;
		exposable_signal& operator=(exposable_signal&&) = default;

		template <typename... StringTypes>
		exposable_signal
		(
		    std::string signal_name,
		    StringTypes&&... in_names
		)
		    : signal_name{std::move(signal_name)}
		    , in_names{std::forward <StringTypes&&>(in_names)...} // move on ptr is copy
		    , flags{0}
		{
		}

		sd_bus_vtable make_vtable_entry(std::size_t offset) const
		{
			prepare_for_expose();

			return SD_BUS_SIGNAL_WITH_NAMES(
			    signal_name.c_str(),
			    signature_.c_str(),
			    io_name_combined_.c_str(),
			    flags
			);
		}
	};
}
