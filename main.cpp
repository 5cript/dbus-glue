#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/sdbus_core.hpp>
#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/bus.hpp>
#include <dbus-mockery/bindings/struct_adapter.hpp>

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>

using namespace DBusMock;

class IAccounts
{
public:
    virtual ~IAccounts() = default;

    virtual auto CacheUser(std::string const& name) const -> object_path = 0;
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
    auto bus = Bindings::open_system_bus();

    try
    {
        //auto accounts = create_interface <IAccounts> ();
        //accounts.CreateUser("DBUS_USER", "DBUS_USER", 0);

        auto user_control = create_interface <IAccounts> (bus, "org.freedesktop.Accounts", "/org/freedesktop/Accounts", "org.freedesktop.Accounts");
        user_control.CreateUser("hello", "hello", 0);

        std::cout << user_control.DaemonVersion << "\n";
    }
    catch (std::exception const& exc)
    {
        std::cout << exc.what() << "\n";
    }

    std::cout << std::flush;

    return 0;
}
