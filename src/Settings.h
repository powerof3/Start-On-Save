#pragma once

#include <SimpleIni.h>
#undef ERROR

class Settings : public REX::TSingleton<Settings>
{
public:
	template <class T>
	class Setting : public REX::TIniSetting<T>
	{
	public:
		Setting(std::string_view a_section, std::string_view a_oldKey, std::string_view a_newKey, T a_default) :
			REX::TIniSetting<T>(a_section, a_newKey, a_default),
			oldKey(a_oldKey),
			newKey(a_newKey)
		{
			GetSettingsToUpdate().emplace_back(a_section, a_oldKey, a_newKey);
		}

	private:
		std::string_view newKey;
		std::string_view oldKey;
	};

	using Bool = Setting<bool>;
	using F32 = Setting<float>;
	using F64 = Setting<double>;
	using I8 = Setting<std::int8_t>;
	using I16 = Setting<std::int16_t>;
	using I32 = Setting<std::int32_t>;
	using U8 = Setting<std::uint8_t>;
	using U16 = Setting<std::uint16_t>;
	using U32 = Setting<std::uint32_t>;
	using Str = Setting<std::string>;

	struct SettingsToUpdate
	{
		std::string_view section;
		std::string_view oldKey;
		std::string_view newKey;
	};

	void LoadSettings()
	{
		std::error_code ec;
		if (!std::filesystem::exists(path, ec)) {
			CSimpleIniA ini;
			ini.LoadFile(path);
			(void)ini.SaveFile(path);
		} else {
			UpdateINISettings();
		}
		
		const auto store = REX::FIniSettingStore::GetSingleton();
		store->Init(path, "");
		store->Load();

		useSpecificSave = !specificSave.GetValue().empty();
		useCharName = !charName.GetValue().empty();

		store->Save();
	}

	void UpdateINISettings()
	{
		CSimpleIniA ini;
		ini.SetUnicode();

		if (ini.LoadFile(path) < SI_OK) {
			return;
		}

		if (ini.GetValue("Settings", "sSaveFile")) {
			REX::INFO("No settings to migrate...");
			return;
		}

		for (auto& [section, oldKey, newKey] : GetSettingsToUpdate()) {
			CSimpleIniA::TNamesDepend values;
			if (ini.GetAllValues(section.data(), oldKey.data(), values) && !values.empty()) {
				const auto& entry = values.front();
				ini.SetValue(section.data(), newKey.data(), entry.pItem, entry.pComment);
				ini.Delete(section.data(), oldKey.data(), true);
				REX::INFO("Migrated [{}] {} -> {}", section, oldKey, newKey);
			}
		}

		(void)ini.SaveFile(path);
	}

	bool CheckKeyPress()
	{
		if (skipLoading) {
			return true;
		}

		if (GetAsyncKeyState(KEY.GetValue()) & 0x8000) {
			skipLoading = true;
		}

		return skipLoading;
	}

	[[nodiscard]] bool GetValidSave(const RE::BSFixedString& a_name, std::int32_t a_offset = 0) const
	{
		const auto t = type.GetValue();
		return t == (0 + a_offset) ||
		       t == (1 + a_offset) && REX::STR::ICONTAINS(a_name, "Quicksave") ||
		       t == (2 + a_offset) && REX::STR::ICONTAINS(a_name, "Autosave") ||
		       t == (3 + a_offset) && REX::STR::ICONTAINS(a_name, "Save");
	}

	static std::vector<SettingsToUpdate>& GetSettingsToUpdate()
	{
		static std::vector<SettingsToUpdate> settingsToUpdate;
		return settingsToUpdate;
	};

	// members
	static constexpr auto path = R"(Data\SKSE\Plugins\po3_StartOnSave.ini)";

	Str  specificSave{ "Settings", "Save File", "sSaveFile", "" };
	Str  charName{ "Settings", "Character Name", "sCharacterName", "" };
	I32  type{ "Settings", "Save Type", "iSaveType", 0 };
	I32  KEY{ "Settings", "Skip AutoLoad Hotkey", "bSkipAutoLoadHotkey", 16 };
	Bool startNewGame{ "Settings", "Start New Game", "bStartNewGame", true };
	Bool disableWarning{ "Settings", "Disable Missing Content Warning", "bDisableMissingContentWarning", false };

	bool useCharName{ false };
	bool useSpecificSave{ false };
	bool skipLoading{ false };
};
