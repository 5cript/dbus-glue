#include <dbus-mockery/bindings/bus.hpp>
#include <dbus-mockery/bindings/message.hpp>

#include <stdexcept>
#include <string>
#include <limits>
#include <chrono>

using namespace std::string_literals;

namespace DBusMock::Bindings
{
//#####################################################################################################################
    Bus open_system_bus()
    {
        sd_bus* bus;
        auto r = sd_bus_open_system(&bus);
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return Bus{bus};
    }
//---------------------------------------------------------------------------------------------------------------------
    Bus open_user_bus()
    {
        sd_bus* bus;
        auto r = sd_bus_open_user(&bus);
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return Bus{bus};
    }
//---------------------------------------------------------------------------------------------------------------------
    Bus open_system_bus(std::string const& host)
    {
        sd_bus* bus;
        auto r = sd_bus_open_system_remote(&bus, host.c_str());
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return Bus{bus};
    }
//---------------------------------------------------------------------------------------------------------------------
    Bus open_system_bus_machine(std::string const& machine)
    {
        sd_bus* bus;
        auto r = sd_bus_open_system_machine(&bus, machine.c_str());
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return Bus{bus};
    }
//#####################################################################################################################
    Bus::Bus(sd_bus* bus)
        : bus_{bus}
        , unnamed_slots_{}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void Bus::busy_loop(std::atomic <bool>* running)
    {
        using namespace std::chrono_literals;
        constexpr auto wait_timeout = 500ms;

        int r = 0;
        for (;running->load();)
        {
            sd_bus_message* m = nullptr;
            {
                std::scoped_lock guard{sdbus_lock_};
                r = sd_bus_process(bus_, &m);
            }
            if (m != nullptr)
            {
                message msg{m};
                //std::cout << "message processed: " << msg.comprehensible_type() << "\n";
            }
            if (r < 0 && r != -74 /*badmsg*/)
                throw std::runtime_error("error in bus processing: "s + strerror(-r) + " (" + std::to_string(r) + ")");

            if (r == 0)
            {
                r = sd_bus_wait(bus_, std::chrono::duration_cast <std::chrono::microseconds> (wait_timeout).count());
                if (r < 0)
                    throw std::runtime_error("error in bus waiting: "s + strerror(-r));
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void Bus::flush()
    {
        std::scoped_lock guard{sdbus_lock_};
        auto r = sd_bus_flush(bus_);
        if (r < 0)
            throw std::runtime_error("could not flush bus: "s + strerror(-r));
    }
//---------------------------------------------------------------------------------------------------------------------
    void Bus::attach_event_system(sd_event* e, int priority)
    {
        sd_bus_attach_event(bus_, e, priority);
    }
//---------------------------------------------------------------------------------------------------------------------
    sd_event* Bus::get_event_system()
    {
        return sd_bus_get_event(bus_);
    }
//---------------------------------------------------------------------------------------------------------------------
    void Bus::detach_event_system()
    {
        sd_bus_detach_event(bus_);
    }
//---------------------------------------------------------------------------------------------------------------------
    Bus::~Bus()
    {
        sd_bus_flush(bus_);
        sd_bus_close(bus_);
        sd_bus_unref(bus_);
    }
//#####################################################################################################################
}
