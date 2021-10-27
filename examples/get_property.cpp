#include <dbus-glue/bindings/bus.hpp>

#include <iostream>
#include <vector>
#include <string>

using namespace DBusGlue;

int main()
{
	auto bus = open_system_bus();

	std::string timezone;
	bus.read_property(
	    "org.freedesktop.timedate1",
	    "/org/freedesktop/timedate1",
	    "org.freedesktop.timedate1",
	    "Timezone",
	    timezone
	);
	std::cout << timezone << "\n";

	return 0;
}
