#pragma once

#include <functional>

namespace DBusGlue::detail
{
    struct on_scope_exit
	{
		std::function <void()> func;

		on_scope_exit(std::function <void()> func)
		    : func{std::move(func)}
		{
		}

		~on_scope_exit()
		{
			func();
		}

		on_scope_exit(on_scope_exit const&) = delete;
		on_scope_exit(on_scope_exit&&) = delete;
		on_scope_exit& operator=(on_scope_exit const&) = delete;
		on_scope_exit& operator=(on_scope_exit&&) = delete;
	};
}
