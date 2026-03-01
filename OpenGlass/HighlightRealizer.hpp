#pragma once
#include "resource.h"
#include "framework.hpp"
#include "cpprt.hpp"
#include "dwmcoreProjection.hpp"

namespace OpenGlass
{
	struct HighlightContext
	{
		const dwmcore::CMILMatrix* worldTransform;
		const D2D1_RECT_F* viewport;
		const D2D1_RECT_F* viewbox;
		bool sheetOfGlass;
		bool active;
		float opacity;
		float sideOpacity;
	};

	class CHighlightRealizer
	{
		winrt::com_ptr<ID2D1Bitmap1> m_highlightBitmap{ nullptr };
		winrt::com_ptr<ID2D1Bitmap1> m_highlightBitmapInactive{ nullptr };

		winrt::com_ptr<ID2D1Bitmap1> m_sideHighlightBitmap{ nullptr };
		winrt::com_ptr<ID2D1Bitmap1> m_sideHighlightBitmapInactive{ nullptr };
		int m_sizingMargin{ 2 }; // hardcoded for now!!!
		inline HRESULT LoadTexture(ID2D1DeviceContext* context);
		inline HRESULT LoadSideTexture(ID2D1DeviceContext* context);
	public:
		HRESULT Render(
			ID2D1DeviceContext* context,
			const std::span<const D2D1_RECT_F>& rectangles,
			const HighlightContext& highlightContext
		);
		void Reset()
		{
			m_highlightBitmap = nullptr;
			m_highlightBitmapInactive = nullptr;

			m_sideHighlightBitmap = nullptr;
			m_sideHighlightBitmapInactive = nullptr;
		}
	};
}
