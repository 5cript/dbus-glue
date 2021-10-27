#include <dbus-glue/dbus_interface.hpp>
#include <dbus-glue/bindings/bus.hpp>
#include <dbus-glue/bindings/busy_loop.hpp>

#include <dbus-glue-system/accounts/accounts.hpp>

#include <iostream>
#include <fstream>
#include <string>

#include <thread>
#include <chrono>

using namespace DBusGlue;
using namespace std::chrono_literals;

int main()
{
	auto bus = open_system_bus();

	try
	{
		bus.install_event_loop(std::unique_ptr <event_loop> (new busy_loop(&bus, 50ms)));

		// wrapped interface for creating / deleting accounts.
		auto accountControl = create_interface <Accounts::org::freedesktop::Accounts::Accounts>(
		    bus,
		    "org.freedesktop.Accounts",
		    "/org/freedesktop/Accounts",
		    "org.freedesktop.Accounts"
		);

		// listen for created users
		bus.install_signal_listener <void(object_path)> (
		    "org.freedesktop.Accounts",
		    "/org/freedesktop/Accounts",
		    "org.freedesktop.Accounts",
		    "UserAdded",
		    [](object_path const& p) {
			    // success callback
			    std::cout << "callback - create: " << p << std::endl;
		    },
		    [](message&, std::string const& str) {
			    // failure callback
			    std::cerr << "oh no something gone wrong: " << str << "\n";
		    }
		);

		// listen for user deleted events:
		bus.install_signal_listener <void(object_path)> (
		    "org.freedesktop.Accounts",
		    "/org/freedesktop/Accounts",
		    "org.freedesktop.Accounts",
		    "UserDeleted",
		    [&](object_path const& p) {
			    // this is called from the dbus system.
			    std::cout << "callback - delete: " << p << std::endl;

				// create a user from here.
				auto path = accountControl.CreateUser("tempus", "tempus", 0);
		    },
		    [](message&, std::string const& str) {
			    // this is called when an error got signaled into our callback.
			    std::cerr << "oh no something gone wrong: " << str << "\n";
		    }
		);

		// try to delete a user with id 1001. WARNING, DONT JUST DELETE SOME USER ON YOUR SYSTEM. obviously...
		try {
			// commented out in case you just run this example
			// you should get the id from the name first.
			// accountControl.DeleteUser(1001, false);
		} catch (std::exception const& exc) {
			// Create the user if he doesn't exist
			accountControl.CreateUser("tempus", "tempus", 0);
			std::cout << exc.what() << std::endl;
		}

		// just wait here so we dont exit directly
		std::cin.get();
	}
	catch (std::exception const& exc)
	{
		std::cout << exc.what() << "\n";
	}

	return 0;
}
