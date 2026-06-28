#include "Features.h"

#include <MUASDK.h>
#include <Utils/CPluginConfig.h>

#include <UI/CMenuCodex.h>
#include <UI/CMenuDebug.h>

#include <Utils/MemoryMgr.h>

void PatchOutfitList()
{
	using namespace Memory::VP;
	
	const int outfitCount = 6;

	Patch<unsigned char>(0x6CD154 + 1, outfitCount);//CMenuItemOutfits::IsOutfitExpanded
	Patch<unsigned char>(0x6CD194 + 1, outfitCount);//CMenuItemOutfits::SetOutfitExpanded
	Patch<unsigned char>(0x6CD1C4 + 1, outfitCount);//CMenuItemOutfits::SetOutfitCollapsed
}

Features::Features()
{
	using namespace MUASDK;
	
	OnClientPreInitEvent() += []()
	{
		bool codexMenu = false;
		PluginIni().Get("MAIN", "CodexMenu", &codexMenu, false);
			
		if (codexMenu)
		{
			//RegisterMenu<CMenuCodex>("CODEX_MENU");
		}

		bool fixPassives = false;
		PluginIni().Get("MAIN", "FixMissingOutfitPassives", &fixPassives, false);

		if (fixPassives)
		{
			PatchOutfitList();
		}
	};
}

Features plugin;
