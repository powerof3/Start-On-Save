#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <windows.h> 
#include <SimpleIni.h>

#pragma warning(disable: 4100)

#pragma warning(push)
#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#pragma warning(pop)

namespace logger = SKSE::log;

using namespace std::literals;

namespace stl
{
	using SKSE::stl::adjust_pointer;
	using SKSE::stl::report_and_fail;
	using SKSE::stl::to_underlying;

	template <class T>
	T to_num(const std::string& a_str, bool a_hex = false)
	{
		if constexpr (std::is_floating_point_v<T>) {
			return static_cast<T>(std::stof(a_str));
		} else if constexpr (std::is_signed_v<T>) {
			return static_cast<T>(std::stoi(a_str));
		} else if (a_hex) {
			return static_cast<T>(std::stoul(a_str, nullptr, 16));
		} else {
			return static_cast<T>(std::stoul(a_str));
		}
	}
}

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
