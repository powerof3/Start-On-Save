#pragma once

class Settings : public REX::Singleton<Settings>
{
public:
	void LoadSettings()
	{
		constexpr auto path = L"Data/SKSE/Plugins/po3_StartOnSave.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		ini::get_value(ini, specificSave, "Settings", "Save File", ";Auto load specific save. If blank, the last save will be loaded");
		useSpecificSave = !specificSave.empty();

		ini::get_value(ini, charName, "Settings", "Character Name", ";Auto load saves belonging to this character only. If blank, all saves will be considered.");
		useCharName = !charName.empty();

		ini::get_value(ini, type, "Settings", "Save Type", ";Type of save to auto load\n;0 - Last save (any), 1 - Last quicksave, 2 - Last autosave, 3 - Last manual save.\n;4 - First save (any), 5 - First quicksave, 6 - First autosave, 7 - First manual save");
		ini::get_value(ini, KEY, "Settings", "Skip AutoLoad Hotkey", ";Skip autoload by pressing this key (default: SHIFT) before the main menu loads.\n;List of keycodes - https://www.indigorose.com/webhelp/ams/Program_Reference/Misc/Virtual_Key_Codes.htm");
		ini::get_value(ini, startNewGame, "Settings", "Start New Game", ";Automatically start a new game if there are no saves.");
		ini::get_value(ini, disableWarning, "Settings", "Disable Missing Content Warning", ";Disable warning messagebox when loading saves with missing mods.");

		(void)ini.SaveFile(path);
	}

	bool CheckKeyPress()
	{
		if (skipLoading) {
			return true;
		}

		if (GetAsyncKeyState(KEY) & 0x8000) {
			skipLoading = true;
		}

		return skipLoading;
	}

	[[nodiscard]] bool GetValidSave(const RE::BSFixedString& a_name, std::int32_t a_offset = 0) const
	{
		return type == (0 + a_offset) ||
		       type == (1 + a_offset) && string::icontains(a_name, "Quicksave") ||
		       type == (2 + a_offset) && string::icontains(a_name, "Autosave") ||
		       type == (3 + a_offset) && string::icontains(a_name, "Save");
	}

	// members
	std::string  specificSave{};
	std::string  charName{};
	std::int32_t type{ 0 };
	int          KEY{ 16 };
	bool         useCharName{};
	bool         useSpecificSave{ false };
	bool         disableWarning{ false };
	bool         skipLoading{ false };
	bool         startNewGame{ true };
};
