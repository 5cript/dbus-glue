#pragma once

#include "event_loop.hpp"
#include "message.hpp"

#include <chrono>
#include <thread>
#include <functional>
#include <atomic>

namespace DBusMock
{
    /**
	 * @brief The busy_loop class is a basic polling loop that handles queued dbus operations.
	 *		  it calls sd_bus_process and sd_bus_wait in a loop.
	 */
    class busy_loop : public event_loop
	{
	public:
		/**
		 * @brief busy_loop
		 * @param bus
		 * @param idle_wait_delay The maximum wait for new queued actions on the bus queue.
		 *		  Be warned, that this delay is also the worst case delay on stop().
		 *		  Setting the value too low causes a lot of load.
		 */
		busy_loop(dbus* bus_object, std::chrono::microseconds idle_wait_delay);

		/**
		 * @brief error_callback Set an error callback that is called when an error occurs in the event loop.
		 *						 The return of this function determines wheter the loop shall continue or not.
		 *						 true = continue, false = stop!
		 *						 Be warned, that if you dont use this function and set a callback, the loop will
		 *						 raise an exception within a thread and call std::terminate as a consequence.
		 *						 This is intentional, because silent failings are worse than total teardowns.
		 * @param cb The callback function.
		 */
		void error_callback(std::function <bool(int code, std::string const& message)> const& cb);

		/**
		 * @brief message_callback Called when a message was processed and is now available on the bus.
		 * @param cb a given cb.
		 */
		void message_callback(std::function <void(message& msg)> const& cb);

		/**
		 * @brief starts the busy loop
		 */
		void start() override;

		/**
		 * @brief stops the busy loop and waits for graceful completion. May take up to idle_wait_delay time.
		 */
		void stop() override;

		/**
		 * @brief is_running Returns true if running.
		 * @return
		 */
		bool is_running() const	override;

	private:
		std::thread loop_thread_;
		std::chrono::microseconds idle_wait_delay_;
		std::atomic <bool> running_;
		std::function <bool(int code, std::string const& message)> error_cb_;
		std::function <void(message& msg)> message_cb_;

		// this variable is necessary, because running_ can be false, even when the thread is not joined yet.
		std::atomic <bool> actually_running_;
	};

	void make_busy_loop(dbus* bus_object, std::chrono::microseconds idle_wait_delay = std::chrono::milliseconds(50));
}
