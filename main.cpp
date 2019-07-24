#include <dbus-mockery/generator/generator.hpp>
#include <dbus-mockery-system/accounts/accounts.hpp>

#include <iostream>
#include <fstream>
#include <string>

#include <thread>
#include <chrono>

using namespace DBusMock;
using namespace std::chrono_literals;

int main()
{
    std::cout << "osbus" << std::endl;

    auto bus = Bindings::open_system_bus();

    try
    {
        std::cout << "install 1" << std::endl;

        auto accountControl = create_interface <Accounts::org::freedesktop::Accounts::Accounts>(
            bus,
            "org.freedesktop.Accounts",
            "/org/freedesktop/Accounts",
            "org.freedesktop.Accounts"
        );

        bus.install_signal_listener <void(object_path)> (
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

        std::cout << "install 2" << std::endl;

        bus.install_signal_listener <void(object_path)> (
            "org.freedesktop.Accounts",
            "/org/freedesktop/Accounts",
            "org.freedesktop.Accounts",
            "UserDeleted",
            [&](object_path const& p) {
                std::cout << "callback - delete: " << p << std::endl;

                std::cout << "before create" << std::endl;

                auto path = accountControl.CreateUser("tempus", "tempus", 0);
            },
            [](Bindings::message&, std::string const& str) {
                std::cerr << "oh no something gone wrong: " << str << "\n";
            }
        );

        std::cout << "before thread_loop" << std::endl;

        std::atomic <bool> running{true};
        std::thread loop{[&](){
            try {
                bus.busy_loop(&running);
            } catch (std::exception const& exc) {
                std::cout << exc.what() << std::endl;
            }
        }};

        std::cout << "before delete" << std::endl;

        try {
            accountControl.DeleteUser(1001, false);
        } catch (std::exception const& exc) {
            accountControl.CreateUser("tempus", "tempus", 0);
            std::cout << exc.what() << std::endl;
        }
        bus.flush();

        std::cout << "before enter" << std::endl;

        std::cin.get();
        running.store(false);
        if (loop.joinable())
            loop.join();
    }
    catch (std::exception const& exc)
    {
        std::cout << exc.what() << "\n";
    }

    return 0;
}
