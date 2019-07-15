#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/types.hpp>
#include <dbus-mockery/bindings/sdbus_core.hpp>

#include <stdexcept>

using namespace std::string_literals;

namespace DBusMock::Bindings
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
