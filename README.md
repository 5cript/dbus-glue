# DBus Mockery

## Table of Contents
* [Summary](#Summary)
* [Documentation](#Documentation)
* [Roadmap](#Roadmap)
* [Build](#Build)
* [Tutorial](#Tutorial)
* [Examples](#Examples)

## Summary
The DBus Mockery library wants to make interfacing with a DBus Service / Interface almost as simple as defining a C++ interface.
The DBus protocol is almost entirely hidden from the user. 
This library only enables interfacing with existant DBus APIs. Creating one yourself with this library is currently not planned.

Example interfaces / Predefined interfaces can be found in my repository [dbus-mockery-system]("https://github.com/5cript/dbus-mockery-system").

## Documentation
!! Todo here, will be doxygen generated and served with github.

## Roadmap
Here is a checkbox list of all the tings to come and all that are done so far.

- [x] A bus object for all necessary bus related tasks.
    - [x] Handle the bus lifetime
    - [x] Calling methods
    - [x] Reading properties
    - [x] Writing properties
    - [x] Listening for signals
    - [x] Starting an event loop
- [x] A Macro for declaring DBus interfaces as C++ interfaces.
- [x] Attaching or creating a (simple) event loop to the bus object.
(Note: If you use the sd_event* object system, you still have to setup and teardown the event stuff yourself, this is not wrapped by this library.)
- [x] A rudamentary generator for interfaces. Not really necessary, since writing a simple class declaration is easy and fast, but can be used to get a quick start for big interfaces.

On a created interface, linked to a given DBus interface, you can:
- [x] Call methods.
- [x] Read and Write Properties.
- [x] Connect to slots and listen for signals.
- [x] Call methods and get the results asnchronously.
- [x] Read and Write Properties asynchronously.

##### Improvements not planed, but sensible:
- [ ] Wrap sd_event in another library and make it interact with this.

## Build
This project uses cmake.
* cd dbus-mockery
* mkdir -p build
* cmake ..
* make -j4 (or more/less cores)
* make install (if you want)
##### Dependencies:
- libsystemd, because the systemd sd-bus library is used underneath.
- boost preprocessor

## Tutorial
This is a short tutorial for using the library.

#### Getting started
1. clone the library using git clone
2. create a building directory somewhere and build the library, see the build chapter.
3. now you can add the "include" directory to your projects search path and link the lib. Alternatively use make install. I personally advocate for a project structure where all git dependecies are parallel in the filesystem to the dependent project.

#### Hello World
A very simple program to start off with.
First the includes required for basic mocking:
```C++
#include <dbus-mockery/dbus_interface.hpp>
```

Here we see a very basic interface. You can use d-feet to
inspect and try interfaces provided by services, daemons and other programs.
```C++
namespace org::freedesktop
{
    // Declare an interface. This one is just reduced to one function, which is enough for this example
    class IDBus
    {
    public:
        // We want to make this function callable
        virtual auto ListNames() -> std::vector <std::string> = 0;

        // Silences a warning, but IDBus is never really used polymorphicly.
        virtual ~IDBus() = default;
    };
}
```

Mock the interface (i know the term is not really correct, but the idea comes from mocking frameworks).
When you dont provide any Methods, Properties or Signals, a
special placeholder macro has to be used called "DBUS_MOCK_NO_*"
```C++
DBUS_MOCK_NAMESPACE
(
    (org)(freedesktop),
    IDBus,
    DBUS_MOCK_METHODS(ListNames),
    DBUS_MOCK_NO_PROPERTIES,
    DBUS_MOCK_NO_SIGNALS
)
```

This shows how to use our set up interface:
```C++
int main()
{
    using namespace DBusMock;

    // open the system bus
    auto bus = Bindings::open_system_bus();

    // bind the interface to the remote dbus interface:
    auto dbusInterface = create_interface <org::freedesktop::IDBus>(
        bus,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus"
    );

    // you can now call list names:
    auto services = dbusInterface.ListNames();

    // print all to console, which aren't stupidly named / numbered, therefore unnamed
    for (auto const& service : services)
        if (service.front() != ':')
            std::cout << service << "\n";

    // -> org.freedesktop.DBus
    //    org.freedesktop.Accounts
    //    org.bluez
    //    ...

    return 0;
}
```

#### Asynchronous calls
Now lets change a little bit of the program. We now dont want to do the call synchronously, but asynchronously. Note that as soon as an event handling loop is attached to the bus, even the synchronous calls get processed by the loop and block while its not their turn to be executed, which, in bad cases, can result in "long" wait times at these functions, which may be undersirable.
I therefore recommend to switch to an entirely asynchronous architecture when you use any asynchronous methods/signals on the bus.

Asynchronous functions use "continuation" style. Which means that when ever an asynchronous function finishes, a callback is called from which on execution can be resumed.

Properties can also be read and written asynchronously, by calling get/set on them with the async flag, just the same as with methods.

Only showing relevant differences to before:
```C++
int main()
{
    using namespace DBusMock;
    using namespace std::chrono_literals;

    // open the system bus
    auto bus = Bindings::open_system_bus();

    // create an event loop and attach it to the bus.
    // This is the default implementation, you can provide your own. For instance by using sd_event.
    make_busy_loop(&bus);

    // bind the interface to the remote dbus interface:
    auto dbusInterface = create_interface <org::freedesktop::IDBus>(
        bus,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus"
    );

    // call ListNames asynchronously with async_flag overload:
    // (always the first parameter, if there are any)
    dbusInterface.ListNames(async_flag)
        // if you omit the then, you wont get a result / response.
        // Which can be fine, but is usually not intended
        .then([](auto const& services)
        {
            // print all to console, which aren't stupidly named / numbered, therefore unnamed
            for (auto const& service : services)
                if (service.front() != ':')
                    std::cout << service << "\n";
        })
        // This is called when something goes wrong
        // Can be omited
        .error([](auto&, auto const& errorMessage)
        {
            // first parameter is the result message,
            // It probably does not contain anything useful.

            std::cerr << errorMessage << "\n";
        })
        // A timeout for completion, optional.
        // default is 10 seconds
        .timeout(1s)
    ;

    return 0;
}
```
Asynchronous calls return a proxy object that can be used
to bind callback, error callback and timeout before the call is made. The execution is made when the proxy object gets destroyed, which is after the end of the statement.

So you should ignore the return, and not pass it to a variable, like so:
```C++
// DONT!
auto&& temp = dbusInterface.ListNames(async_flag);
```
unless you actually want that like so:
```C++
{ // artifical scope
    auto&& temp = dbusInterface.ListNames(async_flag);

    temp.then([](auto const&){
        std::cout << "callback!\n";
    });
} // asynchronous call is made here.
```


## Examples
More examples are in the example directory
### Introductory example (User Accounts)
Here is the first example, to show a basis of what this library wants to do.
```C++
#include <dbus-mockery/dbus_interface.hpp>

#include <iostream>
#include <vector>
#include <string>

using namespace DBusMock;

/**
 * @brief The IAccounts interface. Its the provided interface (org.freedesktop.Accounts) as a C++ class.
 */
class IAccounts
{
public:
    virtual ~IAccounts() = default;

    virtual auto CacheUser(std::string const& name) -> object_path = 0;
    virtual auto CreateUser(std::string const& name, std::string const& fullname, int32_t accountType) -> object_path = 0;
    virtual auto DeleteUser(int64_t id, bool removeFiles) -> void = 0;
    virtual auto FindUserById(int64_t id) -> object_path = 0;
    virtual auto ListCachedUsers() -> std::vector <object_path> = 0;
    virtual auto UncacheUser(std::string const& user) -> void = 0;

public: // Properties
    readable <std::vector <object_path>> AutomaticLoginUsers;
    readable <bool> HasMultipleUsers;
    readable <bool> HasNoUsers;
    readable <std::string> DaemonVersion;

public: // signals
    using UserAdded = void(object_path);
    using UserDeleted = void(object_path);
};

//----------------------------------------------------------------------------------------

// This step is necessary to enable interface auto-implementation.
// There is a limit to how many properterties and methods are possible. (currently either 64 or 255 each, haven't tried, assume 64)
// This limit can be circumvented by DBUS_MOCK_N. Which allows to mock the same interface more than once.
// A successory call to DBUS_MOCK_ZIP merges them all together.
DBUS_MOCK
(
    IAccounts,
    DBUS_MOCK_METHODS(CacheUser, CreateUser, DeleteUser, FindUserById, ListCachedUsers, UncacheUser),
    DBUS_MOCK_PROPERTIES(AutomaticLoginUsers, HasMultipleUsers, HasNoUsers, DaemonVersion),
    DBUS_MOCK_SIGNALS(UserAdded, UserDeleted)
)

//----------------------------------------------------------------------------------------

int main()
{
    // open the system bus.
    auto bus = Bindings::open_system_bus();

    try
    {
        // attach interface to remote interface.
        auto user_control = create_interface <IAccounts> (bus, "org.freedesktop.Accounts", "/org/freedesktop/Accounts", "org.freedesktop.Accounts");

        // calling a method with parameters
        //user_control.CreateUser("hello", "hello", 0);

        // calling a method and getting the result
        auto cachedUsers = user_control.ListCachedUsers();

        for (auto const& user : cachedUsers)
        {
            // the object_path type has a stream operator for output
            std::cout << user << "\n";
        }

        // reading a property
        std::cout << user_control.DaemonVersion << "\n";
    }
    catch (std::exception const& exc) // catch all possible exceptions.
    {
        std::cout << exc.what() << "\n";
    }

    return 0;
}
```

### Connect signal with slot
Here is an example on how to listen to emitted signals.
Note that signal handling requires an event loop.

```C++
#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/bus.hpp>
#include <dbus-mockery/bindings/busy_loop.hpp>

#include <iostream>
#include <string>

#include <thread>
#include <chrono>

using namespace DBusMock;
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
	DBusMock::signal <void(object_path)> UserAdded;
	DBusMock::signal <void(object_path)> UserDeleted;
};

//----------------------------------------------------------------------------------------

// This step is necessary to enable interface auto-implementation.
// There is a limit to how many properterties and methods are possible. (currently either 64 or 255 each, haven't tried, assume 64)
// This limit can be circumvented by DBUS_MOCK_N. Which allows to mock the same interface more than once.
// A successory call to DBUS_MOCK_ZIP merges them all together.
DBUS_MOCK
(
    IAccounts,
    DBUS_MOCK_METHODS(CreateUser, DeleteUser),
    DBUS_MOCK_NO_PROPERTIES,
    DBUS_MOCK_SIGNALS(UserAdded, UserDeleted)
)

int main()
{
	auto bus = Bindings::open_system_bus();

	try
	{
		bus.install_event_loop(std::unique_ptr <Bindings::event_loop> (new Bindings::busy_loop(&bus, 50ms)));

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
		    [](Bindings::message&, std::string const& str) {
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
		    [](Bindings::message&, std::string const& str) {
			    // this is called when an error got signaled into our callback.
			    std::cerr << "oh no something gone wrong: " << str << "\n";
		    },
		    DBusMock::release_slot
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
```