#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/busy_loop.hpp>
#include <iostream>
#include <chrono>

namespace BlueZ::org::bluez::hci
{
    class Adapter
	{
	public:
		virtual ~Adapter() = default;

	public: // Methods
	public: // Properties
		DBusMock::readable <std::string> Name;
		DBusMock::read_writeable <std::string> Alias;
	public: // Signals
	};
}

DBUS_MOCK_NAMESPACE
(
    (BlueZ)(org)(bluez)(hci),
    Adapter,
    DBUS_MOCK_NO_METHODS,
    DBUS_MOCK_PROPERTIES(Name, Alias),
    DBUS_MOCK_NO_SIGNALS
)

int main()
{
	using namespace DBusMock;
	using namespace std::chrono_literals;

	auto bus = open_system_bus();
	make_busy_loop(&bus);

	auto adapter = create_interface <BlueZ::org::bluez::hci::Adapter>
	(
	    bus,
	    "org.bluez",
	    "/org/bluez/hci0",
	    "org.bluez.Adapter1"
	);

	adapter.Name.get(async_flag)
	    .then([](auto const& name) {
		    std::cout << "name: " << name << "\n";
	    })
	    .error([](auto&, auto const& errorMessage){
		    std::cerr << errorMessage << "\n";
	    })
	    .timeout(1s)
	;

	adapter.Alias.set(async_flag, "MyAlias")
	    .then([]() {
		    std::cout << "alias setting complete\n";
	    })
	    .error([](auto&, auto const& errorMessage){
		    std::cerr << errorMessage << "\n";
	    })
	    .timeout(1s)
	;

	std::cin.get();
	return 0;
}
