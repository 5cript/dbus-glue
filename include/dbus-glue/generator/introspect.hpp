#pragma once

#include <boost/property_tree/ptree.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <iostream>

namespace DBusGlue::Introspect
{
    struct argument
	{
		std::string name;
		std::string type;

		// not relevant for signals, since signals obviously dont take parameters, but pass them
		std::string direction;
	};

	struct property
	{
		std::string name;
		std::string type; // dbus type id
		std::string access; // read, write, readwrite
	};

	struct method
	{
		std::string name;
		std::vector <argument> arguments;
	};

	struct signal
	{
		std::string name;
		std::vector <argument> arguments;
	};

	struct interface
	{
		std::string name;
		std::vector <method> methods;
		std::vector <signal> signals;
		std::vector <property> properties;
	};

	struct object
	{
		std::string name;
		std::vector <std::string> members;
	};

	class Introspector
	{
	public:
		Introspector(
		    std::string dict_type = "unordered_map",
		    std::string container_type = "vector",
		    bool space_before_template = true,
		    bool space_after_comma = true
		);

		std::vector <interface> parse(boost::property_tree::ptree const& tree);

		/**
		 * @brief convert_type Converts a single dbus type to C++ type.
		 * @param dbus_type
		 * @return
		 */
		std::string convert_type(std::string const& dbus_type);

		/**
		 * @brief convert_types Converts all the dbus types to C++ types.
		 * @param faces
		 */
		void convert_types(std::vector <interface>& faces);

		/**
		 * @brief create_cpp Writes C++ into stream from the interface vector.
		 * @param stream
		 * @param faces
		 */
		void create_cpp(std::ostream& stream, std::vector <interface>& faces, std::string const& nspace);

	private:
		/**
		 * @brief convert_type Converts a single BASIC dbus type to C++ type, if it is one.
		 * @param dbus_type
		 * @return A C++ type as string, std::nullopt if not convertible
		 */
		std::optional <std::string> convert_basic_type(char basic, bool& append_ref);

	private:
		std::string space_b4_template() const;
		std::string space_aft_comma() const;
		std::string generate_struct_name() const;

	private:
		std::unordered_map <std::string, object> objects;
		std::string dict_type;
		std::string container_type;
		bool space_before_template;
		bool space_after_comma;
	};
}
