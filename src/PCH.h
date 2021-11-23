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
namespace string = SKSE::stl::string;

using namespace std::literals;

namespace stl
{
	using SKSE::stl::adjust_pointer;
	using SKSE::stl::report_and_fail;
	using SKSE::stl::to_underlying;
}

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
