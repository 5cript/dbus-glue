#include <dbus-mockery/bindings/exposed_method.hpp>
#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/detail/table_entry.hpp>

#include <iostream>
#include <iomanip>

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

    [[maybe_unused]] auto* entry = reinterpret_cast <detail::table_entry*>(userdata);
    int ret = 0;
    std::visit(
        overloaded{
            [](std::monostate const&){
                std::cout << "std::monostate? shouldn't be\n";
            },
            [m, &ret](basic_exposed_method* method) {
                message msg{m, true};
                ret = method->call(msg);
            }
        },
        entry->entry
    );
    //return sd_bus_reply_method_return(m, "i", 2 * 3);
    return ret;
}
