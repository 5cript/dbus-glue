#pragma once

#include <string>

// TODO:
// Implement restriction checking: https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-marshaling-object-path

namespace DBusMock
{
    class object_path
    {
    private:
        std::string data_;

    public:
        object_path() noexcept
            : data_{}
        {
        }

        explicit operator std::string() const noexcept
        {
            return data_;
        }

        explicit object_path(std::string const& data)
            : data_{data}
        {
        }

        explicit object_path(char const* data)
            : data_{data}
        {
        }

        std::string string() const
        {
            return data_;
        }
    };
}
