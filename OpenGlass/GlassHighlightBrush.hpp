#pragma once
#include "resource.h"
#include "uDWMProjection.hpp"
#include "Shared.hpp"

namespace OpenGlass::GlassHighlightBrush
{
	D2D1_RECT_F CalculateTargetViewport(
		const POINT& offset = { 0, 0 },
		SIZE size = { 0, 0 },
		const DWM::MilSizeD& scale = { 1.0, 1.0 }
	);

	winrt::com_ptr<uDWM::CImageLegacyMilBrushProxy> GetOrCreate(
		void* owner,
		unsigned int slot,
		bool createIfNecessary = false
	);
	winrt::com_ptr<uDWM::CRgnGeometryProxy> CreateOrUpdateGeometry(
		void* owner,
		unsigned int slot,
		HRGN region = nullptr
	);
	void Remove(void* owner);
	void RemoveAll();
}
