# DBus Mockery

Currently a work in Progress.

## Summary
The DBus Mockery library wants to make interfacing with a DBus Service / Interface almost as simple as defining a C++ interface.
The DBus protocol is almost entirely hidden from the user. 
This library only enables interfacing with existant DBus APIs. Creating one yourself with this library is currently not planned.

Example interfaces / Predefined interfaces can be found in my repository [dbus-mockery-system]("https://github.com/5cript/dbus-mockery-system").

## Roadmap
Here is a checkbox list of all the tings to come and all that are done so far.

- [x] A bus object for all necessary bus related tasks.
    - [x] Handle the bus lifetime
    - [x] Calling methods
    - [x] Reading properties
    - [x] Writing properties
    - [x] Listening for signals
    - [x] Starting an event loop
    - [ ] Inspecting interfaces
- [x] A Macro for declaring DBus interfaces as C++ interfaces.
- [x] Attaching or creating a (simple) event loop to the bus object.
(Note: If you use the sd_event* object system, you still have to setup and teardown the event stuff yourself, this is not wrapped by this library.)
- [x] A rudamentary generator for interfaces. Not really necessary, since writing a simple class declaration is easy and fast, but can be used to get a quick start for big interfaces.

On a created interface, linked to a given DBus interface, you can:
- [x] Call methods.
- [x] Read and Write Properties.
- [ ] Connect to slots and listen for signals.
- [ ] Call methods and get the results asnchronously.
- [ ] Read and Write Properties asynchronously.

##### Improvements not planed, but sensible:
- [ ] Wrap sd_event in another library and make it interact with this.

## Build
(Currently the cmake in the project builds a binary executable, this will be changed as soon as I deem it ready to be used.)
This project uses cmake.
##### Dependencies:
- libsystemd, because the systemd sd-bus library is used underneath.
- boost preprocessor

## Examples
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
Not that signal handling requires an event loop.

```C++
#include <dbus-mockery/bindings/bus.hpp>

int main()
{
    using namespace DBusMock;

    auto bus = Bindings::open_system_bus();

    // TODO IMPROVE, shall be callable from mocked interface
    // Note that this call can throw an exception, so you should catch it.
    // Omitted for brevity reasons.
    bus.install_signal_listener <void(object path)> (
        "org.freedesktop.Accounts",
        "/org/freedesktop/Accounts",
        "org.freedesktop.Accounts",
        "UserAdded",
        [](object_path const& p) {
            std::cout << "callback - create: " << p << std::endl;
        },
        [](Bindings::message&, std::string const& str) {
            std::cerr << "oh no something gone wrong: " << str << "\n";
        }
    );

    // TODO IMPROVE: Make the "simple" event loop also part of the bus object:
    std::atomic <bool> running{true};
    std::thread loop{[&](){
        try {
            bus.busy_loop(&running);
        } catch (std::exception const& exc) {
            std::cout << exc.what() << std::endl;
        }
    }};

    std::cin.get(); // artifical pause, so the loop doesnt die.
    running.store(false);
    if (loop.joinable())
        loop.join();
}
```
