#include <dbus-glue/bindings/bus.hpp>

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>

int main()
{
	using namespace DBusGlue;

	auto bus = open_system_bus();

	/* write a property of an interface */
	bus.write_property(
	    "org.bluez",
	    "/org/bluez/hci0",
	    "org.bluez.Adapter1",
	    "Discoverable",
	    variant{{'b', "b"}, true}
	);

	return 0;
}
