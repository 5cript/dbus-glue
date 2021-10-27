#pragma once

#include "message.hpp"

#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>

namespace DBusMock
{
    template <typename FunctionT>
    class function_wrapper
	{
	};

	template <typename R, typename... List>
	class function_wrapper <R(List...)>
	{
		public:
		using return_type = R;
		using signature = R(List...);

		public:
		function_wrapper(std::function <signature> f)
		    : func_{std::move(f)}
		{
		}

		template <typename... Args>
		return_type operator()(Args&&... args)
		{
			return func_(std::forward <Args&&> (args)...);
		}

		void unpack_message(message& msg)
		{
			func_(std::move(read_single <List>(msg))...);
		}

		private:
		template <typename T>
		std::decay_t <T> read_single(message& msg)
		{
			std::decay_t <T> v;
			msg.read(v);
			return v;
		}

		private:
		std::function <signature> func_;
	};
}
