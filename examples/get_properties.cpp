#include <dbus-mockery/bindings/bus.hpp>

#include <iostream>
#include <map>

using namespace DBusMock;

int main()
{
	auto bus = open_system_bus();

	variant_dictionary <std::map> dict;
	bus.read_properties(
	    "org.freedesktop.timedate1",
	    "/org/freedesktop/timedate1",
	    "org.freedesktop.timedate1",
	    dict
	);

	for (auto const& [key, variant] : dict)
	{
		std::cout << key << " = ";
		variant.resolve([](auto const& t){
			std::cout << t << "\n";
		});
	}

	return 0;
}
