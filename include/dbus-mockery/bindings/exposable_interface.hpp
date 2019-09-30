#pragma once

#include "exposable_interface_fwd.hpp"
#include "sdbus_core.hpp"
#include "detail/table_entry.hpp"
#include "basic_exposable_interface.hpp"

#include "exposables/basic_exposable_method.hpp"
#include "exposables/basic_exposable_property.hpp"

#include <memory>
#include <variant>
#include <iomanip>

namespace DBusMock
{
    class exposable_interface : public basic_exposable_interface
	{
	public:
		/**
		 * @brief exposable_interface Creates the interface, but does nothing for now.
		 */
		exposable_interface()
		    : slot_{nullptr}
		    , methods_{}
		    , vtable_{}
		{
		}

		virtual ~exposable_interface()
		{
			sd_bus_slot_unref(slot_);
		}

		virtual std::string path() const = 0;

		virtual std::string service() const = 0;

		template <typename T>
		void add_method(std::unique_ptr <T> method)
		{
			methods_.push_back(std::move(method));
		}

		template <typename T>
		void add_property(std::unique_ptr <T> property)
		{
			methods_.push_back(std::move(property));
		}

		template <typename BusT>
		int expose(BusT& bus)
		{
			vtable_.clear();

			// Start
			vtable_ = {SD_BUS_VTABLE_START(SD_BUS_VTABLE_UNPRIVILEGED)};

			// Reserve size
			table_.reserve(methods_.size());

			auto calculate_offset = [this]()
			{
				return
				    reinterpret_cast <uint64_t> (&table_.back()) -
				    reinterpret_cast <uint64_t> (&table_.front())
				;
			};

			// Methods
			for (auto const& m : methods_)
			{
				table_.emplace_back(m.get());
				auto offset = calculate_offset();
				vtable_.push_back(m->make_vtable_entry(offset));
			}

			// Properties
			for (auto const& p : properties_)
			{
				table_.emplace_back(p.get());
				auto offset = calculate_offset();
				vtable_.push_back(p->make_vtable_entry(offset));
			}

			// End
			vtable_.push_back(SD_BUS_VTABLE_END);

			auto p = path();
			auto s = service();

			int r = sd_bus_add_object_vtable(
			    static_cast <sd_bus*> (bus),
			    /*&slot_,*/
			    nullptr,
			    p.c_str(),
			    s.c_str(),
			    vtable_.data(),
			    &table_.front()
			);

			return r;
		}

	private:
		sd_bus_slot* slot_;
		std::vector <std::unique_ptr <basic_exposable_method>> methods_;
		std::vector <std::unique_ptr <basic_exposable_property>> properties_;
		std::vector <sd_bus_vtable> vtable_;

		// my personal table
		std::vector <detail::table_entry> table_;
	};
}

