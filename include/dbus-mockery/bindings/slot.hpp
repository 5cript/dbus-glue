#pragma once

#include "sdbus_core.hpp"
#include "function_wrap.hpp"

#include <string_view>
#include <boost/type_index.hpp>

namespace DBusMock::Bindings
{
    template <typename SignatureT>
    class slot;

    class slot_base
    {
    public:
        virtual ~slot_base() = default;
        virtual void unpack_message(message& msg) = 0;

        template <typename... List>
        void pass_message_to_slot(message& msg, slot_base* base, std::string_view signature)
        {
            if (signature.empty())
            {
                std::cout << "\n Readable name: " << boost::typeindex::type_id<std::tuple <List...>>().pretty_name() << "\n";

                auto ptr = dynamic_cast <slot <void(List...)>*>(base);
                if (ptr == nullptr)
                {
                    using namespace std::string_literals;
                    throw std::invalid_argument("could not properly cast back slot"s + boost::typeindex::type_id<std::tuple <List...>>().pretty_name());
                }
                ptr->unpack_message(msg);
                return;
            }
        }

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
        slot(std::function <SignatureT> func, sd_bus_slot* ptr = nullptr)
            : ptr_{ptr}
            , attached_{std::move(func)}
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
    };
}
