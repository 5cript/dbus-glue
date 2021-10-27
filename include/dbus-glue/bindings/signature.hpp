#pragma once

#include <string>

namespace DBusMock
{
    class signature
    {
    private:
        std::string data_;

    public:
        signature() noexcept
            : data_{}
        {
        }

        explicit operator std::string() const noexcept
        {
            return data_;
        }

        explicit signature(std::string const& data)
            : data_{data}
        {
        }

        explicit signature(char const* data)
            : data_{data}
        {
        }

        signature& operator=(signature const&) = default;
        signature& operator=(signature&&) = default;
        signature(signature const&) = default;
        signature(signature&&) = default;

        signature& operator=(std::string const& str)
        {
            data_ = str;
            return *this;
        }

        std::string string() const
        {
            return data_;
        }

        char const* c_str() const
        {
            return data_.c_str();
        }
    };

    std::ostream& operator<<(std::ostream& stream, signature const& sign);
}
