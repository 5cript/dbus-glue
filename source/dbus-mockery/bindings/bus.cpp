#include <dbus-mockery/bindings/bus.hpp>

#include <stdexcept>
#include <string>

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
    Bus::~Bus()
    {
        sd_bus_unref(bus);
    }
//#####################################################################################################################
}
