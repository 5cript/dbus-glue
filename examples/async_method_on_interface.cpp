#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/busy_loop.hpp>
#include <iostream>

namespace org::freedesktop
{
    // Declare an interface. This one is just reduced to one function, which is enough for this example
    class IDBus
	{
	public:
		virtual auto ListNames() -> std::vector <std::string> = 0;

		// Silences a warning, but IDBus is never really used polymorphicly.
		virtual ~IDBus() = default;
	};
}

// Mock the interface (i know the term is not really correct, but the idea comes from mocking frameworks).
DBUS_MOCK_NAMESPACE
(
    (org)(freedesktop),
    IDBus,
    DBUS_MOCK_METHODS(ListNames),
    DBUS_MOCK_NO_PROPERTIES,
    DBUS_MOCK_NO_SIGNALS
)

int main()
{
	using namespace DBusMock;
	using namespace std::chrono_literals;

	auto bus = open_system_bus();
	make_busy_loop(&bus);

	auto dbusInterface = create_interface <org::freedesktop::IDBus>(
	    bus,
	    "org.freedesktop.DBus",
	    "/org/freedesktop/DBus",
	    "org.freedesktop.DBus"
	);

	auto names = dbusInterface.ListNames();

	dbusInterface.ListNames(DBusMock::async_flag)
	    .then([](auto const& names){
		    for (auto const& i : names)
			{
				if (i.front() != ':')
					std::cout << i << "\n";
			}
	    })
	    .error([](auto&, auto const& errorMessage){
		    std::cerr << errorMessage << "\n";
	    })
	    .timeout(1s)
	;

	// prevent immediate exit
	std::cin.get();

	return 0;
}
