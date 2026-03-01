#include "pch.h"
#include "GlassHighlightBrush.hpp"

using namespace OpenGlass;
namespace OpenGlass::GlassHighlightBrush
{
	std::unordered_map<void*, std::array<winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy>, 2>> g_glassHighlightBrushMap{};
	std::unordered_map<void*, std::array<winrt::com_ptr<uDWM::CRgnGeometryProxy>, 2>> g_glassHighlightGeometryMap{};
}

D2D1_RECT_F GlassHighlightBrush::CalculateTargetViewport(
	const POINT& offset,
	SIZE size,
	const DWM::MilSizeD& scale
)
{
	D2D1_RECT_F viewport{};

	viewport.left = static_cast<float>(offset.x);
	viewport.top = static_cast<float>(offset.y);
	viewport.right = static_cast<float>(size.cx);
	viewport.bottom = static_cast<float>(size.cy);

	viewport.left /= static_cast<float>(scale.width);
	viewport.top /= static_cast<float>(scale.height);
	viewport.right /= static_cast<float>(scale.width);
	viewport.bottom /= static_cast<float>(scale.height);

	return viewport;
}

winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy> GlassHighlightBrush::GetOrCreate(
	void* owner,
	unsigned int slot,
	bool createIfNecessary
)
{
	auto& array = g_glassHighlightBrushMap.try_emplace(owner, std::array<winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy>, 2>{}).first->second;
	auto& brush = array.at(slot);

	if (!brush && createIfNecessary)
	{
		THROW_IF_FAILED(
			uDWM::CDesktopManager::GetInstance()->GetCompositor()->CreateImageLegacyMilBrushProxy(
				brush.put()
			)
		);
	}

	return brush;
}

winrt::com_ptr<uDWM::CRgnGeometryProxy> GlassHighlightBrush::CreateOrUpdateGeometry(
	void* owner,
	unsigned int slot,
	HRGN region
)
{
	auto& array = g_glassHighlightGeometryMap.try_emplace(owner, std::array<winrt::com_ptr<uDWM::CRgnGeometryProxy>, 2>{}).first->second;
	auto& geometry = array.at(slot);

	if (!geometry && !region)
	{
		uDWM::CRgnGeometryProxy* proxy = geometry.detach();
		wil::unique_hrgn emptyRegion{ CreateRectRgn(0, 0, 0, 0) };

		THROW_IF_FAILED(
			uDWM::ResourceHelper::CreateGeometryFromHRGN(
				emptyRegion.get(),
				&proxy
			)
		);

		geometry.attach(proxy);
	}

	if (region)
	{
		uDWM::CRgnGeometryProxy* proxy = geometry.detach();

		THROW_IF_FAILED(
			uDWM::ResourceHelper::CreateGeometryFromHRGN(
				region,
				&proxy
			)
		);

		geometry.attach(proxy);
	}

	return geometry;
}

void GlassHighlightBrush::Remove(void* owner)
{
	auto itBrush = g_glassHighlightBrushMap.find(owner);

	if (itBrush != g_glassHighlightBrushMap.end())
	{
		g_glassHighlightBrushMap.erase(itBrush);
	}

	auto itGeometry = g_glassHighlightGeometryMap.find(owner);

	if (itGeometry != g_glassHighlightGeometryMap.end())
	{
		g_glassHighlightGeometryMap.erase(itGeometry);
	}
}
void GlassHighlightBrush::RemoveAll()
{
	g_glassHighlightBrushMap.clear();
	g_glassHighlightGeometryMap.clear();
}
