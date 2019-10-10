# DBus Mockery

## Table of Contents
* [Summary](#Summary)
* [Documentation](#Documentation)
* [Roadmap](#Roadmap)
* [Build](#Build)
* [Tutorial (Remote Interfaces)](#Tutorial_Remote)
* [Tutorial (Local Interfaces)](#Tutorial_Local)
* [Examples](#Examples)

## Summary
The DBus Mockery library wants to make interfacing with a DBus Service / Interface almost as simple as defining a C++ interface.
The DBus protocol is almost entirely hidden from the user. 
This library only enables interfacing with existant DBus APIs. Creating one yourself with this library **is** ~~not~~ planned!

Example interfaces / Predefined interfaces can be found in my repository [dbus-mockery-system]("https://github.com/5cript/dbus-mockery-system").

## Documentation (Doxygen)
https://5cript.github.io/dbus-mockery/

## Roadmap / Features
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

With your own registered interface you can:
- [x] Declare your interface
- [x] Expose the interface
- [x] Expose a method
- [x] Expose a property (read, read/write)
- [x] Expose a signal
- [ ] Make exposed signal emittable

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

## Tutorial_Remote
This is a short tutorial for declaring external interfaces, attaching to them and using them.

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
    auto bus = open_system_bus();

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
// new required header for the event loop
#include <dbus-mockery/bindings/busy_loop.hpp>

int main()
{
    using namespace DBusMock;
    using namespace std::chrono_literals;

    // open the system bus
    auto bus = open_system_bus();

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

#### Variants
Variants are containers that can contain a number of types.
In order to read out of a variant, you have to know its stored type first. Luckily sdbus can deliver this information.
One small note: due to implementation restrictions, loading and storing a value from a variant can only be done by a free function and not by a member function.
This would look like follows:

```C++
// Example for reading a variant.

#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/variant_helpers.hpp>
#include <iostream>

int main()
{
    auto bus = open_system_bus();

    // a variant dictionary is a map of variants.
    variant_dictionary<std::map> dict;
    bus.read_properties
    (
        "org.freedesktop.Accounts",
        "/org/freedesktop/Accounts",
        "org.freedesktop.Accounts",
        dict
    );

    // In case that you dont know the type beforehand, but have to test first:
    auto descriptor = dict["DaemonVersion"].type();

    // print it in a somewhat readable way.
    std::cout << descriptor.string() << "\n";

    std::string ver, ver2;
    variant_load<std::string>(dict["DaemonVersion"], ver);
    std::cout << ver << "\n";

    // You do NOT need to rewind a variant before rereading it, this is done for you.
    // dict["DaemonVersion"].rewind();
    variant_load<std::string>(dict["DaemonVersion"], ver2);
    std::cout << ver2 << "\n";
}
```

```C++
// Example for writing to a variant.

#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/variant_helpers.hpp>
#include <iostream>

using namespace std::string_literals;

int main()
{
    auto bus = open_system_bus();

    variant var;
    // has to be non-member unfortunately
    variant_store(bus, var, "hello"s);

    std::string val;
    variant_load(var, val);
    std::cout << val << "\n"; // -> hello

    std::cout << std::flush;
    return 0;
}
```

## Tutorial_Local
This is the tutorial for declaring your own dbus interface.

On contrary to interfaces that are externally supplied, there
is no adapt macro to use. The reason is that a macro to provide all
the parameter and return value names on top of method names etc. would be very verbose and hard to debug on errors.

#### Declaring an interface
Here we have an interface that we want to expose to the world:
```C++
#include <dbus-mockery/bindings/exposable_interface.hpp>

// Your interface to export has to derive from exposable_interface.
class MyInterface : public DBusMock::exposable_interface
{
public:
    // these members determine the path and service name
    // for registration, do not need to be const literals.
    std::string path() const override
    {
        return "/bluetooth";
    }
    std::string service() const override
    {
        return "com.bla.my";
    }

public: // Methods
    auto DisplayText(std::string const& text) -> void {}
    auto Multiply(int lhs, int rhs) -> int {return lhs * rhs;}

public: // Properties
    bool IsThisCool;

public: // Signals
    // this is called emitable, not signal as in adapted interfaces.
    emitable <void(*)(int)> FireMe{this, "FireMe"};
};
```

#### Exposing / Registering the interface on the bus
```C++
#include <dbus-mockery/interface_builder.hpp>

int main()
{
    auto bus = open_user_bus();

    using namespace DBusMock;
    using namespace ExposeHelpers;    

    // creates an instance of MyInterface that can be used.
    auto shared_ptr_to_interface = make_interface <MyInterface>(
        // Multiply Method
        DBusMock::exposable_method_factory{} <<
            name("Multiply") << // Method name
            result("Product") << // Name can only be a single word!
            parameter(0, "a") << // Name can only be a single word!
            parameter(1, "b") << // Parameter number is optional, but if supplied, all should have it supplied. 
            as(&MyInterface::Multiply),

        // Display Text Method
        DBusMock::exposable_method_factory{} <<
            name("DisplayText") <<
            result("Nothing") <<
            parameter("text") <<
            as(&MyInterface::DisplayText),

        // IsThisCool Property
        DBusMock::exposable_property_factory{} <<
            name("IsThisCool") <<
            writeable(true) <<
            as(&MyInterface::IsThisCool),

        // FireMe Signal
        DBusMock::exposable_signal_factory{} <<
            // name is in emitable constructor, not needed here.
            // d-feet does not show signal parameter names
            parameter("integral") <<
            as(&MyInterface::FireMe)   
    );

    // The bus takes a share to hold the interface and exposes it on the bus.
    auto result = bus.expose_interface(shared_ptr_to_interface);

    if (result < 0)
    {
        std::cerr << strerror(-result);
        return 1;
    }

    result = sd_bus_request_name(static_cast <sd_bus*> (bus), "com.bla.my", 0);

    if (result < 0)
    {
        std::cerr << strerror(-result);
        return 1;
    }

    make_busy_loop(&bus, 200ms);
    bus.loop <busy_loop>()->error_callback([](int, std::string const& msg){
        std::cerr << msg << std::endl;
        return true;
    });

    // emit signal
    exposed->FireMe.emit(5);

    // prevent immediate exit here however you like.
    std::cin.get();
}
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
    auto bus = open_system_bus();

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
