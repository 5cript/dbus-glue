#include <dbus-mockery/bindings/event_loop.hpp>
#include <dbus-mockery/bindings/bus.hpp>

namespace DBusMock::Bindings
{
    event_loop::event_loop(dbus* bus)
        : bus{bus}
    {
    }
}
