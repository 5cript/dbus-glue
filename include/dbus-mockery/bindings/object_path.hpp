#pragma once

#include <string>
#include <iosfwd>

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

        object_path& operator=(object_path const&) = default;
        object_path& operator=(object_path&&) = default;
        object_path(object_path const&) = default;
        object_path(object_path&&) = default;

        object_path& operator=(std::string const& str)
        {
            data_ = str;
            return *this;
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

    std::ostream& operator<<(std::ostream& stream, object_path const& opath);
}
