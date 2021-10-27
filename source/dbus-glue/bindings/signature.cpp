#include <dbus-glue/bindings/signature.hpp>
#include <iostream>

namespace DBusMock
{
    std::ostream& operator<<(std::ostream& stream, signature const& sign)
    {
        stream << sign.string();
        return stream;
    }
}
