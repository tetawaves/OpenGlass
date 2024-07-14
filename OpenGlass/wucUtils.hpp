#pragma once
#include "winrt.hpp"
#include "cpprt.hpp"
#include "CrossFadeEffect.hpp"

namespace OpenGlass::Utils
{
	FORCEINLINE wuc::CompositionBrush CreateCrossFadeBrush(
		const wuc::Compositor& compositor,
		const wuc::CompositionBrush& from,
		const wuc::CompositionBrush& to
	)
	{
		auto crossFadeEffect{ winrt::make_self<Win2D::CrossFadeEffect>() };
		crossFadeEffect->SetName(L"Crossfade");
		crossFadeEffect->SetSource(wuc::CompositionEffectSourceParameter{ L"From" });
		crossFadeEffect->SetDestination(wuc::CompositionEffectSourceParameter{ L"To" });
		crossFadeEffect->SetWeight(0);

		auto crossFadeEffectBrush{ compositor.CreateEffectFactory(*crossFadeEffect, {L"Crossfade.Weight"}).CreateBrush() };
		crossFadeEffectBrush.Comment(L"Crossfade");
		crossFadeEffectBrush.SetSourceParameter(L"From", from);
		crossFadeEffectBrush.SetSourceParameter(L"To", to);
		return crossFadeEffectBrush;
	}

	FORCEINLINE wuc::ScalarKeyFrameAnimation CreateCrossFadeAnimation(
		const wuc::Compositor& compositor,
		char easingFunction,
		winrt::Windows::Foundation::TimeSpan const& crossfadeTime
	)
	{
		auto animation{ compositor.CreateScalarKeyFrameAnimation() };
		auto easing{ easingFunction == 1 ? static_cast<wuc::CompositionEasingFunction>(compositor.CreateCubicBezierEasingFunction({ 0.5f, 0.0f }, { 0.0f, 0.9f })) : static_cast<wuc::CompositionEasingFunction>(compositor.CreateLinearEasingFunction()) };
		animation.InsertKeyFrame(0.0f, 0.0f);
		animation.InsertKeyFrame(1.0f, 1.0f, easing);
		animation.Duration(crossfadeTime);
		return animation;
	}
}