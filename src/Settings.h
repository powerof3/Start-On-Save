#pragma once

class Settings
{
public:
	[[nodiscard]] static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
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

	std::string specificSave{};
	bool useSpecificSave{ false };

	std::string charName{};
	bool useCharName{};

	std::int32_t type{ 0 };

	bool disableWarning{ false };

	int KEY{ 16 };
	bool skipLoading{ false };

	bool startNewGame{ true };

private:
	Settings()
	{
		LoadSettings();
	}

	void LoadSettings()
	{
		constexpr auto path = L"Data/SKSE/Plugins/po3_StartOnSave.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		detail::get_value(ini, specificSave, "Settings", "Save File", ";Auto load specific save. If blank, the last save will be loaded");
		useSpecificSave = !specificSave.empty();

		detail::get_value(ini, charName, "Settings", "Character Name", ";Auto load saves belonging to this character only. If blank, all saves will be considered.");
		useCharName = !charName.empty();

		detail::get_value(ini, type, "Settings", "Save Type", ";Type of save to auto load\n;0 - Last save, 1 - Last quicksave, 2 - Last autosave, 3 - Last manual save.\n;4 - First save, 5 - First quicksave, 6 - First autosave, 7 - First manual save.");
		detail::get_value(ini, KEY, "Settings", "Skip AutoLoad Hotkey", ";Skip autoload by pressing this key (default: SHIFT) before the main menu loads.\n;List of keycodes - https://www.indigorose.com/webhelp/ams/Program_Reference/Misc/Virtual_Key_Codes.htm");
		detail::get_value(ini, startNewGame, "Settings", "Start New Game", ";Automatically start a new game if there are no saves.");
		detail::get_value(ini, disableWarning, "Settings", "Disable Missing Content Warning", ";Disable warning messagebox when loading saves with missing mods.");

		(void)ini.SaveFile(path);
	}

	struct detail
	{
		template <class T>
		static void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
		{
			if constexpr (std::is_same_v<bool, T>) {
				a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
				a_ini.SetBoolValue(a_section, a_key, a_value, a_comment);
			} else if constexpr (std::is_floating_point_v<T>) {
				a_value = static_cast<T>(a_ini.GetDoubleValue(a_section, a_key, a_value));
				a_ini.SetDoubleValue(a_section, a_key, a_value, a_comment);
			} else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
				a_value = string::lexical_cast<T>(a_ini.GetValue(a_section, a_key, std::to_string(a_value).c_str()));
				a_ini.SetValue(a_section, a_key, std::to_string(a_value).c_str(), a_comment);
			} else {
				a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
				a_ini.SetValue(a_section, a_key, a_value.c_str(), a_comment);
			}
		}
	};
};
