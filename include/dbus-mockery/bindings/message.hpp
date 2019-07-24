#pragma once

#include "sdbus_core.hpp"
#include "types.hpp"
#include "object_path.hpp"
#include "struct_adapter.hpp"

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
        explicit message(sd_bus_message* messagePointer);

        /**
         *	Frees the sdbus message.
         */
        ~message();

        // This class manages resources, dont copy
        // note: there are functions for reference counting, but i dont want shared_ptr semantics for this.
        message& operator=(message const&) = delete;
        message(message const&) = delete;
        message& operator=(message&&);
        message(message&&);

        /**
         * @brief operator sd_bus_message* The message is directly convertible to the handle.
         *				   Since this class is not meant to be used directly, its ok. Also
         *				   can be used by when functionality is missing.
         */
        explicit operator sd_bus_message*();

        /**
         * @brief release Releases the ownership of the handle and sets the held handle to nullptr
         * @return The handle.
         */
        sd_bus_message* release();

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

        /**
         * @brief handle Returns the handle directly. Do not close obviously.
         * @return The held handle.
         */
        sd_bus_message* handle()
        {
            return msg;
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

    template <template <typename, typename...> typename ContainerT, template <typename> typename AllocatorT, typename ValueT>
    struct message::read_proxy <ContainerT <ValueT, AllocatorT <ValueT>>, void>
    {
        using container_type = ContainerT <ValueT, AllocatorT <ValueT>>;
        static int read(message& msg, container_type& container)
        {
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);
            auto type = msg.type();

            if (type.type != 'a')
                throw std::invalid_argument("contained type is not an array");

            using namespace std::string_literals;
            auto r = sd_bus_message_enter_container(smsg, SD_BUS_TYPE_ARRAY, type.contained.data());
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

    template <>
    struct message::read_proxy <object_path, void>
    {
        static int read(message& msg, object_path& opath)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            char const* value;
            auto r = sd_bus_message_read_basic(smsg, 'o', &value);
            if (r < 0)
                throw std::runtime_error("could not read object path from message: "s + strerror(-r));

            opath = object_path{value};
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

    template <>
    struct message::read_proxy <file_descriptor, void>
    {
        static int read(message& msg, file_descriptor& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            int fd;
            auto r = sd_bus_message_read_basic(smsg, type_detect <file_descriptor>::value[0], &fd);
            value = fd;

            if (r < 0)
                throw std::runtime_error("could not read string from message: "s + strerror(-r));

            return r;
        }
    };

    template <>
    struct message::read_proxy <signature, void>
    {
        static int read(message& msg, signature& sign)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            char const* value;
            auto r = sd_bus_message_read_basic(smsg, type_detect <signature>::value[0], &value);
            if (r < 0)
                throw std::runtime_error("could not read object path from message: "s + strerror(-r));

            sign = signature{value};
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

    template<typename Tuple, std::size_t... Is>
    int read_tuple_impl
    (
        message& msg,
        Tuple& t,
        std::index_sequence<Is...>
    )
    {
        return (msg.read(std::get<Is>(t)) + ... + 0);
    }

    template <typename... Parameters>
    struct message::read_proxy <std::tuple <Parameters...>, void>
    {
        static int read(message& msg, std::tuple <Parameters...>& tuple)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_enter_container(smsg, SD_BUS_TYPE_STRUCT, msg.type().contained.data());
            if (r < 0)
                throw std::runtime_error("could not enter struct: "s + strerror(-r));

            r = read_tuple_impl(msg, tuple, std::make_index_sequence <std::tuple_size_v <std::decay_t <decltype(tuple)>>>{});

            auto r_exit = sd_bus_message_exit_container(smsg);
            if (r_exit < 0)
                throw std::runtime_error("could not exit struct: "s + strerror(-r));

            if (msg.type().type == '\0')
                return 0;
            return r;
        }
    };

    template <typename T>
    struct message::read_proxy <T, std::enable_if_t <std::is_class_v <T> && AdaptedStructs::struct_as_tuple <T>::is_adapted>>
    {
        static int read(message& msg, T& object)
        {
            using tuple_type = typename AdaptedStructs::struct_as_tuple <T>::tuple_type;
            tuple_type tuple;
            int r = msg.read(tuple);
            object = AdaptedStructs::struct_as_tuple <T>::from_tuple(tuple);
            return r;
        }
    };

    template <template <typename, typename...> typename MapT, typename ValueT, typename KeyT, typename CompareOrHash, typename AllocatorOrKeyEqual, typename... MaybeAllocator>
    struct message::read_proxy <MapT <KeyT, ValueT, CompareOrHash, AllocatorOrKeyEqual, MaybeAllocator...>, void>
    {
        using map_type = MapT <KeyT, ValueT, CompareOrHash, AllocatorOrKeyEqual, MaybeAllocator...>;
        static int read(message& msg, map_type& dict)
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

                ValueT value;
                msg.read(value);

                dict.emplace(name, value);

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
    struct message::append_proxy <object_path, void>
    {
        static int write(message& msg, object_path const& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_append_basic(smsg, 'o', value.c_str());

            if (r < 0)
                throw std::runtime_error("could not append value: "s + strerror(-r));
            return r;
        }
    };

    template <>
    struct message::append_proxy <signature, void>
    {
        static int write(message& msg, signature const& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            auto r = sd_bus_message_append_basic(smsg, 'g', value.c_str());

            if (r < 0)
                throw std::runtime_error("could not append value: "s + strerror(-r));
            return r;
        }
    };

    template <>
    struct message::append_proxy <file_descriptor, void>
    {
        static int write(message& msg, file_descriptor const& value)
        {
            using namespace std::string_literals;
            sd_bus_message* smsg = static_cast <sd_bus_message*> (msg);

            int descr = value.descriptor();
            auto r = sd_bus_message_append_basic(smsg, 'h', &descr);

            if (r < 0)
                throw std::runtime_error("could not append value: "s + strerror(-r));
            return r;
        }
    };

    template <int S>
    struct message::append_proxy <char[S], void>
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
