#include <dbus-mockery/bindings/bus.hpp>
#include <dbus-mockery/bindings/message.hpp>

#include <dbus-mockery/bindings/detail/scope_exit.hpp>

#include <stdexcept>
#include <string>
#include <limits>
#include <chrono>

using namespace std::string_literals;

//#####################################################################################################################
int dbus_mock_signal_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    using namespace DBusMock::Bindings;

    auto* base = reinterpret_cast<slot_base*>(userdata);
    message msg{m};

    DBusMock::Bindings::detail::on_scope_exit lifetime_bound([&](){
        msg.release();
    });

    try
    {
        if (ret_error != nullptr && sd_bus_error_is_set(ret_error))
        {
            base->on_fail(msg, ret_error->message);
            sd_bus_error_free(ret_error);
        }
        else
            base->unpack_message(msg);
        return 1;
    }
    catch (std::exception const& exc)
    {
        base->on_fail(msg, exc.what());
        return -1;
    }
    catch (...)
    {
        base->on_fail(msg, "exception of unknown type was raised");
        return -1;
    }
}

int dbus_mock_async_callback(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    using namespace DBusMock::Bindings;
    auto* async_context = reinterpret_cast<async_context_base*>(userdata);
    message msg{m};

    detail::on_scope_exit lifetime_bound([&](){
        msg.release();
        async_context->owner()->free_async_context(async_context);
    });

    try
    {
        if (ret_error != nullptr && sd_bus_error_is_set(ret_error))
        {
            async_context->on_fail(msg, ret_error->message);
            sd_bus_error_free(ret_error);
        }
        else
            async_context->unpack_message(msg);
        return 1;
    }
    catch (std::exception const& exc)
    {
        async_context->on_fail(msg, exc.what());
        return -1;
    }
    catch (...)
    {
        return -1;
    }
}
//#####################################################################################################################

namespace DBusMock::Bindings
{
//#####################################################################################################################
    dbus open_system_bus()
    {
        sd_bus* bus;
        auto r = sd_bus_open_system(&bus);
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return dbus{bus};
    }
//---------------------------------------------------------------------------------------------------------------------
    dbus open_user_bus()
    {
        sd_bus* bus;
        auto r = sd_bus_open_user(&bus);
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return dbus{bus};
    }
//---------------------------------------------------------------------------------------------------------------------
    dbus open_system_bus(std::string const& host)
    {
        sd_bus* bus;
        auto r = sd_bus_open_system_remote(&bus, host.c_str());
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return dbus{bus};
    }
//---------------------------------------------------------------------------------------------------------------------
    dbus open_system_bus_machine(std::string const& machine)
    {
        sd_bus* bus;
        auto r = sd_bus_open_system_machine(&bus, machine.c_str());
        if (r < 0)
            throw std::runtime_error("cannot open system bus: "s + strerror(-r));
        return dbus{bus};
    }
//#####################################################################################################################
    dbus::dbus(sd_bus* bus)
        : bus_{bus}
        , unnamed_slots_{}
        , sdbus_lock_{}
        , event_loop_{nullptr}
        , async_slots_{}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    dbus::~dbus()
    {
        if (event_loop_ && event_loop_->is_running())
            event_loop_->stop();

        sd_bus_flush(bus_);
        sd_bus_close(bus_);
        sd_bus_unref(bus_);
    }
//---------------------------------------------------------------------------------------------------------------------
    void dbus::free_async_context(async_context_base* ac)
    {
        async_slots_.erase(ac);
    }
//---------------------------------------------------------------------------------------------------------------------
    void dbus::install_event_loop(std::unique_ptr <event_loop> esys)
    {
        event_loop_.reset(esys.release());
        if (!event_loop_->is_running())
            event_loop_->start();
    }
//---------------------------------------------------------------------------------------------------------------------
    void dbus::busy_loop(std::atomic <bool>* running)
    {
        using namespace std::chrono_literals;
        constexpr auto wait_timeout = 500ms;

        int r = 0;
        for (;running->load();)
        {
            sd_bus_message* m = nullptr;
            {
                std::scoped_lock guard{sdbus_lock_};
                r = sd_bus_process(bus_, &m);
            }
            if (m != nullptr)
            {
                message msg{m};
                //std::cout << "message processed: " << msg.comprehensible_type() << "\n";
            }
            if (r < 0 && r != -74 /*badmsg*/)
                throw std::runtime_error("error in bus processing: "s + strerror(-r) + " (" + std::to_string(r) + ")");

            if (r == 0)
            {
                r = sd_bus_wait(bus_, std::chrono::duration_cast <std::chrono::microseconds> (wait_timeout).count());
                if (r < 0)
                    throw std::runtime_error("error in bus waiting: "s + strerror(-r));
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    std::recursive_mutex& dbus::mutex()
    {
        return sdbus_lock_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void dbus::flush()
    {
        std::scoped_lock guard{sdbus_lock_};
        auto r = sd_bus_flush(bus_);
        if (r < 0)
            throw std::runtime_error("could not flush bus: "s + strerror(-r));
    }
//#####################################################################################################################
}
