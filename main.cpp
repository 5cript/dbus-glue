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
        interface_generator gen;
        std::ofstream temp{"interfaces.hpp", std::ios_base::binary};
        //gen.write_introspected_xml_to("introspected.xml", bus, "org.bluez", "/org/bluez");
        gen.generate_interface_from(temp, bus, "org.freedesktop.Accounts", "/org/freedesktop/Accounts/User1000", "Accounts");
        temp.flush();
    }
    catch (std::exception const& exc)
    {
        std::cout << exc.what() << "\n";
        std::cout << exc.what() << "\n";
    }

    return 0;
}
