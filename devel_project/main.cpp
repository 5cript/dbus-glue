#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/bus.hpp>
#include <dbus-mockery/bindings/busy_loop.hpp>
#include <dbus-mockery/bindings/variant_helpers.hpp>
#include <dbus-mockery/generator/generator.hpp>

#include <dbus-mockery/bindings/exposable_interface.hpp>
#include <dbus-mockery/interface_builder.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <thread>
#include <chrono>

#include <boost/type_index.hpp>

using namespace DBusMock;
using namespace std::chrono_literals;
using namespace std::string_literals;

class Profile : public DBusMock::exposable_interface
{
public:
    Profile(std::string serviceName, std::string path);

    std::string path() const override
    {
        return path_;
    }
    std::string service() const override
    {
        return serviceName_;
    }

    void Release();
    void NewConnection(DBusMock::object_path const& device, DBusMock::variant_dictionary <std::unordered_map> const& fd_properties);
    void RequestDisconnection(DBusMock::object_path const& device);

private:
    std::string serviceName_;
    std::string path_;
};
Profile::Profile(std::string serviceName, std::string path)
    : serviceName_{std::move(serviceName)}
    , path_{std::move(path)}
{
}
void Profile::Release()
{
    std::cout << "I was released\n";
}
void Profile::NewConnection(DBusMock::object_path const& device, DBusMock::variant_dictionary <std::unordered_map> const& fd_properties)
{
    std::cout << "a new connection: " << device.c_str() << "\n";
}
void Profile::RequestDisconnection(DBusMock::object_path const& device)
{
    std::cout << "request disconection: " << device.c_str() << "\n";
}

int main()
{
    auto bus = open_user_bus();

    using namespace DBusMock;
    using namespace ExposeHelpers;

    std::cout << "exposing!\n";
    int r = 0;

    std::shared_ptr <Profile> profile;

    profile.reset(new Profile("de.iwsmesstechnik.ela", "/bluetooth"));

    DBusMock::construct_interface <Profile> (
        profile.get(),
        exposable_method_factory{} <<
            name("Release") <<
            as(&Profile::Release),
        exposable_method_factory{} <<
            name("NewConnection") <<
            parameter("device") <<
            parameter("fd_properties") <<
            as(&Profile::NewConnection),
        exposable_method_factory{} <<
            name("RequestDisconnection") <<
            parameter("device") <<
            as(&Profile::RequestDisconnection)
    );

    int res = 0;
    res = bus.expose_interface(profile);
    if (res < 0)
        throw std::runtime_error("could not register interface: "s + strerror(-res));

    res = sd_bus_request_name(static_cast <sd_bus*> (bus), "de.iwsmesstechnik.ela", 0);
    if (res < 0)
        throw std::runtime_error("could not find interface after register: "s + strerror(-res));

    make_busy_loop(&bus, 200ms);
    bus.loop <busy_loop>()->error_callback([](int r, std::string const& msg){
        std::cout << msg << std::endl;
        return true;
    });

    //while(true){std::this_thread::sleep_for(10ms);}
    std::cout << "exposition complete!" << std::endl;
    std::cin.get();

    /**
     * std::shared_ptr <MyInterface> iface;
     * bus->registerInterface(iface, "de.iwsmesstechnik.ela", "/ela/server");
     *
     *
     */
    // bus->registerInterface()

    return 0;
}
