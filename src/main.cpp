#include "Hooks.h"
#include "Settings.h"

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	// TWO FIXES, both needed. Either alone leaves the plugin broken on a heavy load order.
	//
	// 1. LoadSettings() was never called anywhere in v2.7.0 -- grep the tree, it is defined in
	//    Settings.h and referenced nowhere. Every option therefore ran at its compile-time
	//    default and po3_StartOnSave.ini was decorative: "Save File" and "Character Name" were
	//    ignored (useSpecificSave/useCharName stay false, so EVERY save qualifies), the skip
	//    hotkey stayed VK_SHIFT, and -- the dangerous one -- startNewGame stayed TRUE, so an
	//    empty save list would start a new game regardless of the ini saying otherwise.
	//
	// 2. Registering the menu sink at kInputLoaded assumes the next LoadWaitSpinner is the main
	//    menu. That holds on a light load order, where kDataLoaded follows almost immediately.
	//    On a heavy one it does not: measured on a 385-mod build, kInputLoaded lands at +31s,
	//    LoadWaitSpinner ~1.5s later, and kDataLoaded not for another ~30s. The sink therefore
	//    fired on the INITIAL loading screen and drove the save load before the game was ready,
	//    faulting in the engine every single launch. Registering at kDataLoaded removes the
	//    assumption. Worst case becomes "does not autoload, parks at the main menu", which is a
	//    loud, harmless failure instead of a crash.
	if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
		Settings::GetSingleton()->LoadSettings();
		Settings::GetSingleton()->CheckKeyPress();
		StartOnSave::MenuManager::Register();
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Start On Save");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesNoStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Start On Save";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_SSE_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S.%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::Init(a_skse, false);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", OnInit);

	return true;
}
