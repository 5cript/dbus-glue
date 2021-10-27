#include <dbus-glue/dbus_interface.hpp>
#include <dbus-glue/bindings/busy_loop.hpp>
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

// Make the interface
DBUS_DECLARE_NAMESPACE
(
    (org)(freedesktop),
    IDBus,
    DBUS_DECLARE_METHODS(ListNames),
    DBUS_DECLARE_NO_PROPERTIES,
    DBUS_DECLARE_NO_SIGNALS
)

int main()
{
	using namespace DBusGlue;
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

	dbusInterface.ListNames(DBusGlue::async_flag)
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
