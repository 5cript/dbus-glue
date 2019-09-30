#pragma once

#include "basic_exposable_property.hpp"
#include "../types.hpp"
#include "../detail/dissect.hpp"

extern "C" {
    int dbus_mock_exposable_property_read
	(
	    sd_bus* bus,
	    char const* path,
	    char const* interface,
	    char const* property,
	    sd_bus_message* reply,
	    void* userdata,
	    sd_bus_error* error
	);

	int dbus_mock_exposable_property_write
	(
	    sd_bus* bus,
	    char const* path,
	    char const* interface,
	    char const* property,
	    sd_bus_message* reply,
	    void* userdata,
	    sd_bus_error* error
	);
}

namespace DBusMock
{
    template <typename T>
    class exposable_property : public basic_exposable_property
	{
	public:
		using owner_type = typename detail::member_dissect <T>::interface_type;
		using value_type = typename detail::member_dissect <T>::member_type;

	private:
		mutable std::string signature_;
		mutable uint64_t offset_;
		mutable uint64_t flags_;

		void prepare_for_expose() const
		{
			signature_.clear();

			signature_ = detail::vector_flatten(
			    detail::argument_signature_factory <value_type>::build()
			);

			if (change_behaviour == property_change_behaviour::always_constant)
				flags_ = SD_BUS_VTABLE_PROPERTY_CONST;
			else if (change_behaviour == property_change_behaviour::emits_change)
				flags_ = SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE;
			else if (change_behaviour == property_change_behaviour::emits_invalidation)
				flags_ = SD_BUS_VTABLE_PROPERTY_EMITS_INVALIDATION;
			else if (change_behaviour == property_change_behaviour::explicit_invocation)
				flags_ = SD_BUS_VTABLE_PROPERTY_EXPLICIT;
		}

	public:
		std::string name;
		bool writeable;
		property_change_behaviour change_behaviour;
		owner_type* owner;
		T property;

		void set_owner(owner_type* own)
		{
			owner = own;
		}

		sd_bus_vtable make_vtable_entry(std::size_t offset) const override
		{
			prepare_for_expose();
			offset_ = offset;

			if (writeable)
			{
				return SD_BUS_WRITABLE_PROPERTY(
				    name.c_str(),
				    signature_.c_str(),
				    &::dbus_mock_exposable_property_read, // TO CHANGE
				    &::dbus_mock_exposable_property_write, // TO CHANGE
				    offset,
				    flags_
				);
			}
			else
			{
				return SD_BUS_PROPERTY(
				    name.c_str(),
				    signature_.c_str(),
				    &::dbus_mock_exposable_property_read, // TO CHANGE
				    offset,
				    flags_
				);
			}
		}
		int read(message& msg) override
		{
			return 0;
		}
		int write(message& msg) override
		{
			return 0;
		}
	};
}
