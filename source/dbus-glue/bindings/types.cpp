#include <dbus-glue/bindings/types.hpp>

#include <dbus-glue/bindings/message.hpp>

using namespace std::string_literals;

#define CASE_CONV(ch, name) \
	case(ch): \
    { \
	    return name;\
	}

namespace DBusMock
{
//#####################################################################################################################
    message_variant::message_variant(class message& msg)
	    : message_{}
	{
		assign(msg);
	}
//---------------------------------------------------------------------------------------------------------------------
	message_variant::message_variant(class message&& msg)
	    : message_{new message(std::move(msg))}
	{
	}
//---------------------------------------------------------------------------------------------------------------------
	message_variant::message_variant(message_variant const& other)
	    : message_{new message(other.message_->clone(true))}
	{
	}
//---------------------------------------------------------------------------------------------------------------------
	message_variant::message_variant(class message* msg)
	    : message_(msg)
	{
	}
//---------------------------------------------------------------------------------------------------------------------
	message* message_variant::release()
	{
		return message_.release();
	}
//---------------------------------------------------------------------------------------------------------------------
	message_variant& message_variant::operator=(message_variant const& other)
	{
		message_.reset(new message(other.message_->clone(true)));
		return *this;
	}
//---------------------------------------------------------------------------------------------------------------------
	int message_variant::assign(message& msg)
	{
		int r = 0;

		auto* handle = msg.msg;

		r = sd_bus_message_enter_container(handle, SD_BUS_TYPE_VARIANT, msg.type().contained.data());
		if (r < 0)
			throw std::runtime_error("could not enter variant");

		message_.reset(new message(msg.bus(), 2));
		msg.copy_into(*message_.get(), false);
		message_->seal();

		r = sd_bus_message_exit_container(handle);
		if (r < 0)
			throw std::runtime_error("could copy exit variant");

		return r;
	}
//---------------------------------------------------------------------------------------------------------------------
	void message_variant::clear()
	{
		message_.reset(nullptr);
	}
//---------------------------------------------------------------------------------------------------------------------
	type_descriptor message_variant::type() const
	{
		return message_->type();
	}
//---------------------------------------------------------------------------------------------------------------------
	void message_variant::rewind(bool complete)
	{
		message_->rewind(complete);
	}
//---------------------------------------------------------------------------------------------------------------------
	int message_variant::append_to(message& other) const
	{
		using namespace std::string_literals;
		sd_bus_message* smsg = static_cast <sd_bus_message*> (other);

/*
		auto r = sd_bus_message_open_container(smsg, SD_BUS_TYPE_VARIANT, type().contained.data());
		if (r < 0)
			throw std::runtime_error("could not open variant: "s + strerror(-r));
*/

		auto rc = sd_bus_message_copy(smsg, message_->handle(), true);
		if (rc < 0)
			throw std::runtime_error("could not copy variant data into message: "s + strerror(-rc));

/*
		r = sd_bus_message_close_container(smsg);
		if (r < 0)
			throw std::runtime_error("could not close variant: "s + strerror(-r));
*/

		return rc;
	}
//#####################################################################################################################
	std::string typeToComprehensible(char type)
	{
		switch(type)
		{
		    CASE_CONV('y', "unsigned char")
			CASE_CONV('b', "boolean")
			CASE_CONV('n', "short signed integer")
			CASE_CONV('q', "short unsigned integer")
			CASE_CONV('i', "signed integer")
			CASE_CONV('u', "unsigned integer")
			CASE_CONV('x', "signed long integer")
			CASE_CONV('t', "unsigned long integer")
			CASE_CONV('d', "double precision floating point")
			CASE_CONV('s', "string")
			CASE_CONV('o', "object path")
			CASE_CONV('g', "signature")
			CASE_CONV('h', "file descriptor")
			CASE_CONV('a', "array")
			CASE_CONV('v', "variant")
		}
		return "unknown:"s + type;
	}
//---------------------------------------------------------------------------------------------------------------------
	std::string typeToCpp(char type)
	{
		switch(type)
		{
		    CASE_CONV('y', "uint8_t")
			CASE_CONV('b', "bool")
			CASE_CONV('n', "int16_t")
			CASE_CONV('q', "uint16_t")
			CASE_CONV('i', "int32_t")
			CASE_CONV('u', "uint32_t")
			CASE_CONV('x', "int64_t")
			CASE_CONV('t', "uint64_t")
			CASE_CONV('d', "double")
			CASE_CONV('s', "std::string")
			CASE_CONV('o', "std::string")
			CASE_CONV('g', "std::string")
			CASE_CONV('h', "int")
			CASE_CONV('a', "[array]")
			CASE_CONV('v', "{variant}")
		}
		return "unknown:"s + type;
	}
//#####################################################################################################################
}

#undef CASE_CONV
