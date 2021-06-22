#include "bsa/tes4.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <utility>

#pragma warning(push)
#pragma warning(disable: 4701)  // Potentially uninitialized local variable 'name' used
#pragma warning(disable: 4702)  // unreachable code
#pragma warning(disable: 4244)  // 'argument' : conversion from 'type1' to 'type2', possible loss of data
#include <boost/text/in_place_case_mapping.hpp>
#include <boost/text/text.hpp>
#pragma warning(pop)

namespace bsa::tes4::hashing
{
	namespace
	{
		[[nodiscard]] auto normalize_directory(std::filesystem::path& a_path) noexcept
			-> boost::text::text
		{
			boost::text::text p{
				a_path
					.lexically_normal()
					.make_preferred()
					.u8string()
			};

			boost::text::in_place_to_lower(p);

			while (!p.empty() && p.back() == u8'\\') {
				p.pop_back();
			}

			while (!p.empty() && p.front() == u8'\\') {
				p.erase(p.begin());
			}

			if (p.empty()) {
				p.push_back(u8'.');
			}

			const auto ptr = reinterpret_cast<const char8_t*>(p.data());
			a_path.assign(ptr, ptr + p.storage_code_units());
			return p;
		}

		[[nodiscard]] constexpr auto make_file_extension(std::u8string_view a_extension) noexcept
			-> std::uint32_t
		{
			std::uint32_t ext = 0;
			for (std::size_t i = 0; i < std::min<std::size_t>(a_extension.size(), 4u); ++i) {
				ext |= std::uint32_t{ a_extension[i] } << i * 8;
			}
			return ext;
		}
	}

	hash hash_directory(std::filesystem::path& a_path) noexcept
	{
		const auto p = normalize_directory(a_path);
		const std::span<const std::byte> view{
			reinterpret_cast<const std::byte*>(p.data()),
			p.storage_code_units()
		};

		hash h;

		switch (std::min<std::size_t>(view.size(), 3)) {
		case 3:
			h.last2 = static_cast<std::uint8_t>(*(view.end() - 2));
			[[fallthrough]];
		case 2:
		case 1:
			h.last = static_cast<std::uint8_t>(view.back());
			h.first = static_cast<std::uint8_t>(view.front());
			[[fallthrough]];
		default:
			break;
		}

		h.length = static_cast<std::uint8_t>(view.size());
		if (h.length > 3) {
			// skip first and last two chars -> already processed
			h.crc = detail::crc32(view.subspan(1, view.size() - 3));
		}

		return h;
	}

	hash hash_file(std::filesystem::path& a_path) noexcept
	{
		constexpr std::array lut{
			make_file_extension(u8""sv),
			make_file_extension(u8".nif"sv),
			make_file_extension(u8".kf"sv),
			make_file_extension(u8".dds"sv),
			make_file_extension(u8".wav"sv),
			make_file_extension(u8".adp"sv),
		};

		a_path = a_path.filename();
		const auto pstr = normalize_directory(a_path);
		const std::u8string_view pview{
			reinterpret_cast<const char8_t*>(pstr.data()),
			pstr.storage_code_units()
		};

		const auto [stem, extension] = [&]() noexcept
			-> std::pair<std::u8string_view, std::u8string_view> {
			const auto split = pview.find_last_of(u8'.');
			if (split != std::u8string_view::npos) {
				return {
					pview.substr(0, split),
					pview.substr(split + 1)
				};
			} else {
				return {
					pview,
					u8""sv
				};
			}
		}();

		if (!stem.empty()) {
			auto h = [&]() noexcept {
				std::filesystem::path temp{ stem };
				return hash_directory(temp);
			}();
			h.crc += detail::crc32(
				{ reinterpret_cast<const std::byte*>(extension.data()), extension.size() });

			const auto it = std::find(
				lut.begin(),
				lut.end(),
				make_file_extension(extension));
			if (it != lut.end()) {
				const auto i = static_cast<std::uint8_t>(it - lut.begin());
				h.first += 32u * (i & 0xFCu);
				h.last += (i & 0xFEu) << 6u;
				h.last2 += i << 7u;
			}

			return h;
		} else {
			return {};
		}
	}
}