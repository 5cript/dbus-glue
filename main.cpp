#include <dbus-mockery/declare_interface.hpp>
#include <dbus-mockery/bindings/sdbus_core.hpp>
#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/bus.hpp>

#include <iostream>
#include <utility>
#include <vector>
#include <string>

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

    bus.read_properties(
        "org.freedesktop.timedate1",
        "/org/freedesktop/timedate1",
        "org.freedesktop.timedate1"
    );


    return 0;
}
