#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/types.hpp>
#include <dbus-mockery/bindings/sdbus_core.hpp>

#include <stdexcept>

using namespace std::string_literals;

namespace DBusMock
{
//#####################################################################################################################
    message::message(sd_bus_message* messagePointer)
        : msg{messagePointer}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    message::~message()
    {
        if (msg != nullptr)
            sd_bus_message_unref(msg);
    }
//---------------------------------------------------------------------------------------------------------------------
    message& message::operator=(message&& other)
    {
        msg = other.msg;
        other.msg = nullptr;
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    message::message(message&& other)
        : msg {other.msg}
    {
        other.msg = nullptr;
    }
//---------------------------------------------------------------------------------------------------------------------
    message message::clone(int& r, bool full)
    {
        using namespace std::string_literals;

        auto* bus = sd_bus_message_get_bus(handle());
        sd_bus_message* created;
        uint8_t msgType;
        r = sd_bus_message_get_type(handle(), &msgType);
        if (r < 0)
            throw std::runtime_error("could not clone message: "s + strerror(-r));

        r = sd_bus_message_new(bus, &created, msgType);
        if (r < 0)
            throw std::runtime_error("could not clone message: "s + strerror(-r));

        sd_bus_message_copy(created, handle(), full);
        if (full)
            rewind(full);
        return message{created};
    }
//---------------------------------------------------------------------------------------------------------------------
    message message::clone(bool full)
    {
        int r;
        return clone(r, full);
    }
//---------------------------------------------------------------------------------------------------------------------
    void message::rewind(bool full)
    {
        sd_bus_message_rewind(handle(), full);
    }
//---------------------------------------------------------------------------------------------------------------------
    message::operator sd_bus_message*()
    {
        return msg;
    }
//---------------------------------------------------------------------------------------------------------------------
    sd_bus_message* message::release()
    {
        auto* temp = msg;
        msg = nullptr;
        return temp;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string message::comprehensible_type() const
    {
        auto descriptor = type();

        std::string result;
        if (descriptor.type == '\0')
        {
            return "void";
        }
        if (descriptor.type == 'a')
        {
            result = "array of "s;
        }
        for (char const* c = descriptor.contained.data(); c != nullptr && *c != 0; ++c)
        {
            result += typeToComprehensible(*c);
            if (*(c+1) != 0)
                result += ',';
        }

        return result;
    }
//---------------------------------------------------------------------------------------------------------------------
    type_descriptor message::type() const
    {
        char typeSig;
        const char* contentsSig = nullptr;
        auto r = sd_bus_message_peek_type(msg, &typeSig, &contentsSig);
        if (r < 0)
            std::runtime_error("Failed to peek message type: "s + strerror(-r));

        if (contentsSig == nullptr)
            contentsSig = "";

        return {typeSig, contentsSig};
    }
//#####################################################################################################################
}
