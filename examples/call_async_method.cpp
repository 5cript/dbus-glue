#include <dbus-glue/dbus_interface.hpp>
#include <dbus-glue/bindings/busy_loop.hpp>
#include <iostream>

int main()
{
	using namespace DBusGlue;
	using namespace std::chrono_literals;

	auto bus = open_system_bus();

	// install busy_loop as event_loop.
	make_busy_loop(&bus);

	bus.call_method_async <void(std::vector <std::string> const&)> (
	    "org.freedesktop.timedate1",
	    "/org/freedesktop/timedate1",
	    "org.freedesktop.timedate1",
	    "ListTimezones",
	    [](std::vector <std::string> const& timeZones)
	    {
		    for (auto const& zone : timeZones)
				std::cout << zone << "\n";
	    },
	    [](message&, std::string const& errorMessage)
	    {
		    std::cerr << errorMessage << "\n";
	    },
	    1s
	);

	// prevent immediate exit
	std::cin.get();

	return 0;
}
