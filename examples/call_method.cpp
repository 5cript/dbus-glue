#include <dbus-glue/bindings/bus.hpp>

#include <vector>
#include <string>
#include <iostream>

int main()
{
	using namespace DBusMock;

	auto bus = open_system_bus();

	/* Issue the method call and store the respons message in m */
	auto response = bus.call_method(
	    "org.freedesktop.timedate1",
	    "/org/freedesktop/timedate1",
	    "org.freedesktop.timedate1",
	    "ListTimezones"
	);

	std::cout << response.comprehensible_type() << "\n";

	std::vector <std::string> vec;
	response.read(vec);

	for (auto const& s : vec)
		std::cout << s << "\n";

	return 0;
}
