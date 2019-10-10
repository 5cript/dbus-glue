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

using type = variant_dictionary <std::unordered_map>;
//using type = int;

class Alubber
{
public:
    virtual auto Blub(type const& dict) -> void;
};

class AlubberExpose : public DBusMock::exposable_interface
{
public:
    // these members determine the path and service name
    // for registration, do not need to be const literals.
    std::string path() const override
    {
        return "/asdf";
    }
    std::string service() const override
    {
        return "com.bla.my";
    }

public: // Methods
    auto Blub(type dict) -> void
    {
        std::cout << "roflcopter\n";

        std::cout << dict.size() << std::endl;
        int32_t f;
        variant_load (dict["param1"], f);
        std::cout << f << std::endl;
    }
};

DBUS_MOCK
(
    Alubber,
    DBUS_MOCK_METHODS(Blub),
    DBUS_MOCK_NO_PROPERTIES,
    DBUS_MOCK_NO_SIGNALS
)

int main()
{
    auto bus = open_user_bus();

    using namespace DBusMock;
    using namespace ExposeHelpers;

    auto shared_ptr_to_interface = make_interface <AlubberExpose>(
        DBusMock::exposable_method_factory{} <<
            name("Blub") <<
            parameter("dict") <<
            as(&AlubberExpose::Blub)
    );
    bus.expose_interface(shared_ptr_to_interface);

    auto result = sd_bus_request_name(bus.handle(), "com.bla.my", 0);

    if (result < 0)
    {
        std::cerr << strerror(-result);
        return 1;
    }

    auto dbusInterface = create_interface <Alubber>(
        bus,
        "com.bla.my",
        "/asdf",
        "com.bla.my"
    );

    type params;
    variant_store(bus, params["param1"], static_cast <int32_t>(7));


    make_busy_loop(&bus, 200ms);

    dbusInterface.Blub(async_flag, params)
        .then([](){std::cout << "called";})
        .error([](auto&, auto const& errorMessage)
        {
            // first parameter is the result message,
            // It probably does not contain anything useful.

            std::cerr << errorMessage << "\n";
        })
        .timeout(3s)
    ;

    std::cin.get();
    return 0;
}
