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
    bus.read_property(
        "org.freedesktop.ColorManager",
        "org/freedesktop/ColorManager/profiles/icc_7f88bf8e67e4785ac16bb15544c97086",
        "org.freedesktop.ColorManager.Profile",
        "Metadata",
        metadata
    );

    for (auto const& [key, value] : metadata)
    {
        std::cout << key << "=" << value << "\n";
    }

    return 0;
}
