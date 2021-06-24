#pragma once

#include <cstdint>

namespace bsa
{
	namespace tes4
	{
		namespace detail
		{
			class header_t;

			template <class, bool>
			class index_t;
		}

		namespace hashing
		{
			struct hash;
		}

		class archive;
		class directory;
		class file;

		enum class archive_flag : std::uint32_t;
		enum class archive_type : std::uint16_t;
		enum class version : std::uint32_t;
	}
}