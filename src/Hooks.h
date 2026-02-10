#pragma once

namespace StartOnSave
{
	class MenuManager :
		public REX ::Singleton<MenuManager>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
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
	};
}
