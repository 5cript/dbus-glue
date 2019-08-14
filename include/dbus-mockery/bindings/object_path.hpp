#pragma once

#include <string>
#include <iosfwd>

// TODO:
// Implement restriction checking: https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-marshaling-object-path

namespace DBusMock
{
    class object_path
	{
	private:
		std::string data_;

	public:
		object_path() noexcept
		    : data_{}
		{
		}

		object_path& operator=(object_path const&) = default;
		object_path& operator=(object_path&&) = default;
		object_path(object_path const&) = default;
		object_path(object_path&&) = default;

		bool operator==(object_path const& other) const
		{
			return data_ == other.data_;
		}

		object_path& operator=(std::string const& str)
		{
			data_ = str;
			return *this;
		}

		explicit operator std::string() const noexcept
		{
			return data_;
		}

		explicit object_path(std::string const& data)
		    : data_{data}
		{
		}

		explicit object_path(char const* data)
		    : data_{data}
		{
		}

		std::string const& string() const
		{
			return data_;
		}

		char const* c_str() const
		{
			return data_.c_str();
		}
	};

	struct object_path_hasher
	{
		std::size_t operator()(object_path const& op) const
		{
			return std::hash <std::string>()(op.string());
		}
	};

	std::ostream& operator<<(std::ostream& stream, object_path const& opath);
}

#ifndef DBUS_MOCK_NO_STD_HASH_SPECIALIZATION
namespace std
{
    template <>
    struct hash <DBusMock::object_path>
	{
		std::size_t operator()(DBusMock::object_path const& op) const
		{
			return std::hash <std::string>()(op.string());
		}
	};
}
#endif
