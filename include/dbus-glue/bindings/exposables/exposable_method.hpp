#pragma once

#include "../sdbus_core.hpp"
#include "../types.hpp"
#include "../message.hpp"
#include "basic_exposable_method.hpp"

#include "../detail/dissect.hpp"
#include "../detail/tuple_apply.hpp"
#include "../detail/tuple_parameter_decay.hpp"

#include <string>
#include <vector>

#include <iostream>

extern "C" {
    int dbus_mock_exposed_method_handler(sd_bus_message *m, void *userdata, sd_bus_error *);
}

namespace DBusMock
{
    namespace detail
	{
	    template <typename Tuple>
	    struct message_tuple_reader { };

		template <typename... TupleParams>
		struct message_tuple_reader <std::tuple <TupleParams...>>
		{
			static std::tuple <TupleParams...> exec(message& msg)
			{
				return std::tuple <TupleParams...> {
					msg.read_value <std::decay_t <TupleParams>>()...
				};
			}
		};
	}

	template <typename FunctionT>
	class exposable_method : public basic_exposable_method
	{
	public:
		using owner_type = typename detail::function_dissect <FunctionT>::interface_type;

	private:
		// will be generated, but must be stored to survive interface registration.
		mutable std::string signature_;
		mutable std::string result_signature_;
		mutable std::string io_name_combined_;
		mutable std::size_t offset_;
		owner_type* owner;

	public:
		// setable values from related functions
		std::string method_name;
		std::string out_name;
		std::vector <std::string> in_names;
		uint64_t flags;
		FunctionT func;

		// methods
		exposable_method() = default;
		~exposable_method() = default;

		exposable_method(exposable_method const&) = default;
		exposable_method& operator=(exposable_method const&) = default;
		exposable_method(exposable_method&&) = default;
		exposable_method& operator=(exposable_method&&) = default;

		void set_owner(owner_type* own)
		{
			owner = own;
		}

		template <typename... StringTypes>
		exposable_method
		(
		    std::string method_name,
		    std::string out_name,
		    StringTypes&&... in_names
		)
		    : method_name{std::move(method_name)}
		    , out_name{std::move(out_name)}
		    , in_names{std::forward <StringTypes&&>(in_names)...} // move on ptr is copy
		    , func{nullptr}
		{
		}

	private:
		void prepare_for_expose() const
		{
			io_name_combined_.clear();
			result_signature_.clear();
			signature_.clear();

			signature_ = detail::vector_flatten(detail::tuple_apply <
			    typename detail::function_dissect <FunctionT>::parameters,
			    detail::argument_signature_factory
			>::build());
			result_signature_ = detail::vector_flatten(detail::tuple_apply <
			    std::tuple <typename detail::function_dissect <FunctionT>::return_type>,
			    detail::argument_signature_factory
			>::build());

			for (auto const& i : in_names)
			{
				io_name_combined_ += i;
				io_name_combined_.push_back('\0');
			}
			if constexpr (!std::is_same_v <typename detail::function_dissect <FunctionT>::return_type, void>)
			{
				io_name_combined_ += out_name;
				io_name_combined_.push_back('\0');
			}
		}

	public:
		int call(message& msg) override
		{
			using tuple_type = typename detail::tuple_parameter_decay <
			    typename detail::function_dissect <FunctionT>::parameters
			>::type;

			auto res_tuple = detail::message_tuple_reader <tuple_type>::exec(msg);

			if constexpr (!std::is_same_v <typename detail::function_dissect <FunctionT>::return_type, void>)
			{
				return sd_bus_reply_method_return
				(
				    msg.handle(),
				    result_signature_.c_str(),
				    std::apply([this](auto&&... params){
					    return (owner->*func)(std::forward <decltype(params)> (params)...);
				    }, res_tuple)
				);
			}
			else
			{
				std::apply([this](auto&&... params){
					return (owner->*func)(std::forward <decltype(params)> (params)...);
				}, res_tuple);
				sd_bus_reply_method_return(msg.handle(), result_signature_.c_str());
			}
			return 0;
		}

		sd_bus_vtable make_vtable_entry(std::size_t offset) const
		{
			prepare_for_expose();
			offset_ = offset;

			return SD_BUS_METHOD_WITH_NAMES_OFFSET(
			    method_name.c_str(),
			    signature_.c_str(), ,
			    result_signature_.c_str(), io_name_combined_.data(),
			    &dbus_mock_exposed_method_handler,
			    offset_,
			    flags
			);
			/*
			// intentionally disregarding plea to use macros.
			sd_bus_vtable vt;
			memset(&vt, 0, sizeof(vt));
			vt.type = static_cast <decltype(vt.type)> (_SD_BUS_VTABLE_METHOD);
			vt.flags = flags;
			decltype(vt.x.method) method;
			method.member = method_name.c_str();

			method.signature = signature.c_str();
			method.result = result_signature.c_str();
			method.handler = &dbus_mock_exposed_method_handler;
			method.offset = 0;
			method.names = io_name_combined.c_str();
			vt.x.method = method;

			return vt;
			*/
		}
	};
}
