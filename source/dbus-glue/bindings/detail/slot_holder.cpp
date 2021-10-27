#include <dbus-glue/bindings/detail/slot_holder.hpp>

namespace DBusMock::detail
{
//#####################################################################################################################
    slot_holder::async_context* slot_holder::insert(std::unique_ptr <async_context> ac)
	{
		auto* ptr = ac.get();
		contexts_.insert(std::move(ac));
		return ptr;
	}
//---------------------------------------------------------------------------------------------------------------------
	void slot_holder::erase(async_context* ctx)
	{
		contexts_.find(ctx);
	}
//#####################################################################################################################
}
