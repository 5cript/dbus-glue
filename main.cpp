#include <dbus-mockery/generator/generator.hpp>

#include <iostream>
#include <fstream>
#include <string>

using namespace DBusMock;

int main()
{
    auto bus = Bindings::open_system_bus();

    try
    {
        bus.install_signal_listener <void(object_path)> (
            "org.freedesktop.Accounts",
            "/org/freedesktop/Accounts",
            "org.freedesktop.Accounts",
            "UserAdded",
            [](object_path p) {
                std::cout << p << "\n";
            }
        );
    }
    catch (std::exception const& exc)
    {
        std::cout << exc.what() << "\n";
        std::cout << exc.what() << "\n";
    }

    return 0;
}
