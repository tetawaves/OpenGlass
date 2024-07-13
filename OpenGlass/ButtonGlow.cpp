#include "pch.h"
#include "uDwmProjection.hpp"
#include "ButtonGlow.hpp"

#include <ShellScalingApi.h>

using namespace OpenGlass;

namespace OpenGlass::ButtonGlow
{
	HRESULT STDMETHODCALLTYPE MyCTopLevelWindow_DoHitTest(uDwm::CTopLevelWindow* This, tagPOINT* a2, uDwm::CVisual** a3, unsigned int* a4);
	void STDMETHODCALLTYPE MyCTopLevelWindow_NotifyMouseLeave(uDwm::CTopLevelWindow* This);

	void UpdateBackdrop(uDwm::CTopLevelWindow* window, tagPOINT* mouseLocation);

	decltype(&MyCTopLevelWindow_DoHitTest) g_CTopLevelWindow_DoHitTest_Org{ nullptr };
	decltype(&MyCTopLevelWindow_NotifyMouseLeave) g_CTopLevelWindow_NotifyMouseLeave_Org{ nullptr };

	std::unordered_map<uDwm::CTopLevelWindow*, winrt::com_ptr<CButtonGlowVisual>> m_backdropMap{};

	bool ButtonHitTest(SIZE* buttonSize, POINT* buttonPoint, tagPOINT* mouseLocation, int buttonState);
	int btnHold;

	bool g_buttonGlow{ FALSE };
}

winrt::com_ptr<CButtonGlowVisual> ButtonGlow::GetOrCreate(uDwm::CTopLevelWindow* target, bool createIfNecessary)
{
	auto it{ m_backdropMap.find(target) };

	if (createIfNecessary)
	{
		if (it == m_backdropMap.end())
		{
			auto visual = target->GetNonClientVisual();
			auto result{ m_backdropMap.emplace(target, winrt::make_self<CButtonGlowVisual>(visual)) };
			if (result.second == true)
			{
				it = result.first;
			}
		}
	}

	return it == m_backdropMap.end() ? nullptr : it->second;
}

void ButtonGlow::Remove(uDwm::CTopLevelWindow* window)
{
	auto it{ m_backdropMap.find(window) };

	if (it != m_backdropMap.end())
	{
		m_backdropMap.erase(it);
	}
}

bool ButtonGlow::ButtonHitTest(SIZE* buttonSize, POINT* buttonPoint, tagPOINT* mouseLocation, int buttonState)
{
	if (!mouseLocation || buttonState == 3)
		return false;

	if ((mouseLocation->x >= buttonPoint->x && mouseLocation->x <= buttonPoint->x + buttonSize->cx - 1) &&
		(mouseLocation->y >= buttonPoint->y && mouseLocation->y <= buttonPoint->y + buttonSize->cy - 1))
		return true;

	return false;
}

void ButtonGlow::UpdateBackdrop(uDwm::CTopLevelWindow* window, tagPOINT* mouseLocation)
{
	RECT rect;
	window->GetActualWindowRect(&rect, 0, 0, false);
	auto monitor = MonitorFromPoint({ max(0, rect.left), max(0, rect.top) }, 0);
	UINT dpi = 96;
	GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi, &dpi);
	const float scale = (float)dpi / 96.f;


	auto GetButtonPtr = [&](int index) -> uDwm::CButton*
		{
			uDwm::CButton** button;
			if (os::buildNumber < os::build_w11_21h2)
				button = (uDwm::CButton**)((DWORD64)window + 8 * (index + 61));
			else if (os::buildNumber < os::build_w11_22h2)
				button = (uDwm::CButton**)((DWORD64)window + 8 * (index + 63));
			else
				button = (uDwm::CButton**)((DWORD64)window + 8 * (index + 66));
			if (button && *button)
				return *button;
			return nullptr;
		};

	if (auto backdrop = GetOrCreate(window, false))
	{
		for (int index = 3; index > 0; index--)
		{
			if (auto button = GetButtonPtr(index))
			{
				POINT* pt = button->GetPoint();
				SIZE* size = button->GetSize();
				int buttonState = button->GetButtonState();

				if (ButtonHitTest(size, pt, mouseLocation, buttonState))
				{
					if ((GetKeyState(VK_LBUTTON) & 0x80) != 0 && (btnHold != index))
					{
						backdrop->Update(index - 1, *pt, { size->cx, size->cy }, scale, FALSE, TRUE);

						return;
					}
					btnHold = index;
					backdrop->Update(index, *pt, { size->cx, size->cy }, scale, TRUE, FALSE);
				}
				else
				{
					backdrop->Update(index, *pt, { size->cx, size->cy }, scale, FALSE, FALSE);
				}

			}

		}
	}
}

HRESULT STDMETHODCALLTYPE ButtonGlow::MyCTopLevelWindow_DoHitTest(uDwm::CTopLevelWindow* This, tagPOINT* a2, uDwm::CVisual** a3, unsigned int* a4)
{

	if (This->HasTitlebar() && g_buttonGlow)
	{
		UpdateBackdrop(This, a2);
	}

	return g_CTopLevelWindow_DoHitTest_Org(This, a2, a3, a4);
}

void STDMETHODCALLTYPE ButtonGlow::MyCTopLevelWindow_NotifyMouseLeave(uDwm::CTopLevelWindow* This)
{
	g_CTopLevelWindow_NotifyMouseLeave_Org(This);
	UpdateBackdrop(This, NULL);
}

void ButtonGlow::UpdateConfiguration(ConfigurationFramework::UpdateType type)
{
	if (type & ConfigurationFramework::UpdateType::Backdrop)
	{
		g_buttonGlow = static_cast<bool>(ConfigurationFramework::DwmGetDwordFromHKCUAndHKLM(L"ButtonGlow", FALSE));
	}
}

HRESULT ButtonGlow::Startup()
{
	uDwm::GetAddressFromSymbolMap("CTopLevelWindow::DoHitTest", g_CTopLevelWindow_DoHitTest_Org);
	uDwm::GetAddressFromSymbolMap("CTopLevelWindow::NotifyMouseLeave", g_CTopLevelWindow_NotifyMouseLeave_Org);

	return HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Attach(&g_CTopLevelWindow_DoHitTest_Org, MyCTopLevelWindow_DoHitTest);
			HookHelper::Detours::Attach(&g_CTopLevelWindow_NotifyMouseLeave_Org, MyCTopLevelWindow_NotifyMouseLeave);
		});
}

void ButtonGlow::Shutdown()
{
	HookHelper::Detours::Write([]()
		{
			HookHelper::Detours::Detach(&g_CTopLevelWindow_DoHitTest_Org, MyCTopLevelWindow_DoHitTest);
			HookHelper::Detours::Detach(&g_CTopLevelWindow_NotifyMouseLeave_Org, MyCTopLevelWindow_NotifyMouseLeave);
		});

	std::vector<uDwm::CTopLevelWindow*> windowCollection{};
	for (const auto& [window, backdrop] : m_backdropMap)
	{
		windowCollection.push_back(window);
	}
	m_backdropMap.clear();
}