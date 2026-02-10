#pragma once

#include <ranges>

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REX/REX/Singleton.h"

#pragma warning(push)
#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#include "ClibUtil/simpleINI.hpp"
#include <xbyak/xbyak.h>
#pragma warning(pop)

namespace logger = SKSE::log;
namespace string = clib_util::string;
namespace ini = clib_util::ini;

using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;
}

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
