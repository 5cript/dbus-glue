#include <dbus-mockery/generator/generator.hpp>
#include <dbus-mockery-system/accounts/accounts.hpp>

#include <iostream>
#include <fstream>
#include <string>

#include <thread>
#include <chrono>

using namespace DBusMock;

int main()
{
    auto bus = Bindings::open_system_bus();

    try
    {
        bus.install_signal_listener <void(object_path)> (
            "org.freedesktop.Accounts",
            "/org/freedesktop/Accounts",
            "org.freedesktop.Accounts",
            "UserAdded",
            [](object_path p) {
                std::cout << p << "\n";
            }
        );

        std::atomic <bool> running{true};
        std::thread t{[&running, &bus](){
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);



            std::this_thread::sleep_for(60s);
            running.store(false);
        }};
        t.detach();
        bus.busy_loop(running);
    }
    catch (std::exception const& exc)
    {
        std::cout << exc.what() << "\n";
        std::cout << exc.what() << "\n";
    }

    return 0;
}
