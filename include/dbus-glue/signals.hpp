#pragma once

#include "dbus_interface_base.hpp"

#include <memory>

namespace DBusGlue
{
    namespace Detail
    {
        template <typename T>
        struct signature_format_is_correct
        {
            constexpr static bool value = false;
        };
        template <typename... Args>
        struct signature_format_is_correct<void(Args...)>
        {
            constexpr static bool value = true;
        };
    }

    template <typename Signature>
    struct signal
    {
        static_assert(
            Detail::signature_format_is_correct<Signature>::value,
            "The signature of your signal is unexpected, expected signal<void(Ts...)>");
    };

    struct release_slot_t
    {
        bool do_release;
    };

    constexpr static auto release_slot = release_slot_t{true};

    /**
     *	@brief This class is only for external interfaces that get adapted. Should probably called slot.
     */
    template <typename... Args>
    class signal<void(Args...)>
    {
      public:
        auto listen(
            std::function<void(Args const&...)> cb,
            std::function<void(message&, std::string const&)> const& err,
            release_slot_t release = release_slot_t{false})
        {
            if (auto base = base_.lock(); base)
                return base->install_signal_listener(name_, cb, err, release.do_release);
            else
                throw std::runtime_error{"The interface_mock_base has been destroyed"};
        }

        signal(char const* name)
            : name_{name}
        {}

        void set_base(std::weak_ptr<Mocks::interface_mock_base> base)
        {
            base_ = std::move(base);
        }

      private:
        std::weak_ptr<Mocks::interface_mock_base> base_;
        char const* name_;
    };
}
