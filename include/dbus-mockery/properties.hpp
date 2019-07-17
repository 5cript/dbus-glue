#pragma once

#include "dbus_interface_base.hpp"

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
        property(Mocks::interface_mock_base* iface)
            : iface_{iface}
        {
        }

    private:
        Mocks::interface_mock_base* iface_;
    };

    template <typename T>
    class readable : public property <T>
    {
    public:
        using type = T;

        using property <T>::property;

    private:
    };

    template <typename T>
    class writable : public property <T>
    {
    public:
        using type = T;

        using property <T>::property;

    private:
    };

    template <typename T>
    struct read_writeable : public property <T>
    {
    public:
        using type = T;

        using property <T>::property;

    private:
    };
}
