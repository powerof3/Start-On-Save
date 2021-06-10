namespace Setting
{
	std::string specificSave;
	bool useSpecificSave;

	std::int32_t type;

	std::string charName;
	bool useCharName;

	bool disableMissingESPs;

	int KEY;
	bool skipLoading = false;

	bool loadNewGame;

	void Load()
	{
		constexpr auto path = L"Data/SKSE/Plugins/po3_StartOnSave.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		specificSave = ini.GetValue("Settings", "Save File", "");
		ini.SetValue("Settings", "Save File", specificSave.c_str(), ";Auto load specific save. If blank, the last save will be loaded", true);
		useSpecificSave = !specificSave.empty();

		charName = ini.GetValue("Settings", "Character Name", "");
		ini.SetValue("Settings", "Character Name", charName.c_str(), ";Auto load saves belonging to this character only. If blank, all saves will be considered.", true);

		if (!charName.empty()) {
			std::transform(charName.begin(), charName.end(), charName.begin(),
				[](const char c) { return static_cast<char>(std::tolower(c)); });

			useCharName = true;
		} else {
			useCharName = false;
		}

		type = stl::to_num<std::int32_t>(ini.GetValue("Settings", "Save Type", "0"));
		ini.SetValue("Settings", "Save Type", std::to_string(type).c_str(), ";Type of save to auto load\n;0 - Last save, 1 - Last quicksave, 2 - Last autosave, 3 - Last manual save.\n;4 - First save, 5 - First quicksave, 6 - First autosave, 7 - First manual save.", true);

		KEY = stl::to_num<int>(ini.GetValue("Settings", "Skip AutoLoad", "16"));
		ini.SetValue("Settings", "Skip AutoLoad", std::to_string(KEY).c_str(), ";Skip autoload by pressing this key (default: SHIFT) before the main menu loads.\n;List of keycodes - https://www.indigorose.com/webhelp/ams/Program_Reference/Misc/Virtual_Key_Codes.htm", true);

		loadNewGame = ini.GetBoolValue("Settings", "Start New Game", true);
		ini.SetBoolValue("Settings", "Start New Game", loadNewGame, ";Automatically start a new game if there are no saves.", true);

		disableMissingESPs = ini.GetBoolValue("Settings", "Disable Missing Content Warning", false);
		ini.SetBoolValue("Settings", "Disable Missing Content Warning", disableMissingESPs, ";Disables warning messagebox when loading saves with missing mods.", true);

		ini.SaveFile(path);
	}
}

namespace UTIL
{
	bool insenstiveStringFind(std::string& a_str1, std::string_view a_str2, size_t pos = 0)
	{
		std::transform(a_str1.begin(), a_str1.end(), a_str1.begin(),
			[](char c) { return static_cast<char>(std::tolower(c)); });

		return a_str1.find(a_str2, pos) != std::string::npos;
	}

	void CheckKeyPress()
	{
		if (GetAsyncKeyState(Setting::KEY) & 0x8000) {
			Setting::skipLoading = true;
		}
	}

	bool GetValidSave(const std::string& a_name, std::uint32_t a_offset = 0)
	{
		return Setting::type == (0 + a_offset) || Setting::type == (1 + a_offset) && a_name.find("Quicksave") != std::string::npos || Setting::type == (2 + a_offset) && a_name.find("Autosave") != std::string::npos || Setting::type == (3 + a_offset) && a_name.find("Save") != std::string::npos;
	}
}

namespace Game
{
	void PopulateSaveList(RE::BGSSaveLoadManager* a_manager)
	{
		using func_t = decltype(&PopulateSaveList);
		REL::Relocation<func_t> func{ REL::ID(34850) };
		return func(a_manager);
	}

	void StartNewGame()
	{
		using func_t = decltype(&StartNewGame);
		REL::Relocation<func_t> func{ REL::ID(51246) };
		return func();
	}
}

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kInputLoaded:
		{
			UTIL::CheckKeyPress();  //check for first time
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			UTIL::CheckKeyPress();  //check again
			if (Setting::skipLoading) {
				return;
			}

			if (auto manager = RE::BGSSaveLoadManager::GetSingleton(); manager) {
				Game::PopulateSaveList(manager);

				const auto list = manager->saveGameList;
				if (list.empty()) {
					/* if (Setting::loadNewGame) {
						Game::StartNewGame();
					}*/
					return;
				}

				RE::BGSSaveLoadFileEntry* lastGame = nullptr;

				if (Setting::useSpecificSave) {
					if (const auto result = std::find_if(list.begin(), list.end(), [&](const auto& save) {
							return Setting::specificSave == save->fileName.c_str();
						});
						result != list.end()) {
						lastGame = *result;
					}
				}

				if (!lastGame) {
					if (Setting::type < 4) {
						for (auto i = list.size(); i-- > 0;) {  // the most recent save is stored at the back
							const auto save = manager->saveGameList[i];
							if (save && UTIL::GetValidSave(save->fileName.c_str())) {
								std::string playerName{ save->playerName.c_str() };
								if (!Setting::useCharName || !playerName.empty() && UTIL::insenstiveStringFind(playerName, Setting::charName)) {
									lastGame = save;
									break;
								}
							}
						}
					}
				} else {
					for (auto& save : list) {
						if (save && UTIL::GetValidSave(save->fileName.c_str(), 4)) {
							std::string playerName{ save->playerName.c_str() };
							if (!Setting::useCharName || !playerName.empty() && UTIL::insenstiveStringFind(playerName, Setting::charName)) {
								lastGame = save;
								break;
							}
						}
					}
				}

				if (lastGame) {
					manager->Load(lastGame->fileName.c_str(), Setting::disableMissingESPs);
				}
			}
		}
		break;
	case SKSE::MessagingInterface::kPostLoadGame:
		{
			//TO DO - implement auto focus
		}
	default:
		break;
	}
}


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= "po3_StartOnSave.log "sv;

	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] [%l] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded");

	SKSE::Init(a_skse);

	Setting::Load();

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", OnInit)) {
		return false;
	}

	return true;
}
