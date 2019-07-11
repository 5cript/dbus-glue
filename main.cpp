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
    using namespace DBusMock;

    auto bus = Bindings::open_system_bus();

    /* Issue the method call and store the respons message in m */
    bus.write_property(
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        "Discoverable",
        Bindings::resolvable_variant{{'b', "b"}, true}
    );

    bool discov;
    bus.read_property(
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        "Discoverable",
        discov
    );

    std::cout << discov << "\n";
    std::cout.flush();

    return 0;
}
