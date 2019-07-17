#include <dbus-mockery/bindings/object_path.hpp>
#include <iostream>

namespace DBusMock
{
    std::ostream& operator<<(std::ostream& stream, object_path const& opath)
    {
        stream << opath.string();
        return stream;
    }
}
