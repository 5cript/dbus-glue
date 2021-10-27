#include <dbus-glue/bindings/event_loop.hpp>
#include <dbus-glue/bindings/bus.hpp>

namespace DBusGlue
{
    event_loop::event_loop(dbus* bus)
	    : bus{bus}
	{
	}
}
