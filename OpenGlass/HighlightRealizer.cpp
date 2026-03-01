#include "pch.h"
#include "Shared.hpp"
#include "uDWMProjection.hpp"
#include "HighlightRealizer.hpp"
#include "GlassEngine.hpp"

using namespace OpenGlass;

HRESULT CHighlightRealizer::LoadTexture(ID2D1DeviceContext* context)
{
	winrt::com_ptr<IStream> stream{ nullptr };
	if (
		Shared::g_highlightTexturePath.empty() ||
		PathIsRelativeW(Shared::g_highlightTexturePath.data()) ||
		PathIsNetworkPathW(Shared::g_highlightTexturePath.data()) ||
		!PathFileExistsW(Shared::g_highlightTexturePath.data())
		)
	{
		LOG_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !Shared::g_highlightTexturePath.empty(), "invalid highlight texture path");
		std::span<const UCHAR> textureBytes{};
		RETURN_IF_FAILED(Util::GetResDataView(textureBytes, IDB_HIGHLIGHT, wil::GetModuleInstanceHandle(), L"PNG"));

		stream = { SHCreateMemStream(textureBytes.data(), static_cast<UINT>(textureBytes.size_bytes())), winrt::take_ownership_from_abi };
	}
	else
	{
		wil::unique_hfile file{ CreateFileW(Shared::g_highlightTexturePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0) };
		RETURN_LAST_ERROR_IF(!file.is_valid());

		LARGE_INTEGER fileSize{};
		RETURN_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &fileSize));

		auto buffer{ std::make_unique<BYTE[]>(static_cast<size_t>(fileSize.QuadPart)) };
		RETURN_IF_WIN32_BOOL_FALSE(ReadFile(file.get(), buffer.get(), static_cast<DWORD>(fileSize.QuadPart), nullptr, nullptr));
		stream = { SHCreateMemStream(buffer.get(), static_cast<UINT>(fileSize.QuadPart)), winrt::take_ownership_from_abi };
	}
	RETURN_HR_IF_NULL(E_OUTOFMEMORY, stream);

	winrt::com_ptr<IWICImagingFactory> wicFactory{ nullptr };
	wicFactory.copy_from(uDWM::CDesktopManager::GetInstance()->GetWICFactory());
	winrt::com_ptr<IWICBitmapDecoder> wicDecoder{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateDecoderFromStream(stream.get(), &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, wicDecoder.put()));
	winrt::com_ptr<IWICBitmapFrameDecode> wicFrame{ nullptr };
	RETURN_IF_FAILED(wicDecoder->GetFrame(0, wicFrame.put()));
	winrt::com_ptr<IWICBitmapClipper> wicClipper{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateBitmapClipper(wicClipper.put()));
	UINT width, height;
	RETURN_IF_FAILED(wicFrame->GetSize(&width, &height));
	WICRect rcClip = { 0, 0, static_cast<int>(width), static_cast<int>(height) / 2 };
	RETURN_IF_FAILED(wicClipper->Initialize(wicFrame.get(), &rcClip));
	winrt::com_ptr<IWICFormatConverter> wicConverter{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
	RETURN_IF_FAILED(
		wicConverter->Initialize(
			wicClipper.get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0,
			WICBitmapPaletteTypeCustom
		)
	);

	RETURN_IF_FAILED(
		context->CreateBitmapFromWicBitmap(
			wicConverter.get(),
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			),
			m_highlightBitmap.put()
		)
	);

	// inactive
	rcClip.Y = static_cast<int>(height) / 2;
	RETURN_IF_FAILED(wicFactory->CreateBitmapClipper(wicClipper.put()));
	RETURN_IF_FAILED(wicClipper->Initialize(wicFrame.get(), &rcClip));
	RETURN_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
	RETURN_IF_FAILED(
		wicConverter->Initialize(
			wicClipper.get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0,
			WICBitmapPaletteTypeCustom
		)
	);

	RETURN_IF_FAILED(
		context->CreateBitmapFromWicBitmap(
			wicConverter.get(),
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			),
			m_highlightBitmapInactive.put()
		)
	);

	return S_OK;
}

HRESULT CHighlightRealizer::LoadSideTexture(ID2D1DeviceContext* context)
{
	winrt::com_ptr<IStream> stream{ nullptr };
	if (
		Shared::g_sideHighlightTexturePath.empty() ||
		PathIsRelativeW(Shared::g_sideHighlightTexturePath.data()) ||
		PathIsNetworkPathW(Shared::g_sideHighlightTexturePath.data()) ||
		!PathFileExistsW(Shared::g_sideHighlightTexturePath.data())
		)
	{
		LOG_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !Shared::g_sideHighlightTexturePath.empty(), "invalid side highlight texture path");
		std::span<const UCHAR> textureBytes{};
		RETURN_IF_FAILED(Util::GetResDataView(textureBytes, IDB_SIDEHIGHLIGHT, wil::GetModuleInstanceHandle(), L"PNG"));

		stream = { SHCreateMemStream(textureBytes.data(), static_cast<UINT>(textureBytes.size_bytes())), winrt::take_ownership_from_abi };
	}
	else
	{
		wil::unique_hfile file{ CreateFileW(Shared::g_sideHighlightTexturePath.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0) };
		RETURN_LAST_ERROR_IF(!file.is_valid());

		LARGE_INTEGER fileSize{};
		RETURN_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &fileSize));

		auto buffer{ std::make_unique<BYTE[]>(static_cast<size_t>(fileSize.QuadPart)) };
		RETURN_IF_WIN32_BOOL_FALSE(ReadFile(file.get(), buffer.get(), static_cast<DWORD>(fileSize.QuadPart), nullptr, nullptr));
		stream = { SHCreateMemStream(buffer.get(), static_cast<UINT>(fileSize.QuadPart)), winrt::take_ownership_from_abi };
	}
	RETURN_HR_IF_NULL(E_OUTOFMEMORY, stream);

	winrt::com_ptr<IWICImagingFactory> wicFactory{ nullptr };
	wicFactory.copy_from(uDWM::CDesktopManager::GetInstance()->GetWICFactory());
	winrt::com_ptr<IWICBitmapDecoder> wicDecoder{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateDecoderFromStream(stream.get(), &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, wicDecoder.put()));
	winrt::com_ptr<IWICBitmapFrameDecode> wicFrame{ nullptr };
	RETURN_IF_FAILED(wicDecoder->GetFrame(0, wicFrame.put()));
	winrt::com_ptr<IWICBitmapClipper> wicClipper{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateBitmapClipper(wicClipper.put()));
	UINT width, height;
	RETURN_IF_FAILED(wicFrame->GetSize(&width, &height));
	WICRect rcClip = { 0, 0, static_cast<int>(width), static_cast<int>(height / 2) };
	RETURN_IF_FAILED(wicClipper->Initialize(wicFrame.get(), &rcClip));
	winrt::com_ptr<IWICFormatConverter> wicConverter{ nullptr };
	RETURN_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
	RETURN_IF_FAILED(
		wicConverter->Initialize(
			wicClipper.get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0,
			WICBitmapPaletteTypeCustom
		)
	);

	RETURN_IF_FAILED(
		context->CreateBitmapFromWicBitmap(
			wicConverter.get(),
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			),
			m_sideHighlightBitmap.put()
		)
	);

	// inactive
	rcClip.Y = static_cast<int>(height / 2);
	RETURN_IF_FAILED(wicFactory->CreateBitmapClipper(wicClipper.put()));
	RETURN_IF_FAILED(wicClipper->Initialize(wicFrame.get(), &rcClip));
	RETURN_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
	RETURN_IF_FAILED(
		wicConverter->Initialize(
			wicClipper.get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			nullptr,
			0,
			WICBitmapPaletteTypeCustom
		)
	);

	RETURN_IF_FAILED(
		context->CreateBitmapFromWicBitmap(
			wicConverter.get(),
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			),
			m_sideHighlightBitmapInactive.put()
		)
	);

	return S_OK;
}

HRESULT CHighlightRealizer::Render(
	ID2D1DeviceContext* context,
	const std::span<const D2D1_RECT_F>& rectangles,
	const HighlightContext& highlightContext
)
{
	if (!m_highlightBitmap || !m_highlightBitmapInactive)
	{
		RETURN_IF_FAILED(LoadTexture(context));
	}

	if (!highlightContext.sheetOfGlass && !m_sideHighlightBitmap || !m_sideHighlightBitmapInactive)
	{
		RETURN_IF_FAILED(LoadSideTexture(context));
	}

	const auto worldTransform3D = highlightContext.worldTransform->GetD3DMatrix();
	auto worldTransform2DInversed = highlightContext.worldTransform->GetD2DMatrix();
	D2D1InvertMatrix(&worldTransform2DInversed);

	const auto highlightBitmapSize = highlightContext.active ? m_highlightBitmap->GetSize() : m_highlightBitmapInactive->GetSize();
	const auto sideHighlightBitmapSize = highlightContext.active ? m_sideHighlightBitmap->GetSize() : m_sideHighlightBitmapInactive->GetSize();

	const D2D1_SIZE_F viewportSize
	{
		wil::rect_width(*highlightContext.viewport),
		wil::rect_height(*highlightContext.viewport)
	};

	const D2D1_RECT_F frameMargins
	{
		highlightContext.viewbox->left,
		highlightContext.viewbox->top,
		highlightContext.viewbox->right,
		highlightContext.viewbox->bottom,
	};

	const auto clip = highlightContext.viewport->left + (viewportSize.width / 2.0f);

	const D2D1_RECT_F leftDestRectangle
	{
		highlightContext.viewport->left,
		highlightContext.viewport->top,
		std::min(highlightContext.viewport->left + highlightBitmapSize.width, clip),
		highlightContext.viewport->top + highlightBitmapSize.height
	};

	const D2D1_RECT_F rightDestRectangle
	{
		std::max(highlightContext.viewport->right - highlightBitmapSize.width, clip),
		highlightContext.viewport->top,
		highlightContext.viewport->right,
		highlightContext.viewport->top + highlightBitmapSize.height
	};

	const auto cx = 2.0f * (highlightContext.viewport->right - highlightBitmapSize.width + (highlightBitmapSize.width / 2.0f));

	const D2D1_MATRIX_4X4_F flipWorldTransform3D = D2D1::Matrix4x4F(
		-worldTransform3D._11, -worldTransform3D._12, -worldTransform3D._13, -worldTransform3D._14,
		worldTransform3D._21, worldTransform3D._22, worldTransform3D._23, worldTransform3D._24,
		worldTransform3D._31, worldTransform3D._32, worldTransform3D._33, worldTransform3D._34,
		cx * worldTransform3D._11 + worldTransform3D._41,
		cx * worldTransform3D._12 + worldTransform3D._42,
		cx * worldTransform3D._13 + worldTransform3D._43,
		cx * worldTransform3D._14 + worldTransform3D._44
	);

	const auto leftSideWidth = frameMargins.left - 1;
	const auto rightSideWidth = frameMargins.right - 1;
	const auto cxSide = 2.0f * (highlightContext.viewport->right - rightSideWidth + (rightSideWidth / 2.0f));

	const D2D1_MATRIX_4X4_F flipSideWorldTransform3D = D2D1::Matrix4x4F(
		-worldTransform3D._11, -worldTransform3D._12, -worldTransform3D._13, -worldTransform3D._14,
		worldTransform3D._21, worldTransform3D._22, worldTransform3D._23, worldTransform3D._24,
		worldTransform3D._31, worldTransform3D._32, worldTransform3D._33, worldTransform3D._34,
		cxSide * worldTransform3D._11 + worldTransform3D._41,
		cxSide * worldTransform3D._12 + worldTransform3D._42,
		cxSide * worldTransform3D._13 + worldTransform3D._43,
		cxSide * worldTransform3D._14 + worldTransform3D._44
	);

	// not accurate with DWM and probably it will never be
	// some bullshit i dont care
	const auto leftScaleX = sideHighlightBitmapSize.width / leftSideWidth;
	const auto rightScaleX = sideHighlightBitmapSize.width / rightSideWidth;

	const auto topOffset = viewportSize.height * (frameMargins.top / highlightContext.viewport->bottom);
	const auto height = viewportSize.height *
	(
		std::max(0.0f, highlightContext.viewport->bottom - frameMargins.top - static_cast<int>(frameMargins.bottom + (std::max(0.0f, highlightContext.viewport->bottom - frameMargins.top - frameMargins.bottom) * 0.75f))) / highlightContext.viewport->bottom
	);

	const auto destinationHeight = std::max(0.0f, height - std::min(height, static_cast<float>(m_sizingMargin)));
	const auto sourceHeight = std::max(0.0f, sideHighlightBitmapSize.height - static_cast<float>(m_sizingMargin));
	const auto scaleY = sourceHeight / destinationHeight;

	const D2D1_RECT_F leftSideDestRect
	{
		highlightContext.viewport->left,
		highlightContext.viewport->top + topOffset,
		highlightContext.viewport->left + leftSideWidth,
		(highlightContext.viewport->top + topOffset) + destinationHeight
	};
	const D2D1_RECT_F leftSideDestRectBottom
	{
		highlightContext.viewport->left,
		(highlightContext.viewport->top + topOffset) + destinationHeight,
		highlightContext.viewport->left + leftSideWidth,
		(highlightContext.viewport->top + topOffset) + height
	};

	const D2D1_RECT_F rightSideDestRect
	{
		highlightContext.viewport->right - rightSideWidth,
		highlightContext.viewport->top + topOffset,
		highlightContext.viewport->right,
		(highlightContext.viewport->top + topOffset) + destinationHeight
	};
	const D2D1_RECT_F rightSideDestRectBottom
	{
		highlightContext.viewport->right - rightSideWidth,
		(highlightContext.viewport->top + topOffset) + destinationHeight,
		highlightContext.viewport->right,
		(highlightContext.viewport->top + topOffset) + height
	};

	for (auto subRectangle : rectangles)
	{
		subRectangle = RectF::TransformRect(subRectangle, worldTransform2DInversed);

		auto destRectangle = subRectangle;
		if (RectF::IntersectUnsafe(destRectangle, leftDestRectangle))
		{
			D2D1_RECT_F sourceRectangle
			{
				destRectangle.left - highlightContext.viewport->left,
				destRectangle.top - highlightContext.viewport->top,
				destRectangle.right - highlightContext.viewport->left,
				destRectangle.bottom - highlightContext.viewport->top
			};
			context->DrawBitmap(
				highlightContext.active ? m_highlightBitmap.get() : m_highlightBitmapInactive.get(),
				destRectangle,
				highlightContext.opacity,
				D2D1_INTERPOLATION_MODE_LINEAR,
				&sourceRectangle,
				&worldTransform3D
			);
		}

		destRectangle = subRectangle;
		if (RectF::IntersectUnsafe(destRectangle, rightDestRectangle))
		{
			D2D1_RECT_F flipDestRectangle = destRectangle;
			flipDestRectangle.left = cx - destRectangle.right;
			flipDestRectangle.right = cx - destRectangle.left;
			D2D1_RECT_F sourceRectangle
			{
				flipDestRectangle.left - (highlightContext.viewport->right - highlightBitmapSize.width),
				flipDestRectangle.top - highlightContext.viewport->top,
				flipDestRectangle.right - (highlightContext.viewport->right - highlightBitmapSize.width),
				flipDestRectangle.bottom - highlightContext.viewport->top
			};
			context->DrawBitmap(
				highlightContext.active ? m_highlightBitmap.get() : m_highlightBitmapInactive.get(),
				flipDestRectangle,
				highlightContext.opacity,
				D2D1_INTERPOLATION_MODE_LINEAR,
				&sourceRectangle,
				&flipWorldTransform3D
			);
		}

		if (!highlightContext.sheetOfGlass)
		{
			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, leftSideDestRect))
			{
				D2D1_RECT_F sourceRectangle
				{
					(destRectangle.left - leftSideDestRect.left) * leftScaleX,
					(destRectangle.top - leftSideDestRect.top) * scaleY,
					(destRectangle.right - leftSideDestRect.left) * leftScaleX,
					(destRectangle.bottom - leftSideDestRect.top) * scaleY
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					destRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&worldTransform3D
				);
			}
			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, leftSideDestRectBottom))
			{
				D2D1_RECT_F sourceRectangle
				{
					(destRectangle.left - leftSideDestRectBottom.left) * leftScaleX,
					sourceHeight + (destRectangle.top - leftSideDestRectBottom.top),
					(destRectangle.right - leftSideDestRectBottom.left) * leftScaleX,
					sourceHeight + (destRectangle.bottom - leftSideDestRectBottom.top)
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					destRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&worldTransform3D
				);
			}

			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, rightSideDestRect))
			{
				D2D1_RECT_F flipDestRectangle = destRectangle;
				flipDestRectangle.left = cxSide - destRectangle.right;
				flipDestRectangle.right = cxSide - destRectangle.left;

				D2D1_RECT_F sourceRectangle
				{
					(flipDestRectangle.left - rightSideDestRect.left) * rightScaleX,
					(flipDestRectangle.top - rightSideDestRect.top) * scaleY,
					(flipDestRectangle.right - rightSideDestRect.left) * rightScaleX,
					(flipDestRectangle.bottom - rightSideDestRect.top) * scaleY
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					flipDestRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&flipSideWorldTransform3D
				);
			}
			destRectangle = subRectangle;
			if (RectF::IntersectUnsafe(destRectangle, rightSideDestRectBottom))
			{
				D2D1_RECT_F flipDestRectangle = destRectangle;
				flipDestRectangle.left = cxSide - destRectangle.right;
				flipDestRectangle.right = cxSide - destRectangle.left;

				D2D1_RECT_F sourceRectangle
				{
					(flipDestRectangle.left - rightSideDestRectBottom.left) * rightScaleX,
					sourceHeight + (flipDestRectangle.top - rightSideDestRectBottom.top),
					(flipDestRectangle.right - rightSideDestRectBottom.left) * rightScaleX,
					sourceHeight + (flipDestRectangle.bottom - rightSideDestRectBottom.top)
				};
				context->DrawBitmap(
					highlightContext.active ? m_sideHighlightBitmap.get() : m_sideHighlightBitmapInactive.get(),
					flipDestRectangle,
					highlightContext.sideOpacity,
					D2D1_INTERPOLATION_MODE_LINEAR,
					&sourceRectangle,
					&flipSideWorldTransform3D
				);
			}
		}
	}

	return S_OK;
}
