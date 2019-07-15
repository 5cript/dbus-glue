#include <dbus-mockery/declare_interface.hpp>
#include <dbus-mockery/bindings/sdbus_core.hpp>
#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/bus.hpp>

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>

using namespace DBusMock;

using face = declare_interface<
    methods <
        method <std::pair <int, int>()>
    >,
    properties <

    >,
    signals <

    >
>;

int main()
{
    auto bus = Bindings::open_system_bus();

    std::map<std::string, std::string> metadata;

    try {
        //auto response = bus.call_method("org.freedesktop.ColorManager", "/org/freedesktop/ColorManager", "org.freedesktop.ColorManager", "GetDevices");
        auto response = bus.call_method("org.freedesktop.hostname1", "/org/freedesktop/hostname1", "org.freedesktop.hostname1", "GetProductUUID", true);
        /*
        auto response = bus.call_method(
            "org.freedesktop.timedate1",
            "/org/freedesktop/timedate1",
            "org.freedesktop.timedate1",
            "ListTimezones"
        );
        */

        std::cout << response.comprehensible_type() << "\n";

        std::vector <uint8_t> vop;
        response.read(vop);

        for (auto const& path : vop)
        {
            std::cout << path << "\n";
        }
    } catch (std::exception const& exc) {
        std::cout << exc.what() << "\n";
    }

    for (auto const& [key, value] : metadata)
    {
        std::cout << key << "=" << value << "\n";
    }

    std::cout << std::flush;

    return 0;
}
