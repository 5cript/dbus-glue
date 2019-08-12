#include <dbus-mockery/generator/introspect.hpp>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <regex>

namespace DBusMock::Introspect
{
    using namespace std::string_literals;
//#####################################################################################################################
    Introspector::Introspector(
            std::string dict_type,
            std::string container_type,
            bool space_before_template,
            bool space_after_comma
        )
        : objects{}
        , dict_type{std::move(dict_type)}
        , container_type{std::move(container_type)}
        , space_before_template{space_before_template}
        , space_after_comma{space_after_comma}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    std::vector <interface> Introspector::parse(boost::property_tree::ptree const& tree)
    {
        using namespace boost::property_tree;

        auto ifaces = tree.get_child("node");
        std::vector <interface> faces;
        for (auto const& i : ifaces)
        {
            if (i.first == "interface")
            {
                interface face;
                face.name = i.second.get <std::string> ("<xmlattr>.name");

                for (auto const& j: i.second)
                {
                    if (j.first == "<xmlattr>")
                        continue;

                    if (j.first == "method")
                    {
                        method meth;
                        meth.name = j.second.get <std::string> ("<xmlattr>.name");

                        for (auto const& k : j.second)
                        {
                            if (k.first == "<xmlattr>")
                                continue;

                            if (k.first == "arg")
                            {
                                argument arg;
                                arg.name = k.second.get <std::string> ("<xmlattr>.name");
                                arg.type = k.second.get <std::string> ("<xmlattr>.type");
                                arg.direction = k.second.get <std::string> ("<xmlattr>.direction");
                                meth.arguments.push_back(arg);
                            }
                        }
                        face.methods.push_back(meth);
                    }
                    else if (j.first == "property")
                    {
                        property prop;
                        prop.name = j.second.get <std::string> ("<xmlattr>.name");
                        prop.type = j.second.get <std::string> ("<xmlattr>.type");
                        prop.access = j.second.get <std::string> ("<xmlattr>.access");
                        face.properties.push_back(prop);
                    }
                    else if (j.first == "signal")
                    {
                        signal sign;
                        sign.name = j.second.get <std::string> ("<xmlattr>.name");
                        for (auto const& k : j.second)
                        {
                            if (k.first == "<xmlattr>")
                                continue;

                            if (k.first == "arg")
                            {
                                argument arg;
                                arg.name = k.second.get <std::string> ("<xmlattr>.name");
                                arg.type = k.second.get <std::string> ("<xmlattr>.type");
                                sign.arguments.push_back(arg);
                            }
                        }
                        face.signals.push_back(sign);
                    }
                    else
                        throw std::runtime_error("unexpected type string in method/property/signal enumeration");
                }
                faces.push_back(face);
            }
            else
            {
                // ignore
                //throw std::runtime_error("unexpected type string in interface enumeration: "s + i.first);
            }
        }
        return faces;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::optional <std::string> Introspector::convert_basic_type(char basic, bool& append_ref)
    {
        append_ref = false;

        switch (basic)
        {
        case('y'): return "uint8_t"s;
        case('b'): return "bool"s;
        case('n'): return "int16_t"s;
        case('q'): return "uint16_t"s;
        case('i'): return "int32_t"s;
        case('u'): return "uint32_t"s;
        case('x'): return "int64_t"s;
        case('t'): return "uint64_t"s;
        case('d'): return "double"s;
        case('s'):
        {
            append_ref = true;
            return "std::string"s;
        }
        case('o'):
        {
            append_ref = true;
            return "DBusMock::object_path"s;
        }
        case('g'):
        {
            append_ref = true;
            return "DBusMock::signature"s;
        }
        case('h'):
        {
            append_ref = true;
            return "DBusMock::file_descriptor"s;
        }
        case('v'):
        {
            append_ref = true;
            return "DBusMock::variant";
        }
        default: return std::nullopt;
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string Introspector::convert_type(std::string const& dbus_type)
    {
        std::function <std::optional <std::string> (
            std::string::const_iterator&,
            std::string::const_iterator const&)
        > recurseable;

        recurseable =
            [&recurseable, &dbus_type, this]
            (std::string::const_iterator& iter, std::string::const_iterator const& end) -> std::optional <std::string>
        {
            auto advance_tested = [&](std::string const& errorMessage)
            {
                ++iter;
                if (iter >= end)
                    throw std::runtime_error(errorMessage + ": " + dbus_type);
            };

            auto c = *iter;
            if (c == 'a')
            {
                advance_tested("array expected, but nothign follows to indicate what is in it");

                auto next = *iter;
                if (next == '{') // dictionary follows
                {
                    advance_tested("dictionary was opened, but does not contain anything");

                    bool add_ref = false;
                    auto basic = convert_basic_type(*iter, add_ref);
                    if (!basic)
                        throw std::runtime_error("basic_type expected in dictionary"s + dbus_type);

                    auto complete = recurseable(iter, end);
                    if (!complete)
                        throw std::runtime_error("complete_type expected in dictionary"s + dbus_type);

                    advance_tested("dictionary end expected");

                    if (*iter != '}')
                        throw std::runtime_error("dictionary end expected");
                    ++iter;

                    auto comp = complete.value();
                    comp = std::regex_replace(comp, std::regex(" const&"), "");

                    return "std::"s + dict_type + space_b4_template() + "<" +
                        basic.value() + "," + space_aft_comma() +
                        comp + "> const&"
                    ;
                }
                else // ordinary array
                {
                    return recurseable(iter, end);
                }
            }
            else if (c == '(') // structure follows
            {
                advance_tested("structure was opened, but does not contain anything");

                std::vector <std::string> elements;
                do
                {
                    auto element = recurseable(iter, end);
                    if (element)
                        elements.push_back(element.value());
                    else
                        break;
                } while (true);

                if (elements.empty())
                    throw std::runtime_error("structure may not be empty"s + dbus_type);

                object o;
                o.name = generate_struct_name();
                o.members = elements;

                objects[o.name] = o;

                //advance_tested("structure end expected");
                auto b = *iter;
                if (b != ')')
                    throw std::runtime_error("structure end expected"s + dbus_type);

                ++iter;
                return o.name;
            }
            else
            {
                bool add_ref = false;
                auto basic = convert_basic_type(*iter, add_ref);
                if (add_ref)
                    basic.value() += " const&";
                ++iter;
                return basic;
            }
        };

        auto iter = std::begin(dbus_type);
        auto end = std::end(dbus_type);

        auto res = recurseable(iter, end);
        if (!res)
            throw std::runtime_error("unexpected type string: "s + dbus_type);

        if (iter != end)
            throw std::runtime_error("type was unexpectedly not fully consumed: "s + dbus_type);

        return res.value();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string Introspector::space_b4_template() const
    {
        if (space_before_template)
            return " ";
        else
            return "";
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string Introspector::space_aft_comma() const
    {
        if (space_after_comma)
            return " ";
        else
            return "";
    }
//---------------------------------------------------------------------------------------------------------------------
    void Introspector::convert_types(std::vector <interface>& faces)
    {
        for (auto& face : faces)
        {
            for (auto& prop : face.properties)
            {
                prop.type = convert_type(prop.type);
            }
            for (auto& method : face.methods)
            {
                for (auto& arg : method.arguments)
                    arg.type = convert_type(arg.type);
            }
            for (auto& signal : face.signals)
            {
                for (auto& arg : signal.arguments)
                    arg.type = convert_type(arg.type);
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void Introspector::create_cpp(std::ostream& stream, std::vector <interface>& faces, std::string const&nspace)
    {
        stream << "namespace " << nspace << "\n{\n";
        for (auto const& face : faces)
        {
            auto fname = face.name;
            std::replace(fname.begin(), fname.end(), '.', '_');
            stream << "\tclass " << std::regex_replace(fname, std::regex("\\."), "_") << "\n";
            stream << "\t{\n";
            stream << "\tpublic:\n";
            stream << "\t\tvirtual ~" << fname << "() = default;\n";
            stream << "\tpublic: // Methods\n";
            for (auto const& method : face.methods)
            {
                stream << "\t\tvirtual auto " << method.name << "(";
                auto end = std::end(method.arguments);
                for (auto arg = std::begin(method.arguments); arg != end; ++arg)
                {
                    if (arg->direction == "in")
                    {
                        stream << arg->type << " " << arg->name;
                        if (arg + 1 != end)
                        {
                            if ((arg + 1)->direction != "out" || arg + 2 != end)
                                stream << "," << space_aft_comma();
                        }
                    }
                }
                stream << ')';
                bool anyOut = false;
                for (auto arg = std::begin(method.arguments); arg != end; ++arg)
                {
                    if (arg->direction == "out")
                    {
                        anyOut = true;
                        auto type = std::regex_replace(arg->type, std::regex(" const&"), "");
                        stream << " -> " << type << " /* " << arg->name << "*/ = 0;\n";
                        break;
                    }
                }
                if (!anyOut)
                    stream << " -> void = 0;\n";
            }
            stream << "\n";
            stream << "\tpublic: // Properties\n";
            for (auto const& prop : face.properties)
            {
                stream << "\t\t";
                if (prop.access == "read")
                    stream << "DBusMock::readable";
                else if (prop.access == "write")
                    stream << "DBusMock::writeable";
                else if (prop.access == "readwrite")
                    stream << "DBusMock::read_writeable";
                else
                    throw std::runtime_error("unknown access type"s + prop.access);

                auto type = std::regex_replace(prop.type, std::regex(" const&"), "");
                stream << space_b4_template() << "<";
                stream << type << "> " << prop.name << ";\n";
            }
            stream << "\n";
            stream << "\tpublic: // Signals\n";
            for (auto const& signal : face.signals)
            {
                stream << "\t\tsignal <void(";
                auto end = std::end(signal.arguments);
                for (auto arg = std::begin(signal.arguments); arg != end; ++arg)
                {
                    stream << arg->type << " /*" << arg->name << "*/";
                    if (arg + 1 != end)
                        stream << "," << space_aft_comma();
                }
                stream << ")> " << signal.name << ";\n";
            }
            stream << "\t};\n\n";
        }
        stream << "}\n";

        // now the mocking:

        stream << "\n\n\n";
        for (auto const& face : faces)
        {
            stream << "DBUS_MOCK_NAMESPACE\n";
            stream << "(\n";
            std::string modded_namespace = "(";
            for (auto n = std::begin(nspace); n != std::end(nspace); ++n)
            {
                if (*n != ':')
                    modded_namespace.push_back(*n);
                else
                {
                    modded_namespace += ")(";
                    ++n;
                }
            }
            modded_namespace += "),";

            auto fname = face.name;
            std::replace(fname.begin(), fname.end(), '.', '_');
            stream << "\t" << modded_namespace << "\n";
            stream << "\t" << fname << ",\n";

            // Methods
            if (face.methods.empty())
                stream << "\tDBUS_MOCK_NO_METHODS,\n";
            else
            {
                stream << "\tDBUS_MOCK_METHODS(";
                auto end = std::end(face.methods);
                for (auto m = std::begin(face.methods); m != end; ++m)
                {
                    stream << m->name;
                    if (m + 1 != end)
                        stream << "," << space_aft_comma();
                }
                stream << "),\n";
            }

            // Properties
            if (face.properties.empty())
                stream << "\tDBUS_MOCK_NO_PROPERTIES,\n";
            else
            {
                stream << "\tDBUS_MOCK_PROPERTIES(";
                auto end = std::end(face.properties);
                for (auto p = std::begin(face.properties); p != end; ++p)
                {
                    stream << p->name;
                    if (p + 1 != end)
                        stream << "," << space_aft_comma();
                }
                stream << "),\n";
            }

            // Signals
            if (face.signals.empty())
                stream << "\tDBUS_MOCK_NO_SIGNALS\n";
            else
            {
                stream << "\tDBUS_MOCK_SIGNALS(";
                auto end = std::end(face.signals);
                for (auto s = std::begin(face.signals); s != end; ++s)
                {
                    stream << s->name;
                    if (s + 1 != end)
                        stream << "," << space_aft_comma();
                }
                stream << ")\n";
            }

            stream << ")\n\n";
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string Introspector::generate_struct_name() const
    {
        auto size = objects.size();
        return "PLACE_HOLDER_"s + std::to_string(size);
    }
//#####################################################################################################################
}
