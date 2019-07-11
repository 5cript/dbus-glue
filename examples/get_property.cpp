#include <dbus-mockery/bindings/bus.hpp>

#include <iostream>
#include <vector>
#include <string>

using namespace DBusMock;

int main()
{
	auto bus = Bindings::open_system_bus();

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
