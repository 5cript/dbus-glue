#include <dbus-glue/dbus_interface.hpp>
#include <dbus-glue/bindings/bus.hpp>
#include <dbus-glue/bindings/busy_loop.hpp>

#include <iostream>
#include <fstream>
#include <string>

#include <thread>
#include <chrono>

using namespace DBusGlue;
using namespace std::chrono_literals;

/**
 * @brief The IAccounts interface. Its the provided interface (org.freedesktop.Accounts) as a C++ class.
 */
class IAccounts
{
public:
	virtual ~IAccounts() = default;

public: // Methods
	virtual auto CreateUser(std::string const& name, std::string const& fullname, int32_t accountType) -> object_path = 0;
	virtual auto DeleteUser(int64_t id, bool removeFiles) -> void = 0;
public: // Properties
public: // signals
	DBusGlue::signal <void(object_path)> UserAdded;
	DBusGlue::signal <void(object_path)> UserDeleted;
};

//----------------------------------------------------------------------------------------

// This step is necessary to enable interface auto-implementation.
// There is a limit to how many properterties and methods are possible. (currently either 64 or 255 each, haven't tried, assume 64)
// This limit can be circumvented by DBUS_DECLARE_N. Which allows to make the same interface more than once.
// A successory call to DBUS_DECLARE_ZIP merges them all together.
DBUS_DECLARE
(
    IAccounts,
    DBUS_DECLARE_METHODS(CreateUser, DeleteUser),
    DBUS_DECLARE_NO_PROPERTIES,
    DBUS_DECLARE_SIGNALS(UserAdded, UserDeleted)
)

int main()
{
	auto bus = open_system_bus();

	try
	{
		bus.install_event_loop(std::unique_ptr <event_loop> (new busy_loop(&bus, 50ms)));

		// wrapped interface for creating / deleting accounts.
		auto accountControl = create_interface <IAccounts>(
		    bus,
		    "org.freedesktop.Accounts",
		    "/org/freedesktop/Accounts",
		    "org.freedesktop.Accounts"
		);

		accountControl.UserAdded.listen(
		    [](object_path const& p) {
			    // success callback
			    std::cout << "callback - create: " << p << std::endl;
		    },
		    [](message&, std::string const& str) {
			    // failure callback
			    std::cerr << "oh no something gone wrong: " << str << "\n";
		    }
		);

		// WARNING! Passing "release_slot" forces you to manage the slots lifetime yourself!
		// You can use this variation to manage lifetimes of your observed signals. With a unique_ptr for example.
		auto* slot = accountControl.UserDeleted.listen(
		    [&](object_path const& p) {
		        // this is called from the dbus system.
		        std::cout << "callback - delete: " << p << std::endl;

		        // create a user from here.
		        auto path = accountControl.CreateUser("tempus", "tempus", 0);
	        },
		    [](message&, std::string const& str) {
			    // this is called when an error got signaled into our callback.
			    std::cerr << "oh no something gone wrong: " << str << "\n";
		    },
		    DBusGlue::release_slot
			);

			// try to delete a user with id 1001. WARNING, DONT JUST DELETE SOME USER ON YOUR SYSTEM. obviously...
			try {
				// commented out in case you just run this example
				// you should get the id from the name first.
				//accountControl.DeleteUser(1001, false);
			} catch (std::exception const& exc) {
				// Create the user if he doesn't exist
				accountControl.CreateUser("tempus", "tempus", 0);
				std::cout << exc.what() << std::endl;
			}

			// just wait here so we dont exit directly
			std::cin.get();

		// cleanup. IF!!! you passed "release_slot"
		delete slot;
	}
	catch (std::exception const& exc)
	{
		std::cout << exc.what() << "\n";
	}

	return 0;
}
