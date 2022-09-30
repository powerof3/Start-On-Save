#include "Hooks.h"
#include "Settings.h"

namespace StartOnSave
{
	struct Game
	{
		static void BuildSaveGameList(RE::BGSSaveLoadManager* a_manager)
		{
			using func_t = decltype(&BuildSaveGameList);
			REL::Relocation<func_t> func{ RELOCATION_ID(34850, 35760) };
			return func(a_manager);
		}

		static void DoBeforeNewOrLoad()
		{
			using func_t = decltype(&DoBeforeNewOrLoad);
			REL::Relocation<func_t> func{ RELOCATION_ID(35596, 36604) };
			return func();
		}

		static void StartNewGame()
		{
			using func_t = decltype(&StartNewGame);
			REL::Relocation<func_t> func{ RELOCATION_ID(51246, 52118) };
			return func();
		}
	};

	MenuManager::EventResult MenuManager::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_event) {
			return EventResult::kContinue;
		}

		if (a_event->menuName != RE::LoadWaitSpinner::MENU_NAME) {
			return EventResult::kContinue;
		}

		const auto settings = Settings::GetSingleton();
		if (settings->CheckKeyPress()) {
			Unregister();
			return EventResult::kContinue;
		}

	    if (const auto manager = RE::BGSSaveLoadManager::GetSingleton(); manager) {
			Game::BuildSaveGameList(manager);

			const auto& list = manager->saveGameList;
			if (list.empty()) {
				if (settings->startNewGame) {
					Game::StartNewGame();
				}

				Unregister();
			    return EventResult::kContinue;
			}

			RE::BGSSaveLoadFileEntry* lastGame = nullptr;

			if (settings->useSpecificSave) {
				if (const auto result = std::ranges::find_if(list, [&](const auto& save) {
						return settings->specificSave == save->fileName.c_str();
					});
					result != list.end()) {
					lastGame = *result;
				}
			}

			if (!lastGame) {
				const auto get_valid_save = [&](RE::BGSSaveLoadFileEntry* a_save, std::int32_t a_offset = 0) {
					if (a_save && settings->GetValidSave(a_save->fileName, a_offset)) {
						if (!settings->useCharName || string::iequals(a_save->playerName, settings->charName)) {
							lastGame = a_save;
							return true;
						}
					}
					return false;
				};

			    if (settings->type < 4) {
					for (auto& save : list | std::views::reverse) { // the most recent save is stored at back
						if (get_valid_save(save)) {
							break;
						} 
					}
				} else {
					for (auto& save : list) {
						if (get_valid_save(save, 4)) {
						    break;
						}
					}
				}
			}

			if (lastGame) {
				Game::DoBeforeNewOrLoad();
				manager->Load(lastGame->fileName.c_str(), settings->disableWarning);
			}

			Unregister();
		}

		return EventResult::kContinue;
	}
}
