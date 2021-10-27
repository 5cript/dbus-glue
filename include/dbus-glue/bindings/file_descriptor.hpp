#pragma once

namespace DBusMock
{
    class file_descriptor
    {
    private:
        int fd_;

    public:
        file_descriptor() noexcept
            : fd_{}
        {
        }

        file_descriptor& operator=(int fd) noexcept
        {
            fd_ = fd;
            return *this;
        }

        file_descriptor& operator=(file_descriptor const&) = default;
        file_descriptor& operator=(file_descriptor&&) = default;
        file_descriptor(file_descriptor const&) = default;
        file_descriptor(file_descriptor&&) = default;

        explicit operator int() const noexcept
        {
            return fd_;
        }

        explicit file_descriptor(int fd)
            : fd_{fd}
        {
        }

        int descriptor() const
        {
            return fd_;
        }
    };
}
