#include <dbus-mockery/bindings/bus.hpp>

#include <stdexcept>
#include <string>
#include <limits>

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
        : bus{bus}
        , unnamed_slots{}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void Bus::busy_loop(std::atomic <bool>& running)
    {
        int r = 0;
        for (;running.load();)
        {
            sd_bus_message* m = nullptr;
            r = sd_bus_process(bus, &m);
            if (r < 0)
                throw std::runtime_error("error in bus processing: "s + strerror(-r));

            if (r == 0)
            {
                r = sd_bus_wait(bus, 1'000'000);
                if (r < 0)
                    throw std::runtime_error("error in bus waiting: "s + strerror(-r));
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    Bus::~Bus()
    {
        sd_bus_unref(bus);
    }
//#####################################################################################################################
}
