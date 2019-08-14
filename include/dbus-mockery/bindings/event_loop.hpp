#pragma once

#include "bus_fwd.hpp"

namespace DBusMock
{
    /**
	 * @brief The event_loop interface has to be implemented by a custom event loop system, so it can be passed to a
	 *		  bus object and managed.
	 */
    class event_loop
	{
	public:
		/**
		 * @brief event_loop the event loop gets a handle to a bus connection object.
		 * @param bus
		 */
		event_loop(dbus* bus);

		/**
		 * @brief start Starts the event system.
		 */
		virtual void start() = 0;

		/**
		 * @brief stop Gracefully stops the event system.
		 */
		virtual void stop() = 0;

		/**
		 * @brief is_running Return true if running
		 * @return
		 */
		virtual bool is_running() const	= 0;

		virtual ~event_loop() = default;

	protected:
		dbus* bus;
	};
}
