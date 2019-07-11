#include <dbus-mockery/bindings/bus.hpp>

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>

int main()
{
	using namespace DBusMock;

	auto bus = Bindings::open_system_bus();

	/* write a property of an interface */
	bus.write_property(
	    "org.bluez",
	    "/org/bluez/hci0",
	    "org.bluez.Adapter1",
	    "Discoverable",
	    Bindings::resolvable_variant{{'b', "b"}, true}
	);

	return 0;
}
