#include <dbus-mockery/bindings/exposables/exposable_property.hpp>
#include <dbus-mockery/bindings/detail/table_entry.hpp>

#include <iostream>

int dbus_mock_exposable_property_read
(
    sd_bus* bus,
    char const* path,
    char const* interface,
    char const* property,
    sd_bus_message* reply,
    void* userdata,
    sd_bus_error* error
)
{
    using namespace DBusMock;
    [[maybe_unused]] auto* entry = reinterpret_cast <detail::table_entry*>(userdata);

    std::cout << path << std::endl;
    std::cout << interface << std::endl;
    std::cout << property << std::endl;

    std::cout << r << "\n";
    return 1;
}

int dbus_mock_exposable_property_write
(
    sd_bus* bus,
    char const* path,
    char const* interface,
    char const* property,
    sd_bus_message* reply,
    void* userdata,
    sd_bus_error* error
)
{
    using namespace DBusMock;
    [[maybe_unused]] auto* entry = reinterpret_cast <detail::table_entry*>(userdata);

    std::cout << path << std::endl;
    std::cout << interface << std::endl;
    std::cout << property << std::endl;
    return 0;
}
