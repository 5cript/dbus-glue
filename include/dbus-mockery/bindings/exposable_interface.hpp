#pragma once

#include "sdbus_core.hpp"
#include "exposed_method.hpp"
#include "detail/table_entry.hpp"
#include "basic_exposable_interface.hpp"

#include <memory>
#include <variant>

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

		template <typename BusT>
		int expose(BusT& bus)
		{
			vtable_.clear();

			// Start
			vtable_ = {SD_BUS_VTABLE_START(SD_BUS_VTABLE_UNPRIVILEGED)};

			// Methods
			{
				std::cout << reinterpret_cast <int64_t> (methods_.data()) << std::endl;

				int offset = 0;
				for (auto const& m : methods_)
				{
					table_.push_back({
					    m.get()
					});
					vtable_.push_back(std::move(m->make_vtable_entry(offset)));
					offset += sizeof(decltype (methods_)::pointer);
				}
			}

			// End
			vtable_.push_back(SD_BUS_VTABLE_END);

			auto p = path();
			auto s = service();

			int r = sd_bus_add_object_vtable(
			    static_cast <sd_bus*> (bus),
			    &slot_,
			    p.c_str(),
			    s.c_str(),
			    vtable_.data(),
			    &table_.front()
			);

			return r;
		}

	private:
		sd_bus_slot* slot_;
		std::vector <std::unique_ptr <basic_exposed_method>> methods_;
		std::vector <sd_bus_vtable> vtable_;

		// my personal table
		std::vector <detail::table_entry> table_;
	};
}

