#pragma once

namespace StartOnSave
{
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
			if (auto menuSrc = RE::UI::GetSingleton()) {
				menuSrc->AddEventSink(GetSingleton());
			}
		}

		static void Unregister()
		{
			if (auto menuSrc = RE::UI::GetSingleton()) {
				menuSrc->RemoveEventSink(GetSingleton());
			}
		}

	protected:
		using EventResult = RE::BSEventNotifyControl;

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

    private:
		MenuManager() = default;
		MenuManager(const MenuManager&) = delete;
		MenuManager(MenuManager&&) = delete;

		~MenuManager() override = default;

		MenuManager& operator=(const MenuManager&) = delete;
		MenuManager& operator=(MenuManager&&) = delete;
	};
}
