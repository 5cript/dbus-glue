#include <dbus-glue/bindings/busy_loop.hpp>
#include <dbus-glue/bindings/sdbus_core.hpp>
#include <dbus-glue/bindings/bus.hpp>

#include <mutex>

namespace DBusMock
{
//#####################################################################################################################
    busy_loop::busy_loop(dbus* bus_object, std::chrono::microseconds idle_wait_delay)
        : event_loop{bus_object}
        , loop_thread_{}
        , idle_wait_delay_{std::move(idle_wait_delay)}
        , error_cb_{}
        , message_cb_{}
        , actually_running_{false}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void busy_loop::start()
    {
        using namespace std::string_literals;
        running_.store(true);
        actually_running_.store(true);

        if (loop_thread_.joinable())
            stop();

        loop_thread_ = std::thread{[this]()
        {
            int r{0};
            sd_bus_message* m = nullptr;
            for (;running_.load();)
            {
                r = 0;
                m = nullptr;
                {
                    std::scoped_lock guard{bus->mutex()};
                    r = sd_bus_process(static_cast <sd_bus*> (*bus), &m);
                }
                if (m != nullptr)
                {
                    // ! Dont move this into the if, the message must be disposed off. (destructor has side effect)
                    message msg{m};
                    if (message_cb_)
                        message_cb_(msg);
                }
                if (r < 0)
                {
                    if (error_cb_)
                    {
                        if (!error_cb_(r, "error in event loop processing: "s + strerror(-r)))
                            break;
                    }
                    else
                        throw std::runtime_error("error in event loop processing: "s + strerror(-r));
                }
                else if (r == 0)
                {
                    r = sd_bus_wait(static_cast <sd_bus*> (*bus), static_cast <uint64_t> (idle_wait_delay_.count()));
                    if (r < 0)
                    {
                        if (error_cb_)
                        {
                            if (!error_cb_(r, "error in event loop waiting: "s + strerror(-r)))
                                break;
                        }
                        else
                            throw std::runtime_error("error in event loop waiting: "s + strerror(-r));
                    }
                }
            }
            actually_running_ = false;
        }};
    }
//---------------------------------------------------------------------------------------------------------------------
    bool busy_loop::is_running() const
    {
        return actually_running_.load();
    }
//---------------------------------------------------------------------------------------------------------------------
    void busy_loop::message_callback(std::function <void(message& msg)> const& cb)
    {
        message_cb_ = cb;
    }
//---------------------------------------------------------------------------------------------------------------------
    void busy_loop::error_callback(std::function <bool(int code, std::string const& message)> const& cb)
    {
        error_cb_ = cb;
    }
//---------------------------------------------------------------------------------------------------------------------
    void busy_loop::stop()
    {
        running_.store(false);
        if (loop_thread_.joinable())
            loop_thread_.join();
    }
//#####################################################################################################################
    void make_busy_loop(dbus* bus_object, std::chrono::microseconds idle_wait_delay)
    {
        bus_object->install_event_loop
        (
            std::make_unique <busy_loop> (bus_object, idle_wait_delay)
        );
    }
//#####################################################################################################################
}
