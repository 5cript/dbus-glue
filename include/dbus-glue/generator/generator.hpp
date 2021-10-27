#pragma once

#include "../bindings/bus.hpp"
#include <iosfwd>

namespace DBusGlue
{
    class interface_generator
	{
	public:
		/**
		 * @brief generate_interface_from Generate a C++ Interface and DBus_Mock implemenation from the given path
		 * @param str A stream to output the results to
		 * @param bus A bus.
		 * @param service What service
		 * @param path What path in that service
		 */
		void generate_interface_from(
		    std::ostream& str,
		    dbus& bus,
		    std::string const& service,
		    std::string const& path,
		    std::string nspace_base
		);

		/**
		 * @brief get_introspected_xml_from Calls the introspect method on the introspection interface.
		 * @param bus The bus
		 * @param service The service to look at
		 * @param and which of the paths of that service.
		 * @return XML from org.freedesktop.DBUS.Introspectable
		 */
		std::string get_introspected_xml_from(
		    dbus& bus,
		    std::string const& service,
		    std::string const& path
		);

		void write_introspected_xml_to(
		    std::string const& file_name,
		    dbus& bus,
		    std::string const& service,
		    std::string const& path
		);

	private:
	};
}
