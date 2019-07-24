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
        template <typename TupleT, typename T>
        struct tuple_push
        {
        };

        template <typename T, typename... List>
        struct tuple_push <std::tuple <List...>, T>
        {
            using type = tuple_push <List..., T>;
        };

        template <typename TupleT, typename T>
        using tuple_push_t = typename tuple_push <TupleT, T>::type;

        // do not instantiate. its a compiler killer
        template <typename TupleT, typename FunctionT, typename... BuiltBefore>
        [[deprecated]] static void read_struct_until_end(std::string_view& signature, FunctionT&& finalizer)
        {
            if constexpr (std::tuple_size_v <TupleT> > SIGNAL_STRUCT_PARAM_MAX_COUNT)
            {
                using namespace std::string_literals;

                if (signature.front() != ')')
                    throw std::domain_error
                    (
                        "structures with more than "s + std::to_string(SIGNAL_STRUCT_PARAM_MAX_COUNT) +
                        " cannot be automatically type deciphered"
                    );

                signature.remove_prefix(1);
                // continue down the line.
                form_message_types <FunctionT, BuiltBefore..., TupleT>
                (
                    signature,
                    std::forward <FunctionT&&> (finalizer)
                );
            }

            if (signature.empty())
                throw std::invalid_argument("struct has to end with ')'");

            if (signature.front() == ')')
            {
                signature.remove_prefix(1);
                // continue down the line.
                form_message_types <FunctionT, BuiltBefore..., TupleT>
                (
                    signature,
                    std::forward <FunctionT&&> (finalizer)
                );
            }
            else
            {
                form_message_types
                (
                    signature,
                    [&](auto type_dummy)
                    {
                        using type = std::decay_t <decltype(type_dummy)>;
                        read_struct_until_end
                        <
                            tuple_push_t <TupleT, type>,
                            FunctionT,
                            BuiltBefore...
                        >
                        (
                            signature,
                            std::forward <FunctionT&&> (finalizer)
                        );
                    }
                );
            }
        }

        /*
            types ::= complete_type*
            complete_type ::= basic_type | variant | structure | array | dictionary
            basic_type ::= "y" | "n" | "q" | "u" | "i" | "x" | "t" | "d" |
                           "b" | "h" |
                           "s" | "o" | "g"
            variant ::= "v"
            structure ::= "(" complete_type+ ")"
            array ::= "a" complete_type
            dictionary ::= "a" "{" basic_type complete_type "}"
        */
        /**
         * this deconstructs the regular grammar signature and forms a template parameter list.
         * This template parameter list is then passed to finalizer in a tuple from which the type can be retrieved.
         * This is simpler than passing a functor with an argument list. Its less effective,
         * but this is already complicated enough.
         */
        // do not instantiate. its a compiler killer
        template <typename FunctionT, typename... List>
        [[deprecated]] static void form_message_types
        (
            std::string_view& signature,
            FunctionT&& finalizer
        )
        {
            using namespace std::string_literals;

            if (signature.empty())
                finalizer(static_cast <std::tuple <List...>*>(nullptr));

            if constexpr (sizeof...(List) > SIGNAL_PARAM_MAX_COUNT)
            {
                throw std::domain_error
                (
                    "signals with more than "s + std::to_string(SIGNAL_PARAM_MAX_COUNT) +
                    " cannot be automatically type deciphered"
                );
            }

            // basic_type
            auto front = signature.front();
            if (for_signature_do_noexcept({front, ""}, [&](auto type_dummy){
                signature.remove_prefix(1);
                form_message_types <FunctionT, List..., std::decay_t <decltype(type_dummy)>>
                (
                    signature,
                    std::forward <FunctionT&&> (finalizer)
                );
            }))
            {
                return;
            }

            // variant
            if (front == 'v')
            {
                signature.remove_prefix(1);
                form_message_types <FunctionT, List..., variant>
                (
                    signature,
                    std::forward <FunctionT&&> (finalizer)
                );
                return;
            }

            // from here on, signature size cannot be less than one
            if (signature.size() == 1)
                throw std::invalid_argument("signature size must be larger than 1 here");

            // array
            if (front == 'a' && signature[1] != '{')
            {
                signature.remove_prefix(2);
                form_message_types // read complete type, then continue
                (
                    signature,
                    [&](auto type_dummy)
                    {
                        form_message_types <FunctionT, List..., std::vector <std::decay_t <decltype(type_dummy)>>>
                        (
                            signature,
                            std::forward <FunctionT&&> (finalizer)
                        );
                    }
                );
                return;
            }

            // dictionary
            if (front == 'a' && signature[1] == '{')
            {
                signature.remove_prefix(2);
                front = signature.front();

                // Read a basic_type (key_type) and on success continue on to read the complete_type (value_type)
                if (!for_signature_do_noexcept({front, ""}, [&](auto type_dummy){
                    using key_type = std::decay_t <decltype(type_dummy)>;
                    // read complete_type (value_type)
                    signature.remove_prefix(1);
                    form_message_types
                    (
                        signature,
                        [&](auto type_dummy)
                        {
                            using value_type = std::decay_t <decltype(type_dummy)>;
                            if (signature.front() != '}')
                                throw std::invalid_argument("dictionary is not closed off by '}'");
                            signature.remove_prefix(1);
                            form_message_types <FunctionT, List..., std::unordered_map <key_type, value_type>>
                            (
                                signature,
                                std::forward <FunctionT&&> (finalizer)
                            );
                        }
                    );
                }))
                {
                    throw std::invalid_argument("dictionary key must be a basic type, but is not");
                }
            }

            // struct
            if (front == '(')
            {
                signature.remove_prefix(1);
                if (signature.empty() || signature.front() == ')')
                {
                    throw std::invalid_argument("requires at least one type in struct");
                }

                // read first required complete_type
                form_message_types
                (
                    signature,
                    [&](auto type_dummy)
                    {
                        using _1st = std::decay_t <decltype(type_dummy)>;

                        read_struct_until_end
                        <
                            std::tuple <_1st>,
                            FunctionT&&,
                            List...
                        >
                        (signature, std::forward <FunctionT&&> (finalizer));
                    }
                );
            }
        }


    public:
        virtual ~slot_base() = default;
        virtual void unpack_message(message& msg) = 0;

        template <typename... List>
        void pass_message_to_slot
        (
            message& msg,
            slot_base* base,
            std::string_view signature
        )
        {
            form_message_types <>
            (
                signature,
                [base, &msg](auto* result_type_dummy)
                {
                    using result_type = std::decay_t <decltype(result_type_dummy)>;
                    std::cout << "\n Readable name: " << boost::typeindex::type_id<result_type>().pretty_name() << "\n";

                    /*
                    auto ptr = dynamic_cast <slot <void(List...)>*>(base);
                    if (ptr == nullptr)
                    {
                        using namespace std::string_literals;
                        throw std::invalid_argument("could not properly cast back slot"s + boost::typeindex::type_id<std::tuple <List...>>().pretty_name());
                    }
                    ptr->unpack_message(msg);
                    */
                }
            );
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
