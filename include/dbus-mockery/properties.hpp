#pragma once

#include "dbus_interface_base.hpp"

#include <iostream>

namespace DBusMock
{
    template <typename...>
    struct properties
    {
    };

    template <typename T>
    struct property
    {
    public:
        property(Mocks::interface_mock_base* iface, char const* name)
            : iface_{iface}
            , name_{name}
        {
        }

        virtual ~property() = default;

    protected:
        Mocks::interface_mock_base* iface_;
        char const* name_;
    };

    template <typename T>
    class readable : public virtual property <T>
    {
    public:
        using type = T;

        using property <T>::property;

        T get() const
        {
            type t;
            property<T>::iface_->read_property(property<T>::name_, t);
            return t;
        }

        explicit operator T() const
        {
            return get();
        }

    private:
    };

    template <typename U,
              class = decltype(std::declval<std::ostream&>() << std::declval<U>())>
    std::ostream& operator<<(std::ostream& stream, readable <U> const& read)
    {
        stream << read.get();
        return stream;
    }

    template <typename T>
    class writable : public virtual property <T>
    {
    public:
        using type = T;

        using property <T>::property;

        void set(type const& var)
        {
            property<T>::iface_->write_property(property<T>::name_, var);
        }

        writable& operator=(type const& var)
        {
            set(var);
            return *this;
        }
    private:
    };

    template <typename U,
              class = decltype(std::declval<std::ostream&>() >> std::declval<U>())>
    std::istream& operator>>(std::istream& stream, writable <U> const& write)
    {
        U var;
        stream >> var;
        write.set(var);
        return stream;
    }

    template <typename T>
    struct read_writeable : public readable <T>
                          , public writable <T>
    {
        using type = T;
        using property <T>::property;
    };
}
