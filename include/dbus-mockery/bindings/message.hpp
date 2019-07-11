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

        /**
         *	A helper for reading various types. Based on template specialization, not overloading.
         */
        template <typename T, typename Specializer = void>
        struct read_proxy{};

        template <typename T>
        void read(T& value)
        {
            unpack_variant([this, &value](){
                read_proxy<T>::read(*this, value);
            }, type().contained);
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

    template <template <typename, typename...> typename ContainerT, typename ValueT, typename... ContainerTemplParams>
    struct message::read_proxy <ContainerT <ValueT, ContainerTemplParams...>, void>
    {
        static void read(message& msg, ContainerT <ValueT, ContainerTemplParams...>& container)
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
                if constexpr (std::is_fundamental_v <ValueT>)
                {
                    ValueT value;
                    r = sd_bus_message_read_basic(smsg, type_detect <ValueT>::value[0], &value);
                    if (r > 0)
                        container.push_back(value);
                }
                else if constexpr (std::is_same_v <ValueT, std::string>)
                {
                    char const* value;
                    r = sd_bus_message_read_basic(smsg, 's', &value);
                    if (r > 0)
                        container.push_back(value);
                }
                if (r < 0)
                {
                    sd_bus_message_exit_container(smsg);
                    throw std::runtime_error("Coud not read from array: "s + strerror(-r));
                }
            }

            r = sd_bus_message_exit_container(smsg);
            if (r < 0)
                throw std::runtime_error("could not exit array: "s + strerror(-r));
        }
    };

    template <>
    struct message::read_proxy <std::string, void>
    {
        static void read(message& msg, std::string& str)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            char const* value;
            auto r = sd_bus_message_read_basic(smsg, 's', &value);
            if (r < 0)
                throw std::runtime_error("Could not read string from message: "s + strerror(-r));

            str = value;
        }
    };

    template <typename T>
    struct message::read_proxy <T, std::enable_if_t <std::is_fundamental_v <T>>>
    {
        static void read(message& msg, T& value)
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
                throw std::runtime_error("Could not read string from message: "s + strerror(-r));
        }
    };

    template <template <typename...> MapT>
    struct message::read_proxy <variant_dictionary <MapT>>
    {
        static void read(message& msg, variant_dictionary <MapT>& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_open_container(smsg, SD_BUS_TYPE_DICT_ENTRY, "sv");
            if (r < 0)
                throw std::runtime_error("could not open dictionary: "s + strerror(-r));

            r = sd_bus_message_close_container(smsg);
            if (r < 0)
                throw std::runtime_error("Could not close dictionary: "s + strerror(-r));
        }
    };
}
