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
    message::message(sd_bus* bus, uint8_t type)
    {
        auto r = sd_bus_message_new(bus, &msg, type);
        if (r < 0)
            throw std::runtime_error("could not create message: "s + strerror(-r));
        sd_bus_message_ref(msg);
    }
//---------------------------------------------------------------------------------------------------------------------
    int message::seal(uint64_t cookie)
    {
        return sd_bus_message_seal(msg, cookie, 0);
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
//--------------------------------------------------------------------------------------------------------------------
    sd_bus* message::bus() const
    {
        return sd_bus_message_get_bus(msg);
    }
//--------------------------------------------------------------------------------------------------------------------
    uint8_t message::message_type() const
    {
        uint8_t msgType;
        sd_bus_message_get_type(msg, &msgType);
        return msgType;
    }
//--------------------------------------------------------------------------------------------------------------------
    message message::clone(int& r, bool full) const
    {
        using namespace std::string_literals;

        auto* bus = sd_bus_message_get_bus(msg);
        sd_bus_message* created;
        uint8_t msgType;
        r = sd_bus_message_get_type(msg, &msgType);
        if (r < 0)
            throw std::runtime_error("could not clone message: "s + strerror(-r));

        r = sd_bus_message_new(bus, &created, msgType);
        if (r < 0)
            throw std::runtime_error("could not clone message: "s + strerror(-r));

        sd_bus_message_copy(created, msg, full);
        if (full)
            rewind(full);

        return [&created](){
            message m{created};
            m.seal();
            return m;
        }();
    }
//---------------------------------------------------------------------------------------------------------------------
    message message::clone(bool full) const
    {
        int r;
        return clone(r, full);
    }
//---------------------------------------------------------------------------------------------------------------------
    int message::copy_into(message& msg, bool full) const
    {
        int r = sd_bus_message_copy(msg.msg, this->msg, full);
        if (r < 0)
            throw std::runtime_error("could not copy message into another: "s + strerror(-r));
        return r;
    }
//---------------------------------------------------------------------------------------------------------------------
    int message::rewind(bool full) const
    {
        int r = sd_bus_message_rewind(msg, full);
        if (r < 0)
            throw std::runtime_error("could not rewind message: "s + strerror(-r));
        return r;
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
