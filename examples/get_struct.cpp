#include <dbus-glue/bindings/message.hpp>
#include <dbus-glue/bindings/bus.hpp>

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>

using namespace DBusGlue;

struct Session
{
	std::string name;
	object_path op;
};

MAKE_DBUS_STRUCT(Session, name, op)

int main()
{
	auto bus = open_system_bus();

	try {
		Session session;
		bus.read_property("org.freedesktop.login1", "/org/freedesktop/login1/seat/self", "org.freedesktop.login1.Seat", "ActiveSession", session);

		std::cout << session.name << "\n" << session.op.string() << "\n";
	} catch (std::exception const& exc) {
		std::cout << exc.what() << "\n";
	}

	std::cout << std::flush;

	return 0;
}
