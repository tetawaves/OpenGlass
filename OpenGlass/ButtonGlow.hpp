#pragma once
#include "resource.h"
#include "framework.hpp"
#include "cpprt.hpp"
#include "wucUtils.hpp"
#include "ConfigurationFramework.hpp"

namespace OpenGlass
{
	class CButtonGlowVisual : public winrt::implements<CButtonGlowVisual, IUnknown>, uDwm::COverlayVisual
	{
		static inline wuc::CompositionDrawingSurface s_glowCloseTexture{ nullptr };
		static inline wuc::CompositionDrawingSurface s_glowOtherTexture{ nullptr };
		static inline winrt::com_ptr<dcomp::IDCompositionDesktopDevicePartner> s_dcompDevice{ nullptr };
		static inline wg::SizeInt32 s_glowCloseSize{};
		static inline wg::SizeInt32 s_glowOtherSize{};

		wuc::CompositionSurfaceBrush m_textureSurfaceBrush[2]{ { nullptr}, { nullptr } };
		wuc::CompositionNineGridBrush m_textureNineGridBrush[2]{ { nullptr}, { nullptr } };
		wuc::CompositionColorBrush m_opacityBrush{ nullptr };
		wuc::SpriteVisual m_spriteVisual[3]{ {nullptr}, {nullptr},{nullptr} };

		bool m_visible[3] = { false };
	public:
		CButtonGlowVisual(uDwm::CCanvasVisual* visual) : uDwm::COverlayVisual(visual)
		{
			CButtonGlowVisual::InitializeVisual();
		}
		~CButtonGlowVisual()
		{
			UninitializeVisual();
		}

		HRESULT InitializeVisual() override
		{
			uDwm::COverlayVisual::InitializeInteropDevice(uDwm::CDesktopManager::s_pDesktopManagerInstance->GetDCompDevice());
			RETURN_IF_FAILED(uDwm::COverlayVisual::InitializeVisual());
			EnsureTextureSurface();

			auto compositor{ m_dcompDevice.as<wuc::Compositor>() };
			m_spriteVisual[0] = compositor.CreateSpriteVisual();
			m_spriteVisual[1] = compositor.CreateSpriteVisual();
			m_spriteVisual[2] = compositor.CreateSpriteVisual();

			m_visualCollection.InsertAtTop(m_spriteVisual[2]);
			m_visualCollection.InsertAtTop(m_spriteVisual[1]);
			m_visualCollection.InsertAtTop(m_spriteVisual[0]);

			m_textureNineGridBrush[0] = compositor.CreateNineGridBrush();
			m_textureNineGridBrush[1] = compositor.CreateNineGridBrush();

			m_textureSurfaceBrush[0] = compositor.CreateSurfaceBrush(s_glowCloseTexture);
			m_textureSurfaceBrush[0].Stretch(wuc::CompositionStretch::Fill);

			m_textureSurfaceBrush[1] = compositor.CreateSurfaceBrush(s_glowOtherTexture);
			m_textureSurfaceBrush[1].Stretch(wuc::CompositionStretch::Fill);

			m_textureNineGridBrush[0].Source(m_textureSurfaceBrush[0]);
			m_textureNineGridBrush[0].SetInsets(15.f, 10.f, 15.f, 10.f);
			m_textureNineGridBrush[1].Source(m_textureSurfaceBrush[1]);
			m_textureNineGridBrush[1].SetInsets(15.f, 10.f, 15.f, 10.f);

			m_opacityBrush = compositor.CreateColorBrush();
			m_spriteVisual[0].Brush(m_opacityBrush);
			m_spriteVisual[1].Brush(m_opacityBrush);
			m_spriteVisual[2].Brush(m_opacityBrush);

			return S_OK;
		}

		void UninitializeVisual()
		{
			m_opacityBrush = nullptr;
			m_spriteVisual[0] = nullptr;
			m_spriteVisual[1] = nullptr;
			m_spriteVisual[2] = nullptr;
		}
		void Update(int index, POINT offset, SIZE size, float scale, bool show, bool hack) try
		{
			EnsureTextureSurface();

			m_textureSurfaceBrush[0].Surface(s_glowCloseTexture);
			m_textureSurfaceBrush[1].Surface(s_glowOtherTexture);

			index--;

			if (index == 2)
			{
				m_spriteVisual[index].Offset(
					winrt::Windows::Foundation::Numerics::float3{ (float)offset.x - 9.5f, (float)offset.y - 7.f, 0.f }
				);
				m_spriteVisual[index].Size(winrt::Windows::Foundation::Numerics::float2
					{
						(float)size.cx + 19.5f + scale, (float)size.cy + 16.f + round(scale)
					});

			}
			else if (index == 1 || index == 0)
			{
				m_spriteVisual[index].Offset(
					winrt::Windows::Foundation::Numerics::float3{ (float)offset.x - 9.5f, (float)offset.y - 7.f, 0.f }
				);
				m_spriteVisual[index].Size(winrt::Windows::Foundation::Numerics::float2
					{
						(float)size.cx + 19.5f + scale, (float)size.cy + 16.f + round(scale)
					});
			}
			else
				return;

			auto newBrush = show ? m_textureNineGridBrush[index != 2].as<wuc::CompositionBrush>() : m_opacityBrush.as<wuc::CompositionBrush>();
			auto currentBrush = m_visible[index] ? m_textureNineGridBrush[index != 2].as<wuc::CompositionBrush>() : m_opacityBrush.as<wuc::CompositionBrush>();

			if (m_visible[index] != show)
			{
				auto compositor{ m_dcompDevice.as<wuc::Compositor>() };

				auto crossfadeBrush{ Utils::CreateCrossFadeBrush(compositor, currentBrush, newBrush) };

				m_visible[index] = show;
				m_spriteVisual[index].Brush(crossfadeBrush);

				if (hack)
				{
					auto animation{ compositor.CreateScalarKeyFrameAnimation() };
					auto easing{ static_cast<wuc::CompositionEasingFunction>(compositor.CreateLinearEasingFunction()) };
					animation.InsertKeyFrame(0.0f, 0.0f);
					animation.InsertKeyFrame(0.0f, 1.0f, easing);
					animation.Duration(std::chrono::milliseconds(60));
					crossfadeBrush.StartAnimation(L"Crossfade.Weight", animation);
				}
				else
				{
					crossfadeBrush.StartAnimation(
						L"Crossfade.Weight",
						Utils::CreateCrossFadeAnimation(
							compositor,
							0,
							std::chrono::milliseconds(60)
						)
					);
				}
			}
		}
		CATCH_LOG_RETURN()
			static void EnsureTextureSurface()
		{
			if (!uDwm::CheckDeviceState(s_dcompDevice))
			{
				s_glowCloseTexture = nullptr;
				s_glowOtherTexture = nullptr;
			}
			if (!s_glowCloseTexture)
			{
				UpdateCloseTextureSurface();
			}
			if (!s_glowOtherTexture)
			{
				UpdateOtherTextureSurface();

			}
		}
		static void UpdateCloseTextureSurface() try
		{
			winrt::com_ptr<IStream> stream{ nullptr };
			if (s_glowCloseTexture)
			{
				return;
			}

			HMODULE currentModule{ wil::GetModuleInstanceHandle() };
			auto resourceHandle{ FindResourceW(currentModule, MAKEINTRESOURCE(IDB_GLOWCLOSE), L"PNG") };
			THROW_LAST_ERROR_IF_NULL(resourceHandle);
			auto globalHandle{ LoadResource(currentModule, resourceHandle) };
			THROW_LAST_ERROR_IF_NULL(globalHandle);
			auto cleanUp = wil::scope_exit([&]
				{
					if (globalHandle)
					{
						UnlockResource(globalHandle);
						FreeResource(globalHandle);
					}
				});
			DWORD resourceSize{ SizeofResource(currentModule, resourceHandle) };
			THROW_LAST_ERROR_IF(resourceSize == 0);
			auto resourceAddress{ reinterpret_cast<PBYTE>(LockResource(globalHandle)) };
			stream = { SHCreateMemStream(resourceAddress, resourceSize), winrt::take_ownership_from_abi };

			s_dcompDevice.copy_from(uDwm::CDesktopManager::s_pDesktopManagerInstance->GetDCompDevice());
			winrt::com_ptr<IWICImagingFactory2> wicFactory{ nullptr };
			wicFactory.copy_from(uDwm::CDesktopManager::s_pDesktopManagerInstance->GetWICFactory());
			winrt::com_ptr<IWICBitmapDecoder> wicDecoder{ nullptr };
			THROW_IF_FAILED(wicFactory->CreateDecoderFromStream(stream.get(), &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, wicDecoder.put()));
			winrt::com_ptr<IWICBitmapFrameDecode> wicFrame{ nullptr };
			THROW_IF_FAILED(wicDecoder->GetFrame(0, wicFrame.put()));
			winrt::com_ptr<IWICFormatConverter> wicConverter{ nullptr };
			THROW_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
			winrt::com_ptr<IWICPalette> wicPalette{ nullptr };
			THROW_IF_FAILED(
				wicConverter->Initialize(
					wicFrame.get(),
					GUID_WICPixelFormat32bppPBGRA,
					WICBitmapDitherTypeNone,
					wicPalette.get(),
					0, WICBitmapPaletteTypeCustom
				)
			);
			winrt::com_ptr<IWICBitmap> wicBitmap{ nullptr };
			THROW_IF_FAILED(wicFactory->CreateBitmapFromSource(wicConverter.get(), WICBitmapCreateCacheOption::WICBitmapNoCache, wicBitmap.put()));

			UINT width{ 0 }, height{ 0 };
			THROW_IF_FAILED(wicBitmap->GetSize(&width, &height));

			if (!s_glowCloseTexture)
			{
				auto compositor{ s_dcompDevice.as<wuc::Compositor>() };
				wuc::CompositionGraphicsDevice graphicsDevice{ nullptr };
				THROW_IF_FAILED(
					compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>()->CreateGraphicsDevice(
						uDwm::CDesktopManager::s_pDesktopManagerInstance->GetD2DDevice(),
						reinterpret_cast<ABI::Windows::UI::Composition::ICompositionGraphicsDevice**>(winrt::put_abi(graphicsDevice))
					)
				);
				s_glowCloseTexture = graphicsDevice.CreateDrawingSurface(
					{ static_cast<float>(width), static_cast<float>(height) },
					wgd::DirectXPixelFormat::B8G8R8A8UIntNormalized,
					wgd::DirectXAlphaMode::Premultiplied
				);
			}

			auto drawingSurfaceInterop{ s_glowCloseTexture.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>() };
			POINT offset = { 0, 0 };
			winrt::com_ptr<ID2D1DeviceContext> d2dContext{ nullptr };
			THROW_IF_FAILED(
				drawingSurfaceInterop->BeginDraw(nullptr, IID_PPV_ARGS(d2dContext.put()), &offset)
			);
			d2dContext->Clear();
			winrt::com_ptr<ID2D1Bitmap1> d2dBitmap{ nullptr };
			d2dContext->CreateBitmapFromWicBitmap(
				wicBitmap.get(),
				D2D1::BitmapProperties1(
					D2D1_BITMAP_OPTIONS_NONE,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
				),
				d2dBitmap.put()
			);
			d2dContext->DrawBitmap(d2dBitmap.get());
			THROW_IF_FAILED(
				drawingSurfaceInterop->EndDraw()
			);
		}
		CATCH_LOG_RETURN()
			static void UpdateOtherTextureSurface() try
		{
			winrt::com_ptr<IStream> stream{ nullptr };
			if (s_glowOtherTexture)
			{
				return;
			}

			HMODULE currentModule{ wil::GetModuleInstanceHandle() };
			auto resourceHandle{ FindResourceW(currentModule, MAKEINTRESOURCE(IDB_GLOWOTHER), L"PNG") };
			THROW_LAST_ERROR_IF_NULL(resourceHandle);
			auto globalHandle{ LoadResource(currentModule, resourceHandle) };
			THROW_LAST_ERROR_IF_NULL(globalHandle);
			auto cleanUp = wil::scope_exit([&]
				{
					if (globalHandle)
					{
						UnlockResource(globalHandle);
						FreeResource(globalHandle);
					}
				});
			DWORD resourceSize{ SizeofResource(currentModule, resourceHandle) };
			THROW_LAST_ERROR_IF(resourceSize == 0);
			auto resourceAddress{ reinterpret_cast<PBYTE>(LockResource(globalHandle)) };
			stream = { SHCreateMemStream(resourceAddress, resourceSize), winrt::take_ownership_from_abi };

			s_dcompDevice.copy_from(uDwm::CDesktopManager::s_pDesktopManagerInstance->GetDCompDevice());
			winrt::com_ptr<IWICImagingFactory2> wicFactory{ nullptr };
			wicFactory.copy_from(uDwm::CDesktopManager::s_pDesktopManagerInstance->GetWICFactory());
			winrt::com_ptr<IWICBitmapDecoder> wicDecoder{ nullptr };
			THROW_IF_FAILED(wicFactory->CreateDecoderFromStream(stream.get(), &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, wicDecoder.put()));
			winrt::com_ptr<IWICBitmapFrameDecode> wicFrame{ nullptr };
			THROW_IF_FAILED(wicDecoder->GetFrame(0, wicFrame.put()));
			winrt::com_ptr<IWICFormatConverter> wicConverter{ nullptr };
			THROW_IF_FAILED(wicFactory->CreateFormatConverter(wicConverter.put()));
			winrt::com_ptr<IWICPalette> wicPalette{ nullptr };
			THROW_IF_FAILED(
				wicConverter->Initialize(
					wicFrame.get(),
					GUID_WICPixelFormat32bppPBGRA,
					WICBitmapDitherTypeNone,
					wicPalette.get(),
					0, WICBitmapPaletteTypeCustom
				)
			);
			winrt::com_ptr<IWICBitmap> wicBitmap{ nullptr };
			THROW_IF_FAILED(wicFactory->CreateBitmapFromSource(wicConverter.get(), WICBitmapCreateCacheOption::WICBitmapNoCache, wicBitmap.put()));

			UINT width{ 0 }, height{ 0 };
			THROW_IF_FAILED(wicBitmap->GetSize(&width, &height));

			if (!s_glowOtherTexture)
			{
				auto compositor{ s_dcompDevice.as<wuc::Compositor>() };
				wuc::CompositionGraphicsDevice graphicsDevice{ nullptr };
				THROW_IF_FAILED(
					compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>()->CreateGraphicsDevice(
						uDwm::CDesktopManager::s_pDesktopManagerInstance->GetD2DDevice(),
						reinterpret_cast<ABI::Windows::UI::Composition::ICompositionGraphicsDevice**>(winrt::put_abi(graphicsDevice))
					)
				);
				s_glowOtherTexture = graphicsDevice.CreateDrawingSurface(
					{ static_cast<float>(width), static_cast<float>(height) },
					wgd::DirectXPixelFormat::B8G8R8A8UIntNormalized,
					wgd::DirectXAlphaMode::Premultiplied
				);
			}

			auto drawingSurfaceInterop{ s_glowOtherTexture.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>() };
			POINT offset = { 0, 0 };
			winrt::com_ptr<ID2D1DeviceContext> d2dContext{ nullptr };
			THROW_IF_FAILED(
				drawingSurfaceInterop->BeginDraw(nullptr, IID_PPV_ARGS(d2dContext.put()), &offset)
			);
			d2dContext->Clear();
			winrt::com_ptr<ID2D1Bitmap1> d2dBitmap{ nullptr };
			d2dContext->CreateBitmapFromWicBitmap(
				wicBitmap.get(),
				D2D1::BitmapProperties1(
					D2D1_BITMAP_OPTIONS_NONE,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
				),
				d2dBitmap.put()
			);
			d2dContext->DrawBitmap(d2dBitmap.get());
			THROW_IF_FAILED(
				drawingSurfaceInterop->EndDraw()
			);
		}
		CATCH_LOG_RETURN()
			static void Shutdown()
		{
			s_glowCloseTexture = nullptr;
			s_glowOtherTexture = nullptr;
			s_dcompDevice = nullptr;
		}
	};
}

namespace OpenGlass::ButtonGlow
{
	winrt::com_ptr<CButtonGlowVisual> GetOrCreate(uDwm::CTopLevelWindow* window, bool createIfNecessary = false);
	void Remove(uDwm::CTopLevelWindow* window);

	void UpdateConfiguration(ConfigurationFramework::UpdateType type);

	HRESULT Startup();
	void Shutdown();
}