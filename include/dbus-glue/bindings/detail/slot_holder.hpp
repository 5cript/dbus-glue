#pragma once

#include "../async_context.hpp"

#include <set>
#include <memory>

namespace DBusGlue::detail
{
    /**
	 * @brief The slot_holder class holds slots for at least as long as the bus lives,
	 *		  but allows for releasing slots earlier. This way we dont leak if for some reason, a callback is not
	 *		  called.
	 */
    class slot_holder
	{
	public:
		using async_context = async_context_base;

	public:
		slot_holder()
		    : contexts_()
		{
		}

		/**
		 * @brief insert Insert new slot to holder.
		 * @param ac
		 * @return A pointer to the inserted slot. Can later be used to remove it again.
		 */
		async_context* insert(std::unique_ptr <async_context> ac);

		/**
		 * @brief erase Erases the slot from the holder.
		 * @param ctx
		 */
		void erase(async_context* ctx);

	private:
		template<class T>
		struct pointer_comp
		{
			typedef std::true_type is_transparent;
			// helper does some magic in order to reduce the number of
			// pairs of types we need to know how to compare: it turns
			// everything into a pointer, and then uses `std::less<T*>`
			// to do the comparison:
			struct helper
			{
				T* ptr;
				helper():ptr(nullptr) {}
				helper(helper const&) = default;
				helper(T* p):ptr(p) {}

				template<class U>
				helper(std::shared_ptr<U> const& sp):ptr(sp.get()) {}

				template<class U>
				helper(std::unique_ptr<U> const& up):ptr(up.get()) {}

				// && optional: enforces rvalue use only
				bool operator<( helper o ) const
				{
					return std::less<T*>()( ptr, o.ptr );
				}
			};
			// without helper, we would need 2^n different overloads, where
			// n is the number of types we want to support (so, 8 with
			// raw pointers, unique pointers, and shared pointers).  That
			// seems silly:
			// && helps enforce rvalue use only
			bool operator()( helper const&& lhs, helper const&& rhs ) const
			{
				return lhs < rhs;
			}
		};

	private:
		std::set <std::unique_ptr <async_context>, pointer_comp <async_context>> contexts_;
	};
}
