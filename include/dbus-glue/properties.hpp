#pragma once

#include "dbus_interface_base.hpp"

#include <iostream>

namespace DBusGlue
{
    template <typename...>
    struct properties
    {};

    template <typename T>
    struct property
    {
      public:
        property(char const* name)
            : name_{name}
        {}

        void set_base(std::weak_ptr<Mocks::interface_mock_base> base)
        {
            iface_ = std::move(base);
        }

        virtual ~property() = default;

      protected:
        std::weak_ptr<Mocks::interface_mock_base> iface_;
        char const* name_;
    };

    template <typename T>
    class readable : public virtual property<T>
    {
      public:
        using type = T;

        using property<T>::property;

        T get() const
        {
            type t;
            if (auto iface = property<T>::iface_.lock(); iface)
                property<T>::iface_->read_property(property<T>::name_, t);
            else
                throw std::runtime_error{"The interface_mock_base has been destroyed"};
            return t;
        }

        auto get(async_flag_t const&) const
        {
            if (auto iface = property<T>::iface_.lock(); iface)
            {
                Mocks::interface_async_property_proxy<T()> prox{iface, property<T>::name_};
                return prox;
            }
            else
                throw std::runtime_error{"The interface_mock_base has been destroyed"};
        }

        bool operator==(T const& t)
        {
            return get() == t;
        }

        operator T() const
        {
            return get();
        }

      private:
    };

    template <typename U, class = decltype(std::declval<std::ostream&>() << std::declval<U>())>
    std::ostream& operator<<(std::ostream& stream, readable<U> const& read)
    {
        stream << read.get();
        return stream;
    }

    template <typename T>
    class writable : public virtual property<T>
    {
      public:
        using type = T;

        using property<T>::property;

        void set(type const& var)
        {
            if (auto iface = property<T>::iface_.lock(); iface)
                iface->write_property(property<T>::name_, var);
            else
                throw std::runtime_error{"The interface_mock_base has been destroyed"};
        }

        auto set(async_flag_t const&, type const& var) const
        {
            if (auto iface = property<T>::iface_.lock(); iface)
            {
                Mocks::interface_async_property_proxy<void(type)> prox{iface, property<T>::name_};
                prox.bind_parameters(var);
                return prox;
            }
            else
                throw std::runtime_error{"The interface_mock_base has been destroyed"};
        }

        writable& operator=(type const& var)
        {
            set(var);
            return *this;
        }

      private:
    };

    template <typename U, class = decltype(std::declval<std::ostream&>() >> std::declval<U>())>
    std::istream& operator>>(std::istream& stream, writable<U> const& write)
    {
        U var;
        stream >> var;
        write.set(var);
        return stream;
    }

    template <typename T>
    struct read_writeable
        : public readable<T>
        , public writable<T>
    {
        using type = T;
        using writable<T>::writable;
        using readable<T>::readable;

        read_writeable& operator=(type const& var)
        {
            writable<T>::set(var);
            return *this;
        }

        bool operator==(T const& t)
        {
            return readable<T>::get() == t;
        }

        operator T() const
        {
            return readable<T>::get();
        }
    };

    template <typename U, class = decltype(std::declval<std::ostream&>() << std::declval<U>())>
    std::ostream& operator<<(std::ostream& stream, read_writeable<U> const& read)
    {
        stream << read.get();
        return stream;
    }

    template <typename U, class = decltype(std::declval<std::ostream&>() >> std::declval<U>())>
    std::istream& operator>>(std::istream& stream, read_writeable<U> const& write)
    {
        U var;
        stream >> var;
        write.set(var);
        return stream;
    }
}
