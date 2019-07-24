#pragma once

#include "sdbus_core.hpp"
#include "function_wrap.hpp"
#include "struct_adapter.hpp"

#include <boost/type_index.hpp>

#include <string_view>
#include <unordered_map>

#ifndef SIGNAL_STRUCT_PARAM_MAX_COUNT
#   define SIGNAL_STRUCT_PARAM_MAX_COUNT 1
#endif

#ifndef SIGNAL_PARAM_MAX_COUNT
#   define SIGNAL_PARAM_MAX_COUNT 1
#endif

namespace DBusMock::Bindings
{
    template <typename SignatureT>
    class slot;

    class slot_base
    {
    private:

    public:
        virtual ~slot_base() = default;
        virtual void unpack_message(message& msg) = 0;
        virtual void on_fail(message&, std::string const&) = 0;

        std::string_view signature() const
        {
            return sign;
        }

    protected:
        // what does the deriving slot have as a signature?
        std::string sign;
    };

    template <typename SignatureT>
    class slot : public slot_base
    {
    public:
        slot
        (
            std::function <SignatureT> func,
            std::function <void(message&, std::string const&)> fail,
            sd_bus_slot* ptr = nullptr
        )
            : ptr_{ptr}
            , attached_{std::move(func)}
            , fail_{std::move(fail)}
        {
        }
        ~slot()
        {
            sd_bus_slot_unref(ptr_);
        }

        explicit operator sd_bus_slot*()
        {
            if (ptr_ != nullptr)
                return ptr_;
        }

        void unpack_message(message& msg) override
        {
            attached_.unpack_message(msg);
        }

        void on_fail(message& msg, std::string const& error_message) override
        {
            fail_(msg, error_message);
        }

        void reset(sd_bus_slot* ptr)
        {
            if (ptr_ != nullptr)
                sd_bus_slot_unref(ptr_);
            ptr_ = ptr;
        }

        slot& operator=(slot const&) = delete;
        slot(slot const&) = delete;
        slot& operator=(slot&&) = default;
        slot(slot&&) = default;

    private:
        sd_bus_slot* ptr_;
        function_wrapper <SignatureT> attached_;
        std::function <void(message&, std::string const&)> fail_;
    };
}
