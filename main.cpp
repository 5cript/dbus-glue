#include <dbus-mockery/declare_interface.hpp>
#include <dbus-mockery/bindings/sdbus_core.hpp>
#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/bus.hpp>
#include <dbus-mockery/bindings/struct_adapter.hpp>

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>

using namespace DBusMock;

struct User
{
    uint32_t id;
    std::string name;
    object_path dbusProfile;
};

struct Session
{
    std::string name;
    object_path op;
};

MAKE_DBUS_STRUCT(User, id, name, dbusProfile)
MAKE_DBUS_STRUCT(Session, name, op)

int main()
{
    auto bus = Bindings::open_system_bus();

    std::map<std::string, std::string> metadata;

    try {
        auto response = bus.call_method("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "ListUsers");

        std::cout << response.type().type << "" << response.type().contained << "\n";

        std::vector <User> users;
        response.read(users);

        for (auto const& u: users)
        {
            std::cout << u.id << ", " << u.name << ", " << u.dbusProfile << "\n";
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
