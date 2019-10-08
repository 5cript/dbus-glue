#include <dbus-mockery/bindings/exposables/exposable_property.hpp>
#include <dbus-mockery/bindings/detail/table_entry.hpp>
#include <dbus-mockery/bindings/detail/lambda_overload.hpp>

#include <iostream>

int dbus_mock_exposable_property_read
(
    sd_bus* /*bus*/,
    char const* /*path*/,
    char const* /*interface*/,
    char const* /*property*/,
    sd_bus_message* reply,
    void* userdata,
    sd_bus_error* /*error*/
)
{
    using namespace DBusMock;
    [[maybe_unused]] auto* entry = reinterpret_cast <detail::table_entry*>(userdata);

    int ret = 0;
    std::visit(
        detail::overloaded{
            [&ret](std::monostate const&){
                ret = -EINVAL;
            },
            [&ret](basic_exposable_method*) {
                ret = -EINVAL;
            },
            [reply, &ret](basic_exposable_property* property) {
                message msg{reply, true};
                ret = property->read(msg);
            }
        },
        entry->entry
    );

    return ret;
}

int dbus_mock_exposable_property_write
(
    sd_bus* /*bus*/,
    char const* /*path*/,
    char const* /*interface*/,
    char const* /*property*/,
    sd_bus_message* value,
    void* userdata,
    sd_bus_error* /*error*/
)
{
    using namespace DBusMock;
    [[maybe_unused]] auto* entry = reinterpret_cast <detail::table_entry*>(userdata);

    int ret = 0;
    std::visit(
        detail::overloaded{
            [&ret](std::monostate const&){
                ret = -EINVAL;
            },
            [&ret](basic_exposable_method*) {
                ret = -EINVAL;
            },
            [value, &ret](basic_exposable_property* property) {
                message msg{value, true};
                ret = property->write(msg);
            }
        },
        entry->entry
    );
    return 0;
}
