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
			std::ranges::transform(charName, charName.begin(),
				[](const char c) { return static_cast<char>(std::tolower(c)); });

			useCharName = true;
		} else {
			useCharName = false;
		}

		type = string::lexical_cast<std::int32_t>(ini.GetValue("Settings", "Save Type", "0"));
		ini.SetValue("Settings", "Save Type", std::to_string(type).c_str(), ";Type of save to auto load\n;0 - Last save, 1 - Last quicksave, 2 - Last autosave, 3 - Last manual save.\n;4 - First save, 5 - First quicksave, 6 - First autosave, 7 - First manual save.", true);

		KEY = string::lexical_cast<int>(ini.GetValue("Settings", "Skip AutoLoad", "16"));
		ini.SetValue("Settings", "Skip AutoLoad", std::to_string(KEY).c_str(), ";Skip autoload by pressing this key (default: SHIFT) before the main menu loads.\n;List of keycodes - https://www.indigorose.com/webhelp/ams/Program_Reference/Misc/Virtual_Key_Codes.htm", true);

		loadNewGame = ini.GetBoolValue("Settings", "Start New Game", true);
		ini.SetBoolValue("Settings", "Start New Game", loadNewGame, ";Automatically start a new game if there are no saves.", true);

		disableMissingESPs = ini.GetBoolValue("Settings", "Disable Missing Content Warning", false);
		ini.SetBoolValue("Settings", "Disable Missing Content Warning", disableMissingESPs, ";Disables warning messagebox when loading saves with missing mods.", true);

		(void)ini.SaveFile(path);
	}
}

namespace UTIL
{
	void CheckKeyPress()
	{
		if (GetAsyncKeyState(Setting::KEY) & 0x8000) {
			Setting::skipLoading = true;
		}
	}

	bool GetValidSave(const std::string& a_name, std::int32_t a_offset = 0)
	{
		return Setting::type == (0 + a_offset) ||
		       Setting::type == (1 + a_offset) && a_name.contains("Quicksave") ||
		       Setting::type == (2 + a_offset) && a_name.contains("Autosave") ||
		       Setting::type == (3 + a_offset) && a_name.contains("Save");
	}
}

namespace Game
{
	void BuildSaveGameList(RE::BGSSaveLoadManager* a_manager)
	{
		using func_t = decltype(&BuildSaveGameList);
		REL::Relocation<func_t> func{ RELOCATION_ID(34850, 35760) };
		return func(a_manager);
	}

	void DoBeforeNewOrLoad()
	{
		using func_t = decltype(&DoBeforeNewOrLoad);
		REL::Relocation<func_t> func{ RELOCATION_ID(35596, 36604) };
		return func();
	}
}

class MenuManager : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	static MenuManager* GetSingleton()
	{
		static MenuManager singleton;
		return &singleton;
	}

	static void Register()
	{
		if (auto menuSrc = RE::UI::GetSingleton(); menuSrc) {
			menuSrc->AddEventSink(GetSingleton());
		}
	}

	static void Unregister()
	{
		if (auto menuSrc = RE::UI::GetSingleton(); menuSrc) {
			menuSrc->RemoveEventSink(GetSingleton());
		}
	}

protected:
	using EventResult = RE::BSEventNotifyControl;

	EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
	{
		if (!a_event) {
			return EventResult::kContinue;
		}

		const auto intfcStr = RE::InterfaceStrings::GetSingleton();
		if (a_event->menuName == intfcStr->loadWaitSpinner) {
			if (const auto manager = RE::BGSSaveLoadManager::GetSingleton(); manager) {
				Game::BuildSaveGameList(manager);

				const auto list = manager->saveGameList;
				if (list.empty()) {
					Unregister();
					return EventResult::kContinue;
				}

				RE::BGSSaveLoadFileEntry* lastGame = nullptr;

				if (Setting::useSpecificSave) {
					if (const auto result = std::ranges::find_if(list, [&](const auto& save) {
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
								if (!Setting::useCharName || !playerName.empty() && string::iequals(playerName, Setting::charName)) {
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
							if (!Setting::useCharName || !playerName.empty() && string::iequals(playerName, Setting::charName)) {
								lastGame = save;
								break;
							}
						}
					}
				}

				if (lastGame) {
					Game::DoBeforeNewOrLoad();
				    manager->Load(lastGame->fileName.c_str(), Setting::disableMissingESPs);
				}
			}

			Unregister();
		}

		return EventResult::kContinue;
	}

private:
	MenuManager() = default;
	MenuManager(const MenuManager&) = delete;
	MenuManager(MenuManager&&) = delete;

	~MenuManager() override = default;

	MenuManager& operator=(const MenuManager&) = delete;
	MenuManager& operator=(MenuManager&&) = delete;
};

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
			MenuManager::Register();
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

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Illusion Spells Affect Player");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Illusion Spells Affect Player";
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
	spdlog::set_pattern("[%l] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded");

	SKSE::Init(a_skse);

	Setting::Load();

	SKSE::GetMessagingInterface()->RegisterListener(OnInit);

	return true;
}
