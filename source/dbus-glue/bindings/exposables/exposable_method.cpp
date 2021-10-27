#include <dbus-glue/bindings/exposables/exposable_method.hpp>
#include <dbus-glue/bindings/message.hpp>
#include <dbus-glue/bindings/detail/table_entry.hpp>
#include <dbus-glue/bindings/detail/lambda_overload.hpp>

#include <iostream>
#include <iomanip>

int dbus_mock_exposed_method_handler
(
    sd_bus_message *m,
    void *userdata,
    sd_bus_error*
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
	        [m, &ret](basic_exposable_method* method) {
	            message msg{m, true};
	            ret = method->call(msg);
	        },
	        [&ret](basic_exposable_property*) {
	            ret = -EINVAL;
	        },
	        [&ret](basic_exposable_signal*) {
	            ret = -EINVAL;
	        }
	    },
	    entry->entry
	);
	//return sd_bus_reply_method_return(m, "i", 2 * 3);
	return ret;
}

