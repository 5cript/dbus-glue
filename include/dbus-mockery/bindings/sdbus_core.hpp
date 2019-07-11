#pragma once

#include <systemd/sd-bus.h>

namespace DBusMock::Bindings
{
    class message;

    /**
     *
     */
    template <typename T, typename FuncT>
    class basic_handle_wrapper
    {
    public:
        basic_handle_wrapper(T* handle, FuncT incrementCount, FuncT decrementCount)
            : handle{handle}
            , incrementCount{incrementCount}
            , decrementCount{decrementCount}
        {
        }

        ~basic_handle_wrapper()
        {
            decrementCount(handle);
        }

        basic_handle_wrapper(basic_handle_wrapper const& wrap)
            : handle{wrap.handle}
            , incrementCount{wrap.incrementCount}
            , decrementCount{wrap.decrementCount}
        {
            incrementCount(handle);
        }

        basic_handle_wrapper& operator=(basic_handle_wrapper const& wrap)
        {
            // all noexcept
            incrementCount = wrap.incrementCount;
            decrementCount = wrap.decrementCount;
            handle = wrap.handle;

            incrementCount(handle);
        }

        basic_handle_wrapper(basic_handle_wrapper&& wrap) = default;
        basic_handle_wrapper& operator=(basic_handle_wrapper&& wrap) = default;

    private:
        T* handle;
        FuncT incrementCount;
        FuncT decrementCount;
    };

    using basic_message_handle = basic_handle_wrapper <
        sd_bus_message,
        decltype(&sd_bus_message_ref)
    >;

    inline basic_message_handle make_basic_message_handle(sd_bus_message* msg)
    {
        return basic_message_handle(msg, &sd_bus_message_ref, &sd_bus_message_unref);
    }
}

