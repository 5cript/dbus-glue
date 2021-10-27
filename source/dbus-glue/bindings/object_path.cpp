#include <dbus-glue/bindings/object_path.hpp>
#include <iostream>

namespace DBusGlue
{
    std::ostream& operator<<(std::ostream& stream, object_path const& opath)
    {
        stream << opath.string();
        return stream;
    }
}
