#pragma once

#include "sdbus_core.hpp"
#include "types.hpp"

#include <memory>
#include <string>
#include <utility>
#include <iostream>

namespace DBusMock::Bindings
{
    class message
    {
    public:
        /**
         * @brief message Creates a message object. Must be passed a valid sdbus message.
         * @param messagePointer A valid sdbus message.
         */
        message(sd_bus_message* messagePointer);

        /**
         *	Frees the sdbus message.
         */
        ~message();

        // This class manages resources, dont copy
        message& operator=(message const&) = delete;
        message& operator=(message&&) = delete;
        message(message const&) = delete;
        message(message&&) = delete;

        /**
         * @brief operator sd_bus_message* The message is directly convertible to the handle.
         *				   Since this class is not meant to be used directly, its ok. Also
         *				   can be used by when functionality is missing.
         */
        explicit operator sd_bus_message*();

        /**
         * @brief comprehensible_type Returns a human readable representation of the contained type.
         * @return A type string.
         */
        std::string comprehensible_type() const;

        /**
         * @brief type Returns a type descriptor for the type, which is a type and a string of contained types, if
         *			   there is anything contained.
         * @return
         */
        type_descriptor type() const;

        template <typename T, typename Specializer = void>
        struct append_proxy{};

        template <typename T>
        void append(T const& value)
        {
            append_proxy<T>::write(*this, value);
        }

        /**
         *	A helper for reading various types. Based on template specialization, not overloading.
         */
        template <typename T, typename Specializer = void>
        struct read_proxy{};

        /**
         *  @brief Reads a value of known type from the message stack.
         */
        template <typename T>
        int read(T& value)
        {
            int result = 0;
            unpack_variant([this, &value, &result](){
                result = read_proxy<T>::read(*this, value);
            }, type().contained);
            return result;
        }

        /**
         * @brief read_variant Reads a variant where the variant type is unknown
         * @param resolvable A variant
         * @return
         */
        int read_variant(resolvable_variant& resolvable)
        {
            int result = 0;
            unpack_variant([this, &resolvable, &result](){
                //result = read_proxy<T>::read(*this, value);
                auto descr = type();
                resolvable.descriptor = descr;
                for_signature_do(descr, [this, &resolvable, &result](auto dummy){
                    using value_type = std::decay_t<decltype(dummy)>;
                    value_type val;
                    result = read_proxy <value_type>::read(*this, val);
                    resolvable.value = val;
                });
            }, type().contained);
            return result;
        }

    private:
        template <typename FunctionT>
        void unpack_variant(FunctionT func, std::string_view contained)
        {
            using namespace std::string_literals;
            bool isVariant = (type().type == 'v');
            int r{0};
            if (isVariant)
            {
                r = sd_bus_message_enter_container(msg, SD_BUS_TYPE_VARIANT, contained.data());
                if (r < 0)
                    throw std::runtime_error("could not unpack variant: "s + strerror(-r));
            }
            func();
            if (isVariant)
            {
                r = sd_bus_message_exit_container(msg);
                if (r < 0)
                    throw std::runtime_error("could not leave variant: "s + strerror(-r));
            }
        }

    private:
        sd_bus_message* msg;
    };

    //-----------------------------------------------------------------------------------------------------------------
    // read_proxy
    //-----------------------------------------------------------------------------------------------------------------

    template <template <typename, typename...> typename ContainerT, typename ValueT, typename... ContainerTemplParams>
    struct message::read_proxy <ContainerT <ValueT, ContainerTemplParams...>, void>
    {
        static int read(message& msg, ContainerT <ValueT, ContainerTemplParams...>& container)
        {
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            if (msg.type().type != 'a')
                throw std::invalid_argument("contained type is not an array");

            using namespace std::string_literals;
            auto r = sd_bus_message_enter_container(smsg, SD_BUS_TYPE_ARRAY, type_detect<ValueT>::value);
            if (r < 0)
                throw std::runtime_error("could not enter array: "s + strerror(-r));

            container.clear();
            r = 1;
            while (r > 0)
            {
                ValueT v;
                r = msg.read(v);
                if (r < 0)
                {
                    sd_bus_message_exit_container(smsg);
                    throw std::runtime_error("could not read from array: "s + strerror(-r));
                }
                else if (r > 0)
                    container.push_back(v);
            }

            r = sd_bus_message_exit_container(smsg);
            if (r < 0)
                throw std::runtime_error("could not exit array: "s + strerror(-r));

            return r;
        }
    };

    template <>
    struct message::read_proxy <std::string, void>
    {
        static int read(message& msg, std::string& str)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            char const* value;
            auto r = sd_bus_message_read_basic(smsg, 's', &value);
            if (r < 0)
                throw std::runtime_error("could not read string from message: "s + strerror(-r));

            str = value;
            return r;
        }
    };

    template <typename T>
    struct message::read_proxy <T, std::enable_if_t <std::is_fundamental_v <T>>>
    {
        static int read(message& msg, T& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            int r = 0;
            if constexpr (std::is_same <T, bool>::value)
            {
                int v;
                r = sd_bus_message_read_basic(smsg, type_detect <T>::value[0], &v);
                value = (v != 0);
            }
            else
                r = sd_bus_message_read_basic(smsg, type_detect <T>::value[0], &value);

            if (r < 0)
                throw std::runtime_error("could not read string from message: "s + strerror(-r));

            return r;
        }
    };

    template <template <typename...> typename MapT, typename... Remain>
    struct message::read_proxy <variant_dictionary <MapT, Remain...>>
    {
        static int read(message& msg, variant_dictionary <MapT, Remain...>& dict)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_enter_container(smsg, SD_BUS_TYPE_ARRAY, msg.type().contained.data());
            if (r < 0)
                throw std::runtime_error("could not open array for dictionary: "s + strerror(-r));
            dict.clear();
            r = 1;
            while (r > 0)
            {
                r = sd_bus_message_enter_container(smsg, SD_BUS_TYPE_DICT_ENTRY, msg.type().contained.data());
                if (r < 0)
                    throw std::runtime_error("could not open array of dictionary: "s + strerror(-r));

                if (r == 0)
                    break;

                char const* name;
                r = sd_bus_message_read_basic(smsg, 's', &name);
                if (r < 0)
                    throw std::runtime_error("could not read name in dictionary: "s + strerror(-r));

                resolvable_variant resolvable;
                r = msg.read_variant(resolvable);

                dict.emplace(name, resolvable);

                r = sd_bus_message_exit_container(smsg);
                if (r < 0)
                    throw std::runtime_error("could not close array for dictionary: "s + strerror(-r));
            }

            r = sd_bus_message_exit_container(smsg);
            if (r < 0)
                throw std::runtime_error("could not exit dictionary: "s + strerror(-r));

            return r;
        }
    };

    //-----------------------------------------------------------------------------------------------------------------
    // append_proxy
    //-----------------------------------------------------------------------------------------------------------------

    template <>
    struct message::append_proxy <std::string, void>
    {
        static int write(message& msg, std::string const& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_append_basic(smsg, 's', value.c_str());

            if (r < 0)
                throw std::runtime_error("could not append value: "s + strerror(-r));

            return r;
        }
    };

    template <typename T>
    struct message::append_proxy <T, std::enable_if_t <std::is_fundamental_v <T>>>
    {
        static int write(message& msg, T const& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            int r = 0;
            if constexpr(std::is_same_v <T, bool>)
            {
                int lval = type_converter<T>::convert(value);
                r = sd_bus_message_append_basic(
                    smsg,
                    type_detect <T>::value[0],
                    &lval
                );
            }
            else
            {
                auto lval = type_converter<T>::convert(value);
                r = sd_bus_message_append_basic(
                    smsg,
                    type_detect <T>::value[0],
                    &lval
                );
            }

            if (r < 0)
                throw std::runtime_error("could not append value: "s + strerror(-r));

            return r;
        }
    };

    template <>
    struct message::append_proxy <char const*, void>
    {
        static int write(message& msg, char const* value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_append_basic(smsg, 's', value);

            if (r < 0)
                throw std::runtime_error("could not append value: "s + strerror(-r));
            return r;
        }
    };

    template <>
    struct message::append_proxy <resolvable_variant, void>
    {
        static int write(message& msg, resolvable_variant const& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_open_container(smsg, SD_BUS_TYPE_VARIANT, value.descriptor.contained.data());
            if (r < 0)
                throw std::runtime_error("could not open variant: "s + strerror(-r));

            value.resolve([&msg](auto const& val){
                msg.append(val);
            });

            r = sd_bus_message_close_container(smsg);
            if (r < 0)
                throw std::runtime_error("could not close variant: "s + strerror(-r));
            return r;
        }
    };
}
