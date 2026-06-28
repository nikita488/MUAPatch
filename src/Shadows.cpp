#include "Shadows.h"

#include <MUASDK.h>

#include <Utils/MemoryMgr.h>
#include <Misc/CConfig.h>

#include <Utils/CPluginConfig.h>

//#define depthWidth (512)
//#define depthHeight (512)
//#define defWidth (512)
//#define defHeight (512)

static const char* SHADER_NAME = "occlusionX360";
static const char* SHADER_FILE_NAME = "occlusionX360.igb";

static DWORD g_shadomMapHemiSize = 0;
static DWORD g_shadomMapWidth = 0;
static DWORD g_shadomMapHeight = 0;

__declspec(naked) void ShadowMapTextureHook()
{
	static const DWORD continueAddr = 0x721A16;

	_asm
	{
		push g_shadomMapHeight
		push g_shadomMapWidth
		jmp continueAddr
	}
}

__declspec(naked) void ShadowMapHorizontalBlurTextureHook()
{
	static const DWORD continueAddr = 0x721A9A;

	_asm
	{
		push g_shadomMapHeight
		push g_shadomMapWidth
		jmp continueAddr
	}
}

//__declspec(naked) void ShadowMapResolutionHook3()
//{
//	static const DWORD epilogue = 0x721A46;
//
//	_asm
//	{
//		mov dword ptr[ebp + 0xC8], 1024
//		mov dword ptr[ebp + 0xCC], 2048
//		jmp epilogue
//	}
//}

__declspec(naked) void ShadowMapRenderDestHook()
{
	static const DWORD continueAddr = 0x721A46;

	_asm
	{
		push eax
		mov eax, g_shadomMapWidth
		mov dword ptr[ebp + 0xC8], eax
		mov eax, g_shadomMapHeight
		mov dword ptr[ebp + 0xCC], eax
		pop eax
		jmp continueAddr
	}
}

//__declspec(naked) void ShadowMapResolutionHook4()
//{
//	static const DWORD epilogue = 0x721AC4;
//
//	_asm
//	{
//		mov dword ptr[ebp + 0x158], shadowMapWidth
//		mov dword ptr[ebp + 0x15C], shadowMapHeight
//		jmp epilogue
//	}
//}

__declspec(naked) void ShadowMapHorizontalBlurRenderDestHook()
{
	static const DWORD continueAddr = 0x721AC4;

	_asm
	{
		push eax
		mov eax, g_shadomMapWidth
		mov dword ptr[ebp + 0x158], eax
		mov eax, g_shadomMapHeight
		mov dword ptr[ebp + 0x15C], eax
		pop eax
		jmp continueAddr
	}
}

//__declspec(naked) void ShadowMapResolutionHook5()
//{
//	static const DWORD epilogue = 0x721B14;
//
//	_asm
//	{
//		push 1
//		push 3
//		push 0x26
//		push depthHeight
//		push depthWidth
//		jmp epilogue
//	}
//}
//
//__declspec(naked) void ShadowMapResolutionHook6()
//{
//	static const DWORD epilogue = 0x721B3A;
//
//	_asm
//	{
//		mov dword ptr[ebp + 0xF8], depthWidth
//		mov dword ptr[ebp + 0xFC], depthHeight
//		jmp epilogue
//	}
//}
//
//__declspec(naked) void ShadowMapResolutionHook7()
//{
//	static const DWORD epilogue = 0x721B8E;
//
//	_asm
//	{
//		push defHeight
//		lea eax, [ebp + 0x1BC]
//		push defWidth
//		jmp epilogue
//	}
//}
//
//__declspec(naked) void ShadowMapResolutionHook8()
//{
//	static const DWORD epilogue = 0x721BB8;
//
//	_asm
//	{
//		mov dword ptr[ebp + 0x128], defWidth
//		mov dword ptr[ebp + 0x12C], defHeight
//		jmp epilogue
//	}
//}

static void Initialize()
{
	using namespace Memory::VP;

	int shadowType = 0;
	PluginIni().Get("MAIN", "ShadowsType", &shadowType, 0);

	if (shadowType == 1 || shadowType == 2)
	{
		InjectHook(0x721A11, ShadowMapTextureHook, HookType::Jump);
		Nop(0x721A1B, 1);

		InjectHook(0x721A94, ShadowMapHorizontalBlurTextureHook, HookType::Jump);
		Nop(0x721A99, 1);

		InjectHook(0x721A36, ShadowMapRenderDestHook, HookType::Jump);
		Nop(0x721A36 + 5, 11);

		InjectHook(0x721AB4, ShadowMapHorizontalBlurRenderDestHook, HookType::Jump);
		Nop(0x721AB4 + 5, 11);

		//Calculate shader map constats

		if (shadowType == 1)
		{
			g_shadomMapHemiSize = 512;
		}
		else if (shadowType == 2)
		{
			g_shadomMapHemiSize = 1024;
		}

		g_shadomMapWidth = g_shadomMapHemiSize * 2;
		g_shadomMapHeight = g_shadomMapHemiSize * 4;

		//Set hemi sphere size
		Patch<unsigned char>(0x67B40D + 2, unsigned char(log2(g_shadomMapHemiSize)));
		Patch(0x67B554 + 3, g_shadomMapHemiSize);
		Patch(0x664B80 + 1, g_shadomMapHemiSize);
		Patch(0x664B85 + 1, g_shadomMapHemiSize);

		//Set shadow map texel size to texture0.texelsize shader constant
		Patch(0x67AABD + 1, 1.0F / g_shadomMapWidth);
		Patch(0x67AAB8 + 1, 1.0F / g_shadomMapHeight);

		Nop(0x67B8DF, 25);//Disable shadow map blur

		{
			//Set new shader for occlusion

			Patch(0x666526 + 1, SHADER_NAME);
			Patch(0x66653D + 1, SHADER_NAME);
			Patch(0x666554 + 1, SHADER_NAME);
			Patch(0x66656B + 1, SHADER_NAME);
			Patch(0x666585 + 1, SHADER_NAME);

			Patch(0x6673AB + 1, SHADER_NAME);
			Patch(0x6673C8 + 1, SHADER_NAME);
			Patch(0x6673E8 + 1, SHADER_NAME);
			Patch(0x667405 + 1, SHADER_NAME);
			Patch(0x667422 + 1, SHADER_NAME);

			Patch(0x669D80 + 1, SHADER_FILE_NAME);
			Patch(0x66A50C + 1, SHADER_FILE_NAME);
		}

		///////Nop(0x721B0C, 8);
		///////InjectHook(0x721B0C, ShadowMapResolutionHook5, HookType::Jump);
		///////
		///////Nop(0x721B2E, 12);
		///////InjectHook(0x721B2E, ShadowMapResolutionHook6, HookType::Jump);
		///////
		///////Nop(0x721B86, 8);
		///////InjectHook(0x721B86, ShadowMapResolutionHook7, HookType::Jump);
		///////
		///////Nop(0x721BAC, 12);
		///////InjectHook(0x721BAC, ShadowMapResolutionHook8, HookType::Jump);
		///////
		///////Patch(0x67B923 + 1, depthWidth);
		///////Patch(0x67B928 + 1, depthHeight);

		//float w = 1.0F / 512.0F;//0.001953125
		//float h = 1.0F / 1024.0F;//0.0009765625
		//float hz = 1.0F / 2048.0F;//0.00048828125
		//float w = 1.0F / shadowMapWidth;//0.001953125
		//float h = 1.0F / shadowMapHeight;//0.0009765625
		//float hz = 1.0F / (shadowMapHeight * 2.0F);//0.00048828125

		//InjectHook(0x67B8F3, Test);
		

		//Patch(0x67845C + 1, w);
		//Patch(0x6784A5 + 1, 1.0F + hz);
		//Patch(0x6784D2 + 1, 1.0F + hz);
		//Patch(0x6784D7 + 1, 1.0F + h);
		//Patch(0x678535 + 1, 1.0F + h);
		//Patch(0x6785C4 + 1, h);
		//
		//Patch(0x7C47E8, h);
		//Patch(0x7C48A4, hz);
		//
		// //sub_677820
		//Patch(0x6778B7 + 1, 1.0F + h);
		//Patch(0x6778D9 + 1, 1.0F + h);
		//Patch(0x6778DE + 1, 1.0F + h);
		//Patch(0x6778FC + 1, 1.0F + h);
	}
}

Shadows::Shadows()
{
	using namespace MUASDK;

	OnClientPreInitEvent() += []()
	{
		Initialize();
	};
}

Shadows plugin;
