#include <dbus-glue/generator/generator.hpp>
#include <dbus-glue/dbus_interface.hpp>
#include <dbus-glue/generator/introspect.hpp>

#include <boost/property_tree/xml_parser.hpp>

#include <sstream>
#include <iostream>
#include <regex>

namespace DBusMock::detail
{
    class IIntrospectable
	{
	public:
		virtual ~IIntrospectable() = default;

	public: // Methods
		virtual auto Introspect() -> std::string /*xml*/ = 0;

	public: // Properties

	public: // signals
	};
}

DBUS_MOCK_NAMESPACE
(
    (DBusMock)(detail),
    IIntrospectable,
    DBUS_MOCK_METHODS(Introspect),
    DBUS_MOCK_NO_PROPERTIES,
    DBUS_MOCK_NO_SIGNALS
)

namespace DBusMock
{
//#####################################################################################################################
	void interface_generator::generate_interface_from(
	    std::ostream& str,
	    dbus& bus,
	    std::string const& service,
	    std::string const& path,
	    std::string nspace_base
	)
	{
		// for now stream the xml.
		using namespace boost::property_tree;
		ptree tree;
		std::stringstream sstr;
		sstr << get_introspected_xml_from(bus, service, path);
		read_xml(sstr, tree);

		Introspect::Introspector intro;
		auto parsed = intro.parse(tree);
		intro.convert_types(parsed);
		intro.create_cpp(str, parsed, nspace_base + std::regex_replace(path, std::regex("/"), "::"));
		return;
	}
//---------------------------------------------------------------------------------------------------------------------
	std::string interface_generator::get_introspected_xml_from(
	    dbus& bus,
	    std::string const& service,
	    std::string const& path
	)
	{
		auto interface = create_interface <DBusMock::detail::IIntrospectable>
		(
		    bus,
		    service,
		    path,
		    "org.freedesktop.DBus.Introspectable"
		);
		return interface.Introspect();
	}
//---------------------------------------------------------------------------------------------------------------------
	void interface_generator::write_introspected_xml_to(
	    std::string const& file_name,
	    dbus& bus,
	    std::string const& service,
	    std::string const& path
	)
	{
		using namespace boost::property_tree;
		ptree tree;
		std::stringstream sstr;
		sstr << get_introspected_xml_from(bus, service, path);
		read_xml(sstr, tree);
		write_xml(file_name, tree, std::locale(), xml_writer_make_settings <std::string> (' ', 4));
	}
//#####################################################################################################################
}
