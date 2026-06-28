#include "NextGenHUD.h"

#include <MUASDK.h>
#include <Utils/CPluginConfig.h>

#if !defined(RATL_MAP_VS_INC)
	#include <Common/Ratl/map_vs.h>
#endif

#include <Display/IDisplay.h>

#include <Game/IGame.h>
#include <Game/IMultiplayer.h>
#include <Game/CActor.h>

#include <UI/CHud.h>
#include <UI/CProcRect.h>
#include <UI/CProcIcons.h>
#include <UI/IFontSystem.h>
#include <UI/IMenuSystem.h>

#include <Misc/Common.h>
#include <Misc/CConfig.h>
#include <Misc/tinyXmlBinary.h>

#include <cmath>
#include <vector>

#include <Utils/MemoryMgr.h>
#include <safetyhook/safetyhook.hpp>

float Lerp(float t, float a, float b)
{
	return a + t * (b - a);
}

#define COORD_COUNT (NextGenHUD::HR_COUNT * 4)

namespace Hud
{
	static igVec2f mHUDCoords[COORD_COUNT];
	static float mHUDCoordAspect[NextGenHUD::HR_COUNT];
	static SHUDRect mHUDRects[NextGenHUD::HR_COUNT];

	static SAFETYHOOK_NOINLINE bool __fastcall InitializeProcHUD_NG(CHud* hud, void*, bool looseFile)
	{
		if (hud->mHUDCoordsInitialized)
		{
			return false;
		}

		TiXmlDocument doc;

		if (doc.LoadFile("data/hud_coords_beenox_ng.xmlb", !looseFile, MP_TEMPORARY, false))
		{
			int coordIndex = 0;

			TiXmlIter it(doc, "coords");

			while (it.Valid())
			{
				float invWidth = 1.0F / it.ReadF("width", 256.0F);
				float invHeight = 1.0F / it.ReadF("height", 256.0F);

				TiXmlIter coordIt(it, "coord");

				while (coordIt.Valid())
				{
					if (coordIndex < COORD_COUNT)
					{
						float left = coordIt.ReadF("left", 0.0F);
						float top = coordIt.ReadF("top", 0.0F);
						float right = coordIt.ReadF("right", 0.0F);
						float bottom = coordIt.ReadF("bottom", 0.0F);

						SHUDRect& rect = mHUDRects[coordIndex / 4];
						rect.mLeft = int(left);
						rect.mTop = int(top);
						rect.mRight = int(right);
						rect.mBottom = int(bottom);

						mHUDCoords[coordIndex + 0] = igVec2f(invWidth * left, 1.0F - invHeight * top);
						mHUDCoords[coordIndex + 1] = igVec2f(invWidth * left, 1.0F - invHeight * bottom);
						mHUDCoords[coordIndex + 2] = igVec2f(invWidth * right, 1.0F - invHeight * top);
						mHUDCoords[coordIndex + 3] = igVec2f(invWidth * right, 1.0F - invHeight * bottom);

						float width = std::abs(right - left);
						float height = std::abs(top - bottom);

						mHUDCoordAspect[coordIndex / 4] = height / width;
					}

					coordIndex += 4;
					coordIt++;
				}

				it++;
			}
		}

		hud->mHUDCoordsInitialized = true;
		return true;
	}

	static SAFETYHOOK_NOINLINE const igVec2f* __fastcall GetHUDCoords_NG(CHud* hud, void*, CProcRectItem* item)
	{
		return &mHUDCoords[Clamp<int>(item->GetUserData(), 0, NextGenHUD::HR_COUNT) * 4];
	}

	static SAFETYHOOK_NOINLINE const float __fastcall GetHUDCoordAspect1_NG(CHud* hud, void*, int hudRectIndex)
	{
		return mHUDCoordAspect[hudRectIndex];
	}

	static SAFETYHOOK_NOINLINE const float __fastcall GetHUDCoordAspect2_NG(CHud* hud, void*, CProcRectItem* item)
	{
		return mHUDCoordAspect[Clamp<int>(item->GetUserData(), 0, NextGenHUD::HR_COUNT)];
	}

	//TODO: Check return
	static SAFETYHOOK_NOINLINE SHUDRect __fastcall GetHUDRect_NG(CHud* hud, void*, int hudRectIndex)
	{
		return mHUDRects[hudRectIndex];
	}

	static SAFETYHOOK_NOINLINE void DrawTextFrame_ChangeColors(SafetyHookContext& ctx)
	{
		ETextFrameStyle style = *(ETextFrameStyle*)(ctx.ebp + 0x14);
		igVec4f& backgroundColor = *(igVec4f*)(ctx.esp + 0x88);
		igVec4f& frameColor = *(igVec4f*)(ctx.esp + 0x58);

		if (style == ETFS_CENTER)
		{
			igVec4f& color = TheMenuMgr().StandardColor(CLR_NG_TEXTFRAME);

			backgroundColor = igVec4f(color[0], color[1], color[2], color[3] * 0.5F);
			frameColor = backgroundColor;

			TheMenuMgr().ScaleColor(&frameColor, 2.0F, 2.0F);

			ctx.eip = 0x68F728;
		}
		else if (style == ETFS_LEFT)
		{
			backgroundColor.set(0.38F, 0.67F, 1.0F, 0.25F);
			frameColor.set(0.38F, 0.67F, 1.0F, 1.0F);
			ctx.eip = 0x68F728;
		}
	}

	static SAFETYHOOK_NOINLINE void __fastcall DrawHealthBar_NG(CHud* hud, void*, igVec3f& loc, igVec2f& size, float cScaleX, float health, int a6, bool a7, bool a8, bool a9, bool noTargetType)
	{
		igVec4f fillColor(0.8F, 0.0F, 0.0F, 1.0F);
		igVec4f defaultColor(1.0F, 1.0F, 1.0F, 1.0F);

		if (a8)
		{
			fillColor[0] = 0.5F;
			fillColor[1] = 0.5F;
			fillColor[2] = std::sin(TheGame().GameTime() * 15.0F) * 0.15F + 0.85F;
		}
		else if (a7)
		{
			fillColor[0] = 1.0F;
			fillColor[1] = 0.6F;
			fillColor[2] = 0.6F;
		}
		else if (a9)
		{
			fillColor[0] = fillColor[1] = std::sin(TheGame().GameTime() * 10.0F) * 0.15F + 0.85F;
			fillColor[2] = 0.0F;
		}

		igVec4f backColor(fillColor[0], fillColor[1], fillColor[2], 0.5F);

		float capWidth = size[1] * 0.25F;

		hud->DrawRect(loc, size, backColor, 0.0F, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_FILL, HB_ALPHA, 0);
		hud->DrawRect(loc + igVec3f(-capWidth, 0.0F, 0.0F), igVec2f(capWidth, size[1]), backColor, 0.0F, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONVEX, HB_ALPHA, 0);
		hud->DrawRect(loc + igVec3f(size[0], 0.0F, 0.0F), igVec2f(capWidth, size[1]), backColor, 0.0F, igVec2f(-1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONVEX, HB_ALPHA, 0);

		if (cScaleX >= 1.0F)
		{
			hud->DrawRect(loc + igVec3f(-capWidth, 0.0F, 0.0F), igVec2f(capWidth, size[1]), fillColor, 0.0F, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONVEX, HB_ALPHA, 0);
			hud->DrawRect(loc, size, fillColor, 0.0F, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_FILL, HB_ALPHA, 0);
			hud->DrawRect(loc + igVec3f(size[0], 0.0F, 0.0F), igVec2f(capWidth, size[1]), fillColor, 0.0F, igVec2f(-1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONVEX, HB_ALPHA, 0);
		}
		else
		{
			float barWidth = cScaleX * size[0];
			float fillWidth = barWidth - capWidth * 2.0F;

			if (barWidth > 0.0F)
			{
				float capFilledWidth = Min(capWidth, barWidth);

				hud->DrawRect(loc + igVec3f(-capWidth, 0.0F, 0.0F), igVec2f(capFilledWidth, size[1]), fillColor, 0.0F, igVec2f(capFilledWidth / capWidth, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONVEX, HB_ALPHA, 0);
				barWidth -= capFilledWidth;
			}

			if (fillWidth > 0.0F)
			{
				hud->DrawRect(loc, igVec2f(fillWidth, size[1]), fillColor, 0.0F, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_FILL, HB_ALPHA, 0);
			}

			if (barWidth > 0.0F)
			{
				hud->DrawRect(loc + igVec3f(Max(fillWidth, 0.0F), 0.0F, 0.0F), igVec2f(Min(capWidth, barWidth), size[1]), fillColor, 0.0F, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_EDGE, HB_ALPHA, 0);
			}
		}

		igVec2f endCapSize((size[1] * 1.33F) * 0.33F, size[1] * 1.33F);

		hud->DrawRect(loc + igVec3f(-endCapSize[0], 0.0F, (endCapSize[1] - size[1]) * 0.5F), endCapSize, defaultColor, 0.0F, igVec2f(-1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_ENDCAP, HB_ALPHA, 0);
		hud->DrawRect(loc + igVec3f(size[0], 0.0F, (endCapSize[1] - size[1]) * 0.5F), endCapSize, defaultColor, 0.0F, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_ENDCAP, HB_ALPHA, 0);
	}
};

namespace HudFrames
{
	static SAFETYHOOK_NOINLINE void DrawCharSlotsSingle_DrawArrow(SafetyHookContext& ctx)
	{
		CHudFrames* frames = (CHudFrames*)ctx.edi;

		int index = *(int*)(ctx.esp + 0x228 - 0x208);
		igVec3f& loc = *(igVec3f*)(ctx.esp + 0x228 - 0x1A8);

		igVec3f offs[4] = {
			igVec3f(0.0F, -50.0F, 3.0F),
			igVec3f(3.0F, -50.0F, 0.0F),
			igVec3f(-3.0F, -50.0F, 0.0F),
			igVec3f(0.0F, -50.0F, -3.0F),
		};
		float rots[4] = { 0.0F, 1.5707964F, -1.5707964F, -3.1415927F };

		igVec4f& color = TheMenuMgr().StandardColor(41);//CLR_WHITE

		igVec3f pos = loc + offs[index];

		frames->GetHudOwner()->DrawIcon(pos, 12.0F, color, rots[index], NextGenHUD::HR_NG_ARROW, 1);

		ctx.eip = 0x6A4770;
	}

	static SAFETYHOOK_NOINLINE void DrawCharSlotsMulti_DrawArrow(SafetyHookContext& ctx)
	{
		//esp start 0x19F638
		//esp cur 0x19f410 + 0x228

		CHudFrames* frames = (CHudFrames*)ctx.esi;

		int index = int(ctx.edi);
		igVec3f* offsets = (igVec3f*)(ctx.esp + 0x228 - 0xA8);

		igVec4f& color = TheMenuMgr().StandardColor(41);
		float rots[4] = { 0.0F, 1.5707964F, -1.5707964F, 3.1415927F };

		frames->GetHudOwner()->DrawIcon(offsets[index], 12.0F, color, rots[index], NextGenHUD::HR_NG_ARROW, HB_ALPHA);

		ctx.eip = 0x6A3C83;
	}

	static SAFETYHOOK_NOINLINE void DrawCharSlot_DrawIcons(SafetyHookContext& ctx)
	{
		CHudComponent* component = (CHudComponent*)ctx.esi;
		CActor* actor = *(CActor**)(ctx.esp + 0x64 + 0x8);
		igVec3f& loc = *(*(igVec3f**)(ctx.esp + 0x64 + 0xC));
		float rot = *(float*)(ctx.esp + 0x64 + 0x10);
		igVec4f& color = *(igVec4f*)ctx.eax;

		ctx.edi = uintptr_t(&loc);

		igVec4f scanColor(color[0], color[1], color[1], 0.25F);

		component->GetHudOwner()->DrawIcon(loc, 40.0F, color, 0.0F, NextGenHUD::HR_NG_RING_MED_BACK, HB_ALPHA);

		igVec3f pos(loc[0], loc[1] - 10.0F, loc[2]);
		component->GetHudOwner()->DrawIcon(pos, 41.0F, scanColor, 0.0F, NextGenHUD::HR_NG_RING_MED_SCAN, HB_ALPHA);

		int iconType;
		float rotation;
		float size;
		igVec4f iconColor;

		if (actor->Alive())
		{
			pos[0] = loc[0];
			pos[1] = loc[1] - 10.0F;
			pos[2] = loc[2];
			size = 41.0F;
			iconColor = color;
			rotation = rot;
			iconType = NextGenHUD::HR_NG_RING_MED_BORDER;
		}
		else
		{
			pos[0] = loc[0];
			pos[1] = loc[1] - 10.0F;
			pos[2] = loc[2];

			igVec4f hurtColor(0.5F, 0.0F, 0.0F, 1.0F);

			component->GetHudOwner()->DrawIcon(pos, 41.0F, hurtColor, rot, NextGenHUD::HR_NG_RING_MED_HURT, HB_ALPHA);

			size = 41.0F;
			iconColor = hurtColor;
			rotation = 0.0F;
			iconType = NextGenHUD::HR_NG_RING_MED_SCAN;
		}

		component->GetHudOwner()->DrawIcon(pos, size, iconColor, rotation, iconType, HB_ALPHA);

		ctx.eip = 0x69ED3B;
	}

	static SAFETYHOOK_NOINLINE void DrawActiveCharSlot_DrawIcons(SafetyHookContext& ctx)
	{
		//esp start 0x19F3E8
		//esp cur 0x19F124 (esp+2C4)

		CHudFrames* frames = (CHudFrames*)ctx.esi;
		bool online = *(bool*)(ctx.esp + 0x2C4 + 0x4);
		CActor* actor = (CActor*)(ctx.ebp);
		igVec3f& loc = *(*(igVec3f**)(ctx.esp + 0x2BC + 0x18));
		float rot = *(float*)(ctx.esp + 0x2BC + 0x1C);

		igVec2f& size = *(igVec2f*)(ctx.esp + 0x2C4 - 0x278);

		size.set(26.0F, 33.0F);

		igVec4f& defaultColor = *(igVec4f*)(ctx.esp + 0x2C4 - 0x1D0);

		igVec4f& healthColor = *(igVec4f*)(ctx.esp + 0x2C4 - 0x1E0);
		igVec4f& energyColor = *(igVec4f*)(ctx.esp + 0x2C4 - 0x218);

		igVec4f& healthDamageColor = *(igVec4f*)(ctx.esp + 0x2BC - 0x18C);
		igVec4f& energyDamageColor = *(igVec4f*)(ctx.esp + 0x2BC - 0x1E8);

		igVec4f scanColor(defaultColor[0], defaultColor[1], defaultColor[2], 0.25F);
		igVec4f& playerColor = *(igVec4f*)(ctx.esp + 0x2BC - 0x1AC);

		int& resolution = *(int*)(ctx.esp + 0x2BC - 0x2A8);

		resolution = 16;

		if (!online || actor->IsAIControlled())
		{
			playerColor = defaultColor;
		}

		frames->GetHudOwner()->DrawIcon(loc, 60.8F, defaultColor, 0.0F, NextGenHUD::HR_NG_RING_LRG_BACK, HB_ALPHA);

		igVec3f overlayLoc(loc[0], loc[1] - 10.0F, loc[2]);

		frames->GetHudOwner()->DrawIcon(overlayLoc, 60.8F, scanColor, 0.0F, NextGenHUD::HR_NG_RING_LRG_SCAN, HB_ALPHA);
		frames->GetHudOwner()->DrawIcon(overlayLoc, 60.8F, playerColor, rot, NextGenHUD::HR_NG_RING_LRG_BORDER, HB_ALPHA);
		frames->sub_69FE50(loc, actor, rot - 3.1415927F, 1.0F);

		ctx.ebx = *(uintptr_t*)&rot;
		ctx.edi = uintptr_t(&loc);

		ctx.eip = 0x6A166C;
	}

	static SAFETYHOOK_NOINLINE void DrawActiveCharSlot_SaveSecondBarIcon(SafetyHookContext& ctx)
	{
		int type = *(int*)(ctx.esp + 0x2BC - 0x294);

		ctx.ecx = type;
	}

	static SAFETYHOOK_NOINLINE void DrawActiveCharSlot_DrawRageBar(SafetyHookContext& ctx)
	{
		CHudFrames* frames = (CHudFrames*)ctx.esi;
		CActor* actor = (CActor*)ctx.ebx;
		igVec3f& loc = *(igVec3f*)ctx.edi;

		float rage = actor->GetRage();
		igVec2f size(17.0F, 22.0F);

		for (int i = 0; i < 6; i++)
		{
			int j = i + 1;

			float startStep = i / 6.0F;
			float endStep = j / 6.0F;

			igVec4f color(0.5F, 0.5F, 0.5F, 0.8F);
			igVec2f startStop;

			startStop[0] = 2.3561945F - startStep * 1.5707963F;
			startStop[1] = (2.3561945F - endStep * 1.5707963F) - 0.017453292F;

			if (rage >= endStep)
			{
				if (rage >= 1.0F)
				{
					endStep = std::sin(TheGame().GameTime() * 15.0F) * 0.5F + 0.5F;
				}

				color[0] = 1.0F;
				color[1] = endStep;
				color[2] = 0.0F;
				color[3] = 1.0F;
			}

			igVec3f pos(loc[0], loc[1] - 55.0F, loc[2]);

			frames->GetHudOwner()->DrawArc(pos, size, startStop, 4, color, igVec2f(1.0F, 1.0F), igVec2f(0.0F, 0.0F), 0.0F, NextGenHUD::HR_NG_BAR_RAGE, HB_ALPHA);
		}

		ctx.eip = 0x6A208B;
	}

	static SAFETYHOOK_NOINLINE void DrawXtremeBar_SkipDrawIfNoXtreme(SafetyHookContext& ctx)
	{
		float xtreme = *(float*)(ctx.esp + 0x70 - 0x50);

		if (xtreme > 0.0F)
		{
			return;
		}

		ctx.eip = 0x6A0224;
	}

	static SAFETYHOOK_NOINLINE void DrawXtremeBar_InitFeedbackInfo(SafetyHookContext& ctx)
	{
		CHudFrames* frames = (CHudFrames*)ctx.esi;
		int val = int(ctx.edi);
		float xtreme = *(float*)(ctx.esp + 0x70 - 0x50);

		CHudFrames::CHudCharInfo& info = frames->mCharInfo[val];
		CHudFrames::CHudCharInfo* infoPtr = &info;

		if (info.field_1088 != xtreme)
		{
			info.field_1088 = xtreme;
			info.field_1092 = TheGame().GameTime();
		}
	}

	static SAFETYHOOK_NOINLINE void DrawXtremeBar_SetArcStartStop(SafetyHookContext& ctx)
	{
		float rot = *(float*)(ctx.ebp + 0x10);
		igVec2f& startStop = *(igVec2f*)ctx.eax;

		startStop.set(rot + 2.3736477F, rot - 2.3736477F);
	}

	static SAFETYHOOK_NOINLINE void DrawXtremeBar_SetColor(SafetyHookContext& ctx)
	{
		igVec4f& color = *(igVec4f*)ctx.eax;
		color.set(1.0F, 1.0F, 1.0F, 1.0F);
	}

	static SAFETYHOOK_NOINLINE void DrawXtremeBar_DrawFillFeedback(SafetyHookContext& ctx)
	{
		IHud* hudOwner = (IHud*)ctx.esi;
		CActor* actor = *(CActor**)(ctx.ebp + 0xC);
		igVec3f& loc = *(igVec3f*)(ctx.edi);
		CHudFrames::CHudCharInfo& info = *(CHudFrames::CHudCharInfo*)(*(uintptr_t*)(ctx.esp + 0x70 - 0x3C) + 40);
		igVec2f& size = *(igVec2f*)(ctx.esp + 0x70 - 0x30);
		igVec2f& startStop = *(igVec2f*)(ctx.esp + 0x70 - 0x28);

		float xtremeAmount = actor->sub_440880();

		if (xtremeAmount > 0.0F)
		{
			float time = TheGame().GameTime() - info.field_1092;

			if (time < 1.0F)
			{
				float f = (size[1] - size[0]) * 0.5F + size[0];
				float angle = (startStop[1] - startStop[0]) * xtremeAmount + startStop[0];

				float c = std::cos(angle);
				float s = std::sin(angle);

				igVec3f pos;
				pos.addScaled(loc, f, igVec3f(s, 0.0F, c));

				igVec4f color(1.0F, 1.0F, 1.0F, 1.0F - time);
				float rot = TheGame().GameTime() * 15.0F;

				hudOwner->DrawIcon(pos, 16.0F, color, rot, HR_SPARKLE, HB_ALPHA);
				hudOwner->DrawIcon(pos, 12.0F, color, rot, HR_RADIAL_GRAD, HB_ALPHA);
			}
		}
	}


	static bool g_aiDrawArrow = false;
	static igVec3f g_aiOffset(0.0F, 0.0F, 0.0F);
	static float g_aiRot = 0.0F;

	static SAFETYHOOK_NOINLINE void DrawAIPanel_Reset(SafetyHookContext& ctx)
	{
		igVec3f& loc = *(igVec3f*)(ctx.esp + 0x188 - 0x14C);

		if (TheMultiplayerSystem().NumActivePlayers() > 1)
		{
			igVec3f& offset = *(igVec3f*)(ctx.esp + 0x188 - 0xF4);
			loc.set(offset);
		}

		g_aiDrawArrow = false;
		g_aiOffset.set(0.0F, 0.0F, 0.0F);
		g_aiRot = 0.0F;
	}

	static SAFETYHOOK_NOINLINE void DrawAIPanel_SkipDrawSelectedIcon(SafetyHookContext& ctx)
	{
		bool visible = *(bool*)(ctx.esp + 0x188 - 0x16D);

		if (!visible)
		{
			ctx.eip = 0x6A11BB;
		}
	}

	static SAFETYHOOK_NOINLINE void DrawAIPanel_Init(SafetyHookContext& ctx)
	{
		int offset = *(int*)(ctx.esp + 0x188 - 0x174);
		int index = offset / 4;

		igVec3f offsets[4] = {
			igVec3f(0.0F, 0.0F, -5.0F),
			igVec3f(0.0F, 0.0F, 5.0F),
			igVec3f(5.0F, 0.0F, 0.0F),
			igVec3f(-5.0F, 0.0F, 0.0F)
		};
		float rots[4] = { 3.1415927F, 0.0F, 1.5707964F, -1.5707964F };

		g_aiDrawArrow = true;
		g_aiOffset = offsets[index];
		g_aiRot = rots[index];
	}

	static SAFETYHOOK_NOINLINE void DrawAIPanel_DrawIcons(SafetyHookContext& ctx)
	{
		CHudFrames* frames = (CHudFrames*)ctx.ebx;
		igVec4f& baseColor = *(*(igVec4f**)(ctx.esp + 0x188 + 0xC));

		igVec3f& loc = *(igVec3f*)(ctx.esp + 0x188 - 0x14C);
		int type = int(ctx.ebp);
		float opacity = *(float*)&ctx.esi;

		int offset = *(int*)(ctx.esp + 0x188 - 0x174);
		int index = offset / 4;

		igVec3f* offsets = (igVec3f*)(ctx.esp + 0x188 - 0x40);

		float rots[4] = { 3.1415927F, 0.0F, 1.5707964F, -1.5707964F };

		igVec3f buttonLoc = loc + offsets[index];

		frames->GetHudOwner()->DrawIcon(buttonLoc, 38.0F, baseColor, rots[index], NextGenHUD::HR_NG_RING_MED_BORDER2, HB_ALPHA);
		frames->GetHudOwner()->DrawIcon(buttonLoc, 28.0F, igVec4f(opacity, opacity, opacity, 1.0F), 0.0F, type, HB_ALPHA);
		frames->GetHudOwner()->DrawIcon(buttonLoc, 28.0F, igVec4f(1.0F, 1.0F, 1.0F, 0.25F), 0.0F, NextGenHUD::HR_NG_RING_SML_SCAN, HB_ALPHA);

		ctx.eip = 0x6A11B2;
	}

	static SAFETYHOOK_NOINLINE void DrawAIPanel_SkipDrawSelectedText(SafetyHookContext& ctx)
	{
		bool visible = *(bool*)(ctx.esp + 0x188 - 0x16D);

		if (!visible)
		{
			ctx.eip = 0x6A13E7;
		}
		else
		{
			ctx.eip = 0x6A1269;
		}
	}

	static SAFETYHOOK_NOINLINE void DrawAIPanel_DrawArrow(SafetyHookContext& ctx)
	{
		CHudFrames* frames = (CHudFrames*)ctx.ebx;
		igVec3f& loc = *(igVec3f*)(ctx.esp + 0x188 - 0x14C);

		if (g_aiDrawArrow)
		{
			frames->GetHudOwner()->DrawIcon(loc + g_aiOffset, 12.0F, igVec4f(1.0F, 1.0F, 1.0F, 1.0F), g_aiRot, NextGenHUD::HR_NG_ARROW, HB_ALPHA);
		}

		ctx.eip = 0x6A12BF;
	}
	
	static SAFETYHOOK_NOINLINE void __fastcall DrawCounts_NG(CHudFrames* frames)
	{
		float height = TheDisplay().MenuScreenSafeTop() - TheDisplay().MenuScreenSafeBottom();

		float baseX = TheDisplay().MenuScreenSafeLeft();
		float baseZ = (TheDisplay().MenuScreenSafeBottom() + (height * 0.5F)) - 37.5F;

		int count = CHudFrames::EHC_2;

		if (TheMultiplayerSystem().NumActivePlayers() > 1)
		{
			count = CHudFrames::EHC_SIM_SCORE;
		}

		float offset = 0.0F;

		for (int i = count; i >= 0; i--)
		{
			CHudFrames::CHudCountInfo& info = frames->mCountInfo[i];

			if (info.Visible())
			{
				frames->DrawCount(CHudFrames::EHudCount(i), igVec3f(baseX, 0.0F, baseZ + offset));
			}

			offset += 25.0F;
		}
	}

	static SAFETYHOOK_NOINLINE igVec2f* __fastcall DrawCount_NG(CHudFrames* frames, void*, igVec2f* result, CHudFrames::EHudCount type, const igVec3f& location)
	{
		CHudFrames::CHudCountInfo& info = frames->mCountInfo[type];

		igVec2f barSize(0.0F, 0.0F);
		float time = TheGame().GameTime();

		float goal = 0.0F;
		float animRate = 1000.0F;
		float curCount = info.GetCurrentCount(&animRate, &goal);

		float animDuration = (curCount - info.mStartCount) / animRate;
		float animProgress = Min((time - info.mStartTime) / animDuration, 1.0F);

		igVec4f textColor(info.mColor);
		igVec4f barColor(TheMenuMgr().StandardColor(94));
		igVec3f loc(location);

		float startTime = info.mStartTime;
		float endTime = info.mStartTime + animDuration + 6.0F + 1.5F;

		if (time < endTime)
		{
			if (info.mFirstShowTime <= 0.0F)
			{
				info.mFirstShowTime = startTime;
			}

			float expandProgress = 0.0F;

			info.mAlphaCoef = 1.0F;

			float showTime = time - info.mFirstShowTime;

			if (showTime < 0.25F)
			{
				info.mAlphaCoef = Clamp(showTime * 4.0F, 0.0F, 1.0F);
			}
			else if (showTime < 0.5F)
			{
				expandProgress = Clamp((showTime - 0.25F) * 4.0F, 0.0F, 1.0F);
			}
			else
			{
				float showTimeLeft = endTime - time;

				if (showTimeLeft < 1.0F)
				{
					info.mAlphaCoef = Clamp(showTimeLeft, 0.0F, 1.0F);
				}
			}

			if (showTime >= 0.5F)
			{
				expandProgress = 1.0F;
			}

			info.mEndTime = startTime + animDuration;

			if (startTime < info.mEndTime)
			{
				curCount = Lerp(animProgress, info.mStartCount, curCount);//(curCount - info.mStartCount)* animProgress + info.mStartCount;
			}

			float pulseScale = 1.0F;

			if (time < startTime + 0.25F)
			{
				pulseScale = std::sin((time - startTime) * 4.0F * 3.1415927F) * 0.33F + 1.0F;
			}

			textColor[3] *= info.mAlphaCoef;
			barColor[3] *= info.mAlphaCoef;

			igVec4f baseColor(1.0F, 1.0F, 1.0F, info.mAlphaCoef);
			igVec4f scanColor(1.0F, 1.0F, 1.0F, info.mAlphaCoef * 0.25F);

			ratl::string_vs<256> text;
			info.FormatDisplay(text.c_str(), text.capacity(), curCount);

			float textWidth = TheFontMgr().TextWidth(text.c_str(), 1.0F, info.mFont);
			float textHeight = TheFontMgr().TextHeight(100, "AB01", info.mFont, false, 1.0F);

			info.mMaxWidth = Max(50, Max<int>(info.mMaxWidth, short(textWidth) + 5));

			barSize[1] = textHeight * 1.25F;

			igVec2f baseSize(barSize[1], barSize[1]);
			igVec2f barCapSize(baseSize[0] * 0.25F, baseSize[1]);
			igVec2f endCapSize(baseSize[0] * 1.5F * 0.33F, baseSize[1] * 1.5F);

			barSize[0] = info.mMaxWidth + barCapSize[0] + (baseSize[0] * 2.0F);

			if (expandProgress < 1.0F)
			{
				barSize[0] = Lerp(expandProgress, barCapSize[0] + (baseSize[0] * 2.0F), barSize[0]);//(out[0] - ((mySize[0] / 4.0F) + (mySize[0] * 2.0F))) * expandProgress + ((mySize[0] / 4.0F) + (mySize[0] * 2.0F));
			}

			igVec3f baseLoc(loc[0], loc[1], loc[2] + barSize[1]);
			igVec2f capSize = baseSize * 1.9F;

			float align = -1.0F;

			igVec2f capIconSize = capSize * 0.8F;

			igVec3f sparkleLoc;

			if ((info.mTextFlags & CProcText::TEXT_ALIGN_RIGHT) != 0)
			{
				baseLoc[0] -= barSize[0];
				sparkleLoc.set(capSize[0] * -0.5F, 0.0F, baseSize[1] * 0.75F);
			}
			else
			{
				align = 1.0F;
				sparkleLoc.set(baseSize[0] * 0.75F, 0.0F, baseSize[1] * 0.75F);
			}

			float textLeft = 0.0F;
			float textRight = 0.0F;

			if (align > 0.0F)
			{
				igVec3f capLoc = baseLoc + igVec3f(0.0F, 0.0F, (capSize[1] - barSize[1]) * 0.5F);

				frames->GetHudOwner()->DrawRect(capLoc, capSize, baseColor, 0.0F, igVec2f(align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_RING_SML_BACK, HB_ALPHA, 0);
				frames->GetHudOwner()->DrawRect(capLoc, capSize, baseColor, 0.0F, igVec2f(-align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_RING_BAR_BORDER, HB_ALPHA, 0);
				frames->GetHudOwner()->DrawRect(capLoc + igVec3f((capSize[0] - capIconSize[0]) * 0.5F, 0.0F, (capSize[1] - capIconSize[1]) * -0.5F), capIconSize, baseColor, 0.0F, igVec2f(align, 1.0F), igVec2f(0.0F, 0.0F), info.mCap, HB_ALPHA, 0);
				frames->GetHudOwner()->DrawRect(capLoc, capSize, scanColor, 0.0F, igVec2f(-align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_RING_SML_SCAN, HB_ALPHA, 0);

				igVec3f endCapLoc(0.0F, 0.0F, (endCapSize[1] - baseSize[1]) * 0.5F);

				if (expandProgress > 0.0F)
				{
					igVec3f offset(capSize[0] * 0.75F, 0.0F, 0.0F);

					frames->GetHudOwner()->DrawRect(baseLoc + offset, barCapSize, barColor, 0.0F, igVec2f(-align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONCAVE, HB_ALPHA, 0);

					offset[0] += barCapSize[0];

					igVec2f fillSize((barSize[0] - barCapSize[0] * 1.5F) - capSize[0], baseSize[1]);

					frames->GetHudOwner()->DrawRect(baseLoc + offset, fillSize, barColor, 0.0F, igVec2f(align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_FILL, HB_ALPHA, 0);

					offset[0] += fillSize[0];

					frames->GetHudOwner()->DrawRect(baseLoc + offset, barCapSize, barColor, 0.0F, igVec2f(-align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONVEX, HB_ALPHA, 0);

					endCapLoc[0] = offset[0] + barCapSize[0] * -0.2F;

					if (type == CHudFrames::EHC_TIMER)
					{
						float timerWidth = ((barSize[0] - barCapSize[0]) - capSize[0]) * TheGame().TimerProgress();

						frames->GetHudOwner()->DrawRect(baseLoc + igVec3f(capSize[0] * 0.75F, 0.0F, 0.0F), igVec2f(Min(timerWidth, barCapSize[0]), baseSize[1]), barColor, 0.0F, igVec2f(-align * Min(timerWidth / barCapSize[0], 1.0F), 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_CONCAVE, HB_ALPHA, 0);

						if (timerWidth > barCapSize[0])
						{
							float rem = timerWidth - barCapSize[0];
							float xOff = (capSize[0] * 0.75F) + barCapSize[0];

							if (rem > barCapSize[0])
							{
								frames->GetHudOwner()->DrawRect(baseLoc + igVec3f(xOff, 0.0F, 0.0F), igVec2f(rem, baseSize[1]), barColor, 0.0F, igVec2f(align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_FILL, HB_ALPHA, 0);
								xOff += rem;
							}

							frames->GetHudOwner()->DrawRect(baseLoc + igVec3f(xOff, 0.0F, 0.0F), igVec2f(Min(rem, barCapSize[0]), baseSize[1]), barColor, 0.0F, igVec2f(align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_EDGE, HB_ALPHA, 0);
						}
					}
				}
				else
				{
					endCapLoc[0] = capSize[0] * 0.69F;
				}

				frames->GetHudOwner()->DrawRect(baseLoc + endCapLoc, endCapSize, baseColor, 0.0F, igVec2f(align, 1.0F), igVec2f(0.0F, 0.0F), NextGenHUD::HR_NG_BAR_ENDCAP, HB_ALPHA, 0);

				textLeft = baseLoc[0] + (capSize[0] * 0.9F);
				textRight = textLeft + 512.0F;
			}
			else
			{

			}

			if (type >= CHudFrames::EHC_2)
			{
				frames->GetHudOwner()->DrawIcon(loc + sparkleLoc, baseSize[0] * pulseScale, baseColor, TheGame().GameTime() * 3.1415927F, HR_SPARKLE, HB_ALPHA);
			}

			if (text[0] && expandProgress >= 0.9F)
			{
				frames->GetHudOwner()->TextGen().Print(IFontMgr::EFontType(info.mFont), textLeft, (baseLoc[2] - (baseSize[1] * 0.45F)), textRight - textLeft, baseSize[1], pulseScale, info.mTextFlags | CProcText::TEXT_ALIGN_BOTTOM, textColor, text);
			}
		}

		if (time >= endTime)
		{
			info.mStartTime = 0.0F;
			info.mEndTime = 0.0F;
			info.mFirstShowTime = 0.0F;
		}

		info.mLastCount = curCount;
		*result = barSize;

		return result;
	}
};

namespace HudInputMap
{
	typedef ratl::map_vs<EChain, igVec2f, 4> TLayout;
	typedef ratl::map_vs<EChain, float, 4> TLayoutRot;

	TLayout& staticLayout = *(TLayout*)0xB2F610;
	TLayoutRot staticLayoutRot;

	SafetyHookInline g_hudInputMapInitialize = {};

	static SAFETYHOOK_NOINLINE void __fastcall Initialize_InitLayoutRot(void* inputMap, void*, CHud* owner)
	{
		if (staticLayout.empty())
		{
			staticLayoutRot.insert(CHAIN_ATTACK, 3.1415927F);
			staticLayoutRot.insert(CHAIN_SMASH, 1.5707964F);
			staticLayoutRot.insert(CHAIN_MOVE, 0.0F);
			staticLayoutRot.insert(CHAIN_GUARD, -1.5707964F);
		}

		g_hudInputMapInitialize.thiscall(inputMap, owner);
	}

	CHudInputMap* g_inputMap = NULL;
	igVec3f g_buttonLoc;

	static SAFETYHOOK_NOINLINE void DrawButton_Init(SafetyHookContext& ctx)
	{
		//esp start 0x19F394
		//esp cur 0x19F2AC (+0xE8)

		CHudInputMap* inputMap = (CHudInputMap*)ctx.edi;

		float multiScale = *(float*)(ctx.esp + 0xE8 + 0x10);
		igVec3f& loc = *(igVec3f*)(ctx.esp + 0xE8 - 0xB4);
		float factor = *(float*)(ctx.esp + 0xE8 - 0xB8);

		g_inputMap = inputMap;
		g_buttonLoc.set(loc);

		loc[2] -= multiScale * 0.5F * factor;

		ctx.eip = 0x6A5ACF;
	}

	static SAFETYHOOK_NOINLINE void DrawButton_DrawIcons(SafetyHookContext& ctx)
	{
		if (!g_inputMap) return;

		CHudInputMap* inputMap = g_inputMap;

		int i = *(int*)(ctx.esp + 0xE4 + 0x4);
		//igVec3f& baseLocation = **(igVec3f**)(ctx.esp + 0xE4 + 0x8);
		//float multiScale = *(float*)(ctx.esp + 0xE4 + 0x10);

		igVec4f& defaultColor = *(igVec4f*)(ctx.esp + 0xE4 - 0x60);

		igVec3f& loc = *(igVec3f*)(ctx.esp + 0xE4 - 0xB4);
		//float factor = *(float*)(ctx.esp + 0xE4 - 0xB8);

		igVec2f& iconSize = *(igVec2f*)(ctx.esp + 0xE0 - 0xBC);
		igVec2f& backgroundSize = *(igVec2f*)(ctx.esp + 0xE0 - 0x8C);

		loc.set(g_buttonLoc);

		igVec3f scanLoc = loc + igVec3f(0.0F, -500.0F, 0.0F);
		igVec4f scanColor(1.0F, 1.0F, 1.0F, 0.25F);

		inputMap->GetHudOwner()->DrawIcon(scanLoc, iconSize[0] * 1.5F, scanColor, 0.0F, NextGenHUD::HR_NG_RING_MED_SCAN, HB_ALPHA);

		igVec4f backgroundColor(defaultColor);

		if (TheMultiplayerSystem().NumActivePlayers() > 1)
		{
			backgroundColor = TheMenuMgr().StandardColor(inputMap->mPlayerId + 14);
		}

		TLayoutRot::iterator rotIt;

		if (i == -1)
		{
			rotIt = staticLayoutRot.find(CHAIN_MOVE);
		}
		else
		{
			rotIt = staticLayoutRot.find(EChain(i));
		}

		float backgroundRot = 0.0F;

		if (rotIt != staticLayoutRot.end())
		{
			backgroundRot = rotIt.value();
		}

		int focusedSlot = inputMap->mFocusedSlot;

		bool flag = false;

		if (i < CHAIN_POWER_ATTACK)
		{
			if (focusedSlot > -1 && (i - 1) == focusedSlot)
			{
				flag = true;
			}
		}

		igVec3f backgroundLoc = loc + igVec3f(0.0F, 50.0F, 0.0F);
		inputMap->GetHudOwner()->DrawIcon(backgroundLoc, backgroundSize[0] * 0.825F, backgroundColor, backgroundRot, NextGenHUD::HR_NG_RING_MED_BORDER2, HB_ALPHA);

		if (inputMap->mButtons[i].mSelected || flag)
		{
			float rots[4] = { -3.1415927F, 1.5707964F, -1.5707964F, 0.0F };
			float rot = focusedSlot < 4 ? rots[focusedSlot] : 0.0F;

			inputMap->GetHudOwner()->DrawIcon(loc, backgroundSize[0] * 1.05F, defaultColor, rot, NextGenHUD::HR_NG_RING_SML_BORDER, HB_ALPHA);
		}

		g_inputMap = NULL;
	}

	static SAFETYHOOK_NOINLINE void Draw_DrawArrow(SafetyHookContext& ctx)
	{
		//esp start 0x19F3F8
		//esp cur 0x19F3B0

		CHudInputMap* inputMap = (CHudInputMap*)ctx.esi;
		igVec3f& loc = *(igVec3f*)(ctx.esp + 0x48 - 0x3C);
		igVec4f& color = *(igVec4f*)(ctx.esp + 0x44 - 0xC);

		int focusedSlot = inputMap->mFocusedSlot;

		if (focusedSlot < 4)
		{
			float rots[4] = { -3.1415927F, 1.5707964F, -1.5707964F, 0.0F };
			igVec3f offsets[4] =
			{
				igVec3f(0.0F, 0.0F, -6.0F),
				igVec3f(6.0F, 0.0F, 0.0F),
				igVec3f(-6.0F, 0.0F, 0.0F),
				igVec3f(0.0F, 0.0F, 6.0F),
			};

			inputMap->GetHudOwner()->DrawIcon(loc + offsets[focusedSlot], 14.0F, color, rots[focusedSlot], NextGenHUD::HR_NG_ARROW, HB_ALPHA);
		}

		ctx.eip = 0x6A6009;
	}
};

namespace HudCombo
{
	static SAFETYHOOK_NOINLINE void Draw_DrawTextFrameAndIcons(SafetyHookContext& ctx)
	{
		//esp start 0x19F500
		//esp curr 0x19F634

		CHudCombo* combo = (CHudCombo*)ctx.esi;

		float textWidth = *(float*)(ctx.esp + 0x10);//+ 0x128 - 0x118
		igVec3f& offset = *(igVec3f*)(ctx.esp + 0x24);//+ 0x130 - 0x100 0x19F530 0x19F524

		igVec2f textFrameSize(textWidth * 2.5F, 40.0F);

		combo->GetHudOwner()->DrawTextFrame(combo->mLocation + offset + igVec3f(-textFrameSize[0] * 0.5F, 0.0F, 12.0F), textFrameSize, 1.0F, ETFS_CENTER);
		combo->GetHudOwner()->DrawIcon(combo->mLocation + offset + igVec3f(-textWidth, -100.0F, -8.0F), 46.0F, igVec4f(1.0F, 1.0F, 1.0F, 1.0F), 0.0F, NextGenHUD::HR_NG_RING_MED_BORDER2, HB_ALPHA);
		combo->GetHudOwner()->DrawIcon(combo->mLocation + offset + igVec3f(textWidth, -100.0F, -8.0F), 46.0F, igVec4f(1.0F, 1.0F, 1.0F, 1.0F), 0.0F, NextGenHUD::HR_NG_RING_MED_BORDER2, HB_ALPHA);
	};
};

namespace HudTeamXtreme
{
	static SAFETYHOOK_NOINLINE void Draw_DrawIcon(SafetyHookContext& ctx)
	{
		//esp start 0x19F610
		//esp curr 0x19F458

		igVec3f& loc = *(igVec3f*)(ctx.esp + 0x4C);

		TheHud().DrawIcon(loc + igVec3f(2.0F, 0.0F, 3.0F), 46.0F, igVec4f(1.0F, 1.0F, 1.0F, 1.0F), 0.0F, NextGenHUD::HR_NG_RING_MED_BORDER2, HB_ALPHA);
	}
};

static void Initialize()
{
	using namespace Memory::VP;

	static std::vector<SafetyHookInline> inlineHooks;
	static std::vector<SafetyHookMid> midHooks;

	//Hud
	{
		using namespace Hud;

		//Coords
		WriteMemDisplacement(0x7C5E44, GetHUDCoords_NG);
		WriteMemDisplacement(0x7C5E48, GetHUDCoordAspect1_NG);
		WriteMemDisplacement(0x7C5E4C, GetHUDCoordAspect2_NG);
		WriteMemDisplacement(0x7C5E54, GetHUDRect_NG);

		inlineHooks.push_back(safetyhook::create_inline(0x690570, InitializeProcHUD_NG));

		//Common
		midHooks.push_back(safetyhook::create_mid(0x68F6C7, DrawTextFrame_ChangeColors));
		inlineHooks.push_back(safetyhook::create_inline(0x690950, DrawHealthBar_NG));
	}

	//Hud models
	{
		//Beenox HudLogitech.png related patches
		static const char* hudTexture = "texs/HudLogitech_NG.png";
		static const int hudTextureHeight = 1024;

		Patch(0x6981EF + 1, hudTexture);

		Patch(0x69833D + 1, hudTextureHeight + 3);
		Patch(0x698498 + 1, hudTextureHeight + 6);

		Patch(0x6985A2 + 4, hudTextureHeight - 196);
		Patch(0x6985AA + 4, hudTextureHeight - 164);

		Patch(0x698813 + 4, hudTextureHeight - 160);
		Patch(0x69881B + 4, hudTextureHeight - 0);

		Patch(0x698AB6 + 1, hudTextureHeight);
	}

	//Frames hud
	{
		using namespace HudFrames;
		
		//Char slots single
		{
			Patch(0x6A4415 + 7, 22.0F);//set shift offset

			Patch(0x6A4154 + 1, 35.0F);//set shift offset
			Patch(0x6A4159 + 1, 35.0F);//set shift offset
			
			midHooks.push_back(safetyhook::create_mid(0x6A471E, DrawCharSlotsSingle_DrawArrow));
		}

		//Char slots multi
		{
			Patch(0x6A3019 + 1, 52.0F);//set online hud arrow x offset
			Patch(0x6A348D + 1, 110.0F);//set online hud counts z offset
			
			midHooks.push_back(safetyhook::create_mid(0x6A387B, [](SafetyHookContext& ctx)
			{
				ctx.eip = 0x6A389B;
			}));

			midHooks.push_back(safetyhook::create_mid(0x6A3C18, DrawCharSlotsMulti_DrawArrow));
		}
		
		//Char slot
		{
			Nop(0x69EDA1, 117);//disable shadow
			
			midHooks.push_back(safetyhook::create_mid(0x69EC5F, DrawCharSlot_DrawIcons));
		}

		//Active Char slot
		{
			Patch<unsigned char>(0x6A1788 + 1, NextGenHUD::HR_NG_BAR_HEALTH);//set health bar background icon
			Patch<unsigned char>(0x6A18E7 + 1, NextGenHUD::HR_NG_BAR_HEALTH);//set health bar damage feedback icon
			Patch<unsigned char>(0x6A1943 + 1, NextGenHUD::HR_NG_BAR_HEALTH);//set health bar icon

			Patch<unsigned char>(0x6A1B87 + 1, NextGenHUD::HR_NG_BAR_ENERGY);//set energy bar background icon
			Patch(0x6A1AC1 + 4, NextGenHUD::HR_NG_BAR_ENERGY);//set energy bar background icon
			Patch(0x6A1B78 + 4, NextGenHUD::HR_NG_BAR_WEAPON);//set weapon bar background icon

			Patch<unsigned char>(0x6A1B87, 0x51);//push ecx - use energy/weapon icon for background bar
			Nop(0x6A1B87 + 1, 1);

			Patch<unsigned char>(0x6A1CD4, 0x51);//push ecx - use energy/weapon icon for feedback bar
			Nop(0x6A1CD4 + 1, 1);
			
			midHooks.push_back(safetyhook::create_mid(0x6A159B, DrawActiveCharSlot_DrawIcons));
			midHooks.push_back(safetyhook::create_mid(0x6A1B80, DrawActiveCharSlot_SaveSecondBarIcon));
			midHooks.push_back(safetyhook::create_mid(0x6A1CCF, DrawActiveCharSlot_SaveSecondBarIcon));
			midHooks.push_back(safetyhook::create_mid(0x6A1DF3, DrawActiveCharSlot_DrawRageBar));
		}

		//Xtreme bar
		{
			Patch(0x69FEED + 1, 25.5F);//set xtreme bar height
			Patch(0x69FEF2 + 1, 22.0F);//set xtreme bar width
			Patch<unsigned char>(0x6A009E + 1, NextGenHUD::HR_NG_BAR_XTREME);//set xtreme bar icon type
			Patch(0x6A00E7 + 2, 0x7D4840);//change xtreme bar visibility threshold 0.050000001F -> 0.0F
			
			midHooks.push_back(safetyhook::create_mid(0x69FEB7, DrawXtremeBar_SkipDrawIfNoXtreme));
			midHooks.push_back(safetyhook::create_mid(0x69FEED, DrawXtremeBar_InitFeedbackInfo));
			midHooks.push_back(safetyhook::create_mid(0x69FF2B, DrawXtremeBar_SetArcStartStop));
			midHooks.push_back(safetyhook::create_mid(0x69FF51, DrawXtremeBar_SetColor));
			midHooks.push_back(safetyhook::create_mid(0x6A0203, DrawXtremeBar_DrawFillFeedback));
		}

		//AI panel
		{
			Patch(0x6A0B0B + 1, 35.0F);//panel y offset in multiplayer
			Patch(0x6A0B11 + 1, 100.0F);//panel x offset in multiplayer
			Patch(0x6A0B20 + 4, 1.0F);//no panel scale in multiplayer
			
			midHooks.push_back(safetyhook::create_mid(0x6A0F35, DrawAIPanel_Reset));
			midHooks.push_back(safetyhook::create_mid(0x6A1064, DrawAIPanel_SkipDrawSelectedIcon));
			midHooks.push_back(safetyhook::create_mid(0x6A1080, DrawAIPanel_Init));
			midHooks.push_back(safetyhook::create_mid(0x6A10CA, DrawAIPanel_DrawIcons));
			midHooks.push_back(safetyhook::create_mid(0x6A11D6, DrawAIPanel_SkipDrawSelectedText));
			midHooks.push_back(safetyhook::create_mid(0x6A1269, DrawAIPanel_DrawArrow));
		}

		//Counts
		{
			//HudFrames::SetupCounts patches
			Patch(0x69F024 + 1, NextGenHUD::HR_NG_ICON_ABILITYORB);//set cap type for count types 2-6
			Patch(0x69F02D + 4, NextGenHUD::HR_NG_ICON_TIMER);//set cap type for count type 0
			Patch(0x69F035 + 4, NextGenHUD::HR_NG_ICON_SCORE);//set cap type for count type 1
			Patch<unsigned char>(0x69F0AE + 1, CLR_NORMAL);//set text standard color index for count type 2
			Patch<unsigned char>(0x69F0D4 + 1, CLR_NORMAL);//set text standard color index for count type 3
			Patch<unsigned char>(0x69F0FA + 1, CLR_NORMAL);//set text standard color index for count type 4
			Patch<unsigned char>(0x69F120 + 1, CLR_NORMAL);//set text standard color index for count type 5
			Patch<unsigned char>(0x69F149 + 1, CLR_NORMAL);//set text standard color index for count type 6
			Nop(0x69F18F, 17);//always text align flags to LEFT
			
			inlineHooks.push_back(safetyhook::create_inline(0x6A2550, DrawCounts_NG));
			inlineHooks.push_back(safetyhook::create_inline(0x69F1D0, DrawCount_NG));
		}
	}

	//Input map
	{
		using namespace HudInputMap;
		
		Patch(0x5B3F2A + 1, 1.0F);//set proc icon depth
		Patch(0x6A588B + 1, 32.0F);//set button height
		Patch(0x6A5890 + 1, 32.0F);//set button width
		Patch(0x6A58E5 + 1, 58.0F);//set background height
		Patch(0x6A58EA + 1, 58.0F);//set background width

		g_hudInputMapInitialize = safetyhook::create_inline(0x6A6220, Initialize_InitLayoutRot);

		midHooks.push_back(safetyhook::create_mid(0x6A59DF, DrawButton_Init));
		midHooks.push_back(safetyhook::create_mid(0x6A5D2F, DrawButton_DrawIcons));
		midHooks.push_back(safetyhook::create_mid(0x6A5FB3, Draw_DrawArrow));
	}

	//Target hud
	{
		Nop(0x6A74B3, 14);//HudTarget::SetVisible disable hud head removing
		Nop(0x6A8317, 16);//HudTarget::Draw disable hud head adding
		Nop(0x6A8680, 287);//HudTarget::Draw disable hud & shadow drawing
		
		//Disable text frame
		midHooks.push_back(safetyhook::create_mid(0x6A7ED8, [](SafetyHookContext& ctx)
		{
			ctx.eip = 0x6A7F7C;
		}));
	}

	//Combo hud
	{
		using namespace HudCombo;
		
		midHooks.push_back(safetyhook::create_mid(0x69D943, Draw_DrawTextFrameAndIcons));
	}

	//Xtreme hud
	{
		using namespace HudTeamXtreme;
		
		midHooks.push_back(safetyhook::create_mid(0x6A8F69, Draw_DrawIcon));
	}
}

NextGenHUD::NextGenHUD()
{	
	MUASDK::OnClientPreInitEvent() += []()
	{
		bool enabled = false;
		PluginIni().Get("MAIN", "NextGenHUD", &enabled);

		if (enabled)
		{
			Initialize();
		}
	};
}

NextGenHUD plugin;
