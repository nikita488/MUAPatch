#include "ConsoleGamma.h"

#include <MUASDK.h>
#include <safetyhook/safetyhook.hpp>

#include <Display/IAlchemyDisplay.h>
#include <Scene/IShaderCache.h>
#include <Scene/CScene.h>

#include <igGapAttrs.h>
#include <igAttrs/igVertexShaderBindAttr.h>
#include <igAttrs/igPixelShaderBindAttr.h>

#include <igGfx/igDx9VisualContext.h>

#include <Utils/CPluginConfig.h>

using namespace Gap;

static bool mEnabled = false;
static igVertexShaderBindAttrRef mGammaVSBind;
static igPixelShaderBindAttrRef mGammaPSBind;

bool& g_advLightingEnabled = *(bool*)0xD40284;

static void ReloadGammaShaderHook(SafetyHookContext& ctx)
{
	if (mEnabled && g_advLightingEnabled)
	{
		TheShaderCache().CacheShader("gamma.igb", NULL, MP_PERMANENT);

		LoadShader("gamma", mGammaVSBind, mGammaPSBind, NULL);
	}
}

static void ApplyGammaShaderHook(SafetyHookContext& ctx)
{
	CScene* scene = (CScene*)ctx.ebp;

	if (!mEnabled || !g_advLightingEnabled || !mGammaVSBind || !mGammaPSBind) return;

	TheAlchemyDisplay().func_50(IAlchemyDisplay::RD_1);

	if (TheAlchemyDisplay().SetActiveRenderDestination(IAlchemyDisplay::RD_0))
	{
		Gfx::igDx9VisualContext* vc = Gfx::igDx9VisualContext::dynamicCast(TheAlchemyDisplay().GetVC());
		igVertexStream* stream = scene->field_240356->getVertexStream();

		igComponentEditInfo2 editInfo;

		editInfo._componentType = igVertexData::IG_VERTEX_COMPONENT_TEXCOORD;
		editInfo._startIndex = 0;
		editInfo._numComponents = 4;
		editInfo._componentIndex = 0;

		int width = 0, height = 0;
		int id = TheAlchemyDisplay().GetRenderDestination(IAlchemyDisplay::RD_0);
		vc->getRenderDestinationSize(id, &width, &height);

		bool lastAlphaTestState = vc->getAlphaTestState();
		bool lastDepthTestState = vc->getDepthTestState();
		bool lastDepthWriteState = vc->getDepthWriteState();

		vc->setAlphaTestState(false);
		vc->setDepthTestState(false);
		vc->setDepthWriteState(false);
		vc->setVertexArray(scene->field_240356, 0);

		TheAlchemyDisplay().ClearActiveRenderDestination_Depth();

		float offsetX = 1.0F / width;
		float offsetY = 1.0F / height;

		vc->setTextureStageState(0, true);

		int texture0 = TheAlchemyDisplay().GetRenderDestinationTexture(IAlchemyDisplay::RD_1);
		vc->setTexture(texture0, 0, IG_GFX_TEXTURE_WRAP_CLAMP, IG_GFX_TEXTURE_WRAP_CLAMP, IG_GFX_TEXTURE_FILTER_NEAREST, IG_GFX_TEXTURE_FILTER_NEAREST);

		mGammaVSBind->apply(vc);
		mGammaPSBind->apply(vc);

		stream->getEditableComponent(editInfo);

		editInfo.getTextureCoord(0) = igVec2f(offsetX + 0.0F, offsetY + 1.0F);
		editInfo.getTextureCoord(1) = igVec2f(offsetX + 1.0F, offsetY + 1.0F);
		editInfo.getTextureCoord(2) = igVec2f(offsetX + 0.0F, offsetY + 0.0F);
		editInfo.getTextureCoord(3) = igVec2f(offsetX + 1.0F, offsetY + 0.0F);

		stream->commitComponentEdits(editInfo);

		vc->draw(IG_GFX_DRAW_TRIANGLE_STRIP, 2, 0);

		vc->setAlphaTestState(lastAlphaTestState);
		vc->setDepthTestState(lastDepthTestState);
		vc->setDepthWriteState(lastDepthWriteState);

		vc->setTextureStageState(0, false);
	}
}

static void Initialize()
{
	static bool initialized = false;

	if (initialized)
		return;

	PluginIni().Get("MAIN", "ConsoleGamma", &mEnabled, false);

	if (mEnabled && g_advLightingEnabled)
	{
		if (Gfx::igDxVisualContext::dynamicCast(TheAlchemyDisplay().GetVC()))
		{
			TheShaderCache().CacheShader("gamma.igb", NULL, MP_PERMANENT);
		}

		LoadShader("gamma", mGammaVSBind, mGammaPSBind, NULL);
	}

	initialized = true;
}

ConsoleGamma::ConsoleGamma()
{
	using namespace MUASDK;

	static SafetyHookMid applyGammaShaderHook = safetyhook::create_mid(0x66A9FF, ApplyGammaShaderHook);
	static SafetyHookMid reloadGammaShaderHook = safetyhook::create_mid(0x66A5C9, ReloadGammaShaderHook);
	
	OnSceneInitEvent() += []()
	{
		Initialize();
	};
	
	OnClientShutdownEvent() += []()
	{
		mGammaVSBind = NULL;
		mGammaPSBind = NULL;
	};
}

ConsoleGamma plugin;
