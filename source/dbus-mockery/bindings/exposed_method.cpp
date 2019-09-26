#include <dbus-mockery/bindings/exposed_method.hpp>
#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/detail/table_entry.hpp>

#include <iostream>

namespace {
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}

int dbus_mock_exposed_method_handler
(
    sd_bus_message *m,
    void *userdata,
    [[maybe_unused]] sd_bus_error *error
)
{
    using namespace DBusMock;

    auto* entry = reinterpret_cast <detail::table_entry*>(userdata);
    std::visit(
        overloaded{
            [](std::monostate const&){
                std::cout << "std::monostate? shouldn't be\n";
            },
            [m](basic_exposed_method* method) {
                message msg{m, true};
                std::cout << "method!\n";
            }
        },
        entry->entry
    );
    return sd_bus_reply_method_return(m, "i", 2 * 3);
}
