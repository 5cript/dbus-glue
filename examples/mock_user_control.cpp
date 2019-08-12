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
