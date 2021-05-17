#include "version.h"

//SETTINGS
std::string specificSave;
bool useSpecificSave;

std::uint32_t type;

std::string charName;
bool useCharName;

bool disableMissingESPs;

int KEY;
bool skipLoading = false;


void LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_StartOnSave.ini";

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();

	ini.LoadFile(path);

	specificSave = ini.GetValue("Settings", "Save File", "");
	ini.SetValue("Settings", "Save File", specificSave.c_str(), ";Auto load specific save. If blank, the last save will be loaded", true);
	useSpecificSave = !specificSave.empty();

	charName = ini.GetValue("Settings", "Character Name", "");
	ini.SetValue("Settings", "Character Name", charName.c_str(), ";Auto load saves belonging to this character only. If blank, all saves will be considered.", true);

	if (!charName.empty()) {
		std::transform(charName.begin(), charName.end(), charName.begin(),
			[](char c) { return static_cast<char>(std::tolower(c)); });

		useCharName = true;
	} else {
		useCharName = false;
	}

	type = SKSE::STRING::to_num<std::uint32_t>(ini.GetValue("Settings", "Save Type", "0"));
	ini.SetValue("Settings", "Save Type", std::to_string(type).c_str(), ";Type of save to auto load\n;0 - Last save, 1 - Last quicksave, 2 - Last autosave, 3 - Last manual save.", true);

	KEY = SKSE::STRING::to_num<int>(ini.GetValue("Settings", "Skip AutoLoad", "16"));
	ini.SetValue("Settings", "Skip AutoLoad", std::to_string(KEY).c_str(), ";Skip autoload by pressing this key (default: SHIFT) before the main menu loads.\n;List of keycodes - https://www.indigorose.com/webhelp/ams/Program_Reference/Misc/Virtual_Key_Codes.htm", true);

	disableMissingESPs = ini.GetBoolValue("Settings", "Disable Missing Content Warning", false);
	ini.SetBoolValue("Settings", "Disable Missing Content Warning", disableMissingESPs, ";Disables warning messagebox when loading saves with missing mods", true);

	ini.SaveFile(path);
}


void CheckKeyPress()
{
	if (GetAsyncKeyState(KEY) & 0x8000) {
		skipLoading = true;
	}
}


void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	using namespace SKSE::STRING;

	switch (a_msg->type) {
	case SKSE::MessagingInterface::kInputLoaded:
		{
			CheckKeyPress();  //check for first time		
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			if (auto manager = RE::BGSSaveLoadManager::GetSingleton(); manager) {
				manager->PopulateSaveList();

				const auto list = manager->saveGameList;
				if (list.empty()) {
					return;
				}

				CheckKeyPress(); //check again
				if (skipLoading) {
					return;
				}

				RE::BGSSaveLoadFileEntry* lastGame = nullptr;

				if (useSpecificSave) {
					if (const auto result = std::find_if(list.begin(), list.end(), [&](const auto& save) {
							return specificSave == save->fileName.c_str();
						});
						result != list.end()) {
						lastGame = *result;
					}
				}

				if (!lastGame) {
					for (auto i = list.size(); i-- > 0;) {	// the most recent save is stored at the back
						const auto save = manager->saveGameList[i];
						if (save) {
							std::string name{ save->fileName.c_str() };
							std::string playerName{ save->playerName.c_str() };

							if (type == 0 || type == 1 && name.find("Quicksave") != std::string::npos || type == 2 && name.find("Autosave") != std::string::npos || type == 3 && name.find("Save") != std::string::npos) {
								if (!useCharName || !playerName.empty() && insenstiveStringFind(playerName, charName)) {
									lastGame = save;
									break;
								}
							}
						}
					}
				}

				if (lastGame) {
					manager->Load(lastGame->fileName.c_str(), disableMissingESPs);
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


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	try {
		auto path = logger::log_directory().value() / "po3_StartOnSave.log";
		auto log = spdlog::basic_logger_mt("global log", path.string(), true);
		log->flush_on(spdlog::level::info);

#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
		log->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
		log->set_level(spdlog::level::info);

#endif
		spdlog::set_default_logger(log);
		spdlog::set_pattern("[%H:%M:%S] [%l] %v");

		logger::info("Start On Save {}", SOS_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "StartOnSave";
		a_info->version = SOS_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			logger::critical("Loaded in editor, marking as incompatible");
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver < SKSE::RUNTIME_1_5_39) {
			logger::critical("Unsupported runtime version {}", ver.string());
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	try {
		logger::info("Start On Save loaded");

		SKSE::Init(a_skse);

		LoadSettings();

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", OnInit)) {
			return false;
		}

	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}
