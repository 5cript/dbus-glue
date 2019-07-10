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

    /* Issue the method call and store the respons message in m */
    auto response = bus.call_method(
        "org.freedesktop.timedate1",
        "/org/freedesktop/timedate1",
        "org.freedesktop.timedate1",
        "ListTimezones"
    );

    std::cout << response.comprehensible_type() << "\n";

    std::vector <std::string> vec;
    response.read(vec);

    for (auto const& s : vec)
        std::cout << s << "\n";

    return 0;
}
