#include <dbus-glue/bindings/signature.hpp>
#include <iostream>

namespace DBusGlue
{
    std::ostream& operator<<(std::ostream& stream, signature const& sign)
    {
        stream << sign.string();
        return stream;
    }
}
