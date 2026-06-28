#include "SkipIntros.h"

#include <safetyhook/safetyhook.hpp>

#include <Utils/CPluginConfig.h>

#include <Misc/CConfig.h>
#include <UI/IMenuSystem.h>

//#include <MUASDK.h>
//#include <Utils/MemoryMgr.h>

//#include <Misc/Common.h>
//#include <Misc/IClock.h>
//#include <Misc/ICommand.h>
//#include <Game/ISaveLoad.h>
//#include <Game/ISimulator.h>
//#include <Display/IDisplay.h>
//#include <Scene/IScene.h>
//#include <UI/IDialog.h>

//#include <Game/IMultiplayer.h>
//#include <Game/IOptions.h>
//#include <Network/CNetMan.h>
//#include <Network/CNetPlay.h>
//#include <Network/CNetLobby.h>
//#include <Input/IInput.h>

//#include <Client/CClient.h>

//char* dword_D3F74C = (char*)0xD3F74C;
//bool& byte_D3F774 = *(bool*)0xD3F774;
//bool& byte_D3F776 = *(bool*)0xD3F776;
//bool& byte_D3F798 = *(bool*)0xD3F798;
//bool& byte_D3F799 = *(bool*)0xD3F799;
//
//bool sub_401000()
//{
//    return CallAndReturn<bool, 0x401000>();
//}
//
//static void __fastcall UpdateGameBeginHook(CClient* client)
//{
//    bool skipLegalScreen = false;
//    PluginIni().Get("MAIN", "skipLegalScreen", &skipLegalScreen);
//
//    bool skipWarningScreen = false;
//    PluginIni().Get("MAIN", "skipESRBWarningScreen", &skipWarningScreen);
//
//    bool skipIntro = false;
//    PluginIni().Get("MAIN", "skipIntro", &skipIntro);
//
//    if (skipLegalScreen || client->IsLegalScreenFinished())
//    {
//        if (!client->field_40_8)
//        {
//            TheMenuMgr().ShutdownAllMenus(false);
//            TheMenuMgr().SetDrawTime(0.0F);
//            TheScene().Render(true);
//            client->field_40_8 = true;
//
//            if (!TheDisplay().IsNotNorthAmerica())
//            {
//                byte_D3F798 = true;
//                TheMenuMgr().OpenMenu("ESRB_warning", NULL);
//                client->sub_418970();
//            }
//        }
//
//        if (skipWarningScreen || client->IsWarningScreenFinished())
//        {
//            if (!client->field_48_2)
//            {
//                TheMenuMgr().ShutdownAllMenus(false);
//                TheMenuMgr().SetDrawTime(0.0F);
//                TheScene().Render(true);
//                client->field_48_2 = true;
//            }
//
//            if (!client->field_40_4)
//            {
//                Command().ExecuteCommandString("savedefaultoptions");
//                TheSaveLoad().BeginProcess(ISaveLoad::SL_LOAD_OPTIONS);
//                client->field_40_4 = true;
//            }
//
//            if (!client->mGameHasBegun)
//            {
//                if (TheSaveLoad().GetProcess() == ISaveLoad::SL_IDLE)
//                {
//                    if (!TheDialog().IsVisible())
//                    {
//                        bool launchMap = false;
//                        XMenIni().Get("INIT", "launchMap", &launchMap);
//
//                        Command().ExecuteCommandString("resetgame");
//
//                        bool launchSimulator = false;
//                        XMenIni().Get("SIMULATOR", "launchSimulator", &launchSimulator);
//
//                        ratl::string_vs<64> pauseMenu("pda");
//                        XMenIni().Get("INIT", "pauseMenu", pauseMenu.c_str(), pauseMenu.capacity(), "pda");
//
//                        TheMenuMgr().SetPauseMenu(pauseMenu.c_str());
//
//                        ratl::string_vs<64> teamMenu("team");
//                        XMenIni().Get("INIT", "teamMenu", teamMenu.c_str(), teamMenu.capacity(), "team");
//
//                        TheMenuMgr().SetTeamMenu(teamMenu.c_str());
//
//                        if (launchMap)
//                        {
//                            EPlayerId playerId = TheInput().GetNthConnectedPlayer(1);
//
//                            if (playerId == EPLAYERID_NONE)
//                            {
//                                playerId = EPLAYERID_ONE;
//                            }
//
//                            TheMultiplayerSystem().SetFirstPrimaryPlayerId(playerId);
//
//                            TheMenuMgr().SetIsReadyForUse();//Bugfix for pause menu not opening
//
//                            ratl::string_vs<64> mapName;
//
//                            Command().ExecuteCommandString("resetgame");
//
//                            ratl::string_vs<100> command;
//
//                            XMenIni().Get("MAP", "filename", mapName.c_str(), mapName.capacity());
//
//                            ratl::string_vs<64> fixedPath(Filename_EnsureForwardSlashes(mapName.c_str()));
//
//                            ratl::str::printf(command.c_str(), "loadmapaddteam %s", fixedPath.c_str());
//                            Command().ExecuteCommandString(command.c_str());
//                        }
//                        else if (launchSimulator)
//                        {
//                            EPlayerId playerId = TheInput().GetNthConnectedPlayer(1);
//
//                            if (playerId == EPLAYERID_NONE)
//                            {
//                                playerId = EPLAYERID_ONE;
//                            }
//
//                            TheMultiplayerSystem().SetFirstPrimaryPlayerId(playerId);
//
//                            TheMenuMgr().SetIsReadyForUse();//Bugfix for pause menu not opening
//
//                            Command().ExecuteCommandString("resetgame");
//
//                            ratl::string_vs<100> command;
//
//                            int courseIndex;
//                            XMenIni().Get("SIMULATOR", "courseIndex", &courseIndex);
//
//                            if (courseIndex >= 0)
//                            {
//                                TheSimulator().func_60(courseIndex, false, false);
//                            }
//                        }
//                        else
//                        {
//                            ratl::string_vs<64> startMenu(TheMenuMgr().GetStartMenu());
//
//                            XMenIni().Get("INIT", "startMenu", startMenu.c_str(), startMenu.capacity(), TheMenuMgr().GetStartMenu());
//
//                            if (stricmp(startMenu.c_str(), TheMenuMgr().GetStartMenu()) == 0)
//                            {
//                                if (sub_401000())//FirstRunCompleted == 0 -> true
//                                {
//                                    byte_D3F799 = true;
//                                }
//
//                                if (byte_D3F776)
//                                {
//                                    TheMenuMgr().SetMainMenuLastClickedPlayer(EPLAYERID_ONE);
//                                    TheMultiplayerSystem().SetFirstPrimaryPlayerId(EPLAYERID_ONE);
//
//                                    TheNetwork().Setup();
//                                    TheNetPlay().Setup();
//                                    TheNetLobby().Setup();
//
//                                    TheNetwork().SetOnlineMenuState(NOS_ONLINE);
//
//                                    TheOptions().SetPlayerName(dword_D3F74C);
//                                    TheNetPlay().SetLocalPlayerName(TheOptions().GetPlayerName());
//                                    byte_D3F798 = false;
//
//                                    if (byte_D3F774)
//                                    {
//                                        Command().ExecuteCommandString("openmenu game_options");
//                                    }
//                                    else
//                                    {
//                                        Command().ExecuteCommandString("openmenu games_list");
//                                    }
//                                }
//                                else
//                                {
//                                    if (skipIntro)
//                                    {
//                                        TheMenuMgr().OpenMenu(startMenu.c_str(), NULL);
//                                    }
//                                    else
//                                    {
//                                        Command().ExecuteCommandString("runscript menus/intro_normal");
//                                    }
//                                }
//                            }
//                            else
//                            {
//                                EPlayerId playerId = TheInput().GetNthConnectedPlayer(1);
//
//                                if (playerId == EPLAYERID_NONE)
//                                {
//                                    playerId = EPLAYERID_ONE;
//                                }
//
//                                TheMultiplayerSystem().SetFirstPrimaryPlayerId(playerId);
//                                TheMultiplayerSystem().SetRealPrimaryController(EController(playerId));
//
//                                TheMenuMgr().OpenMenu(startMenu.c_str(), NULL);
//                            }
//                        }
//
//                        client->mGameHasBegun = true;
//                    }
//                }
//            }
//        }
//    }
//}

void SkipLegalScreenHook(SafetyHookContext& ctx)
{
    bool skipLegalScreen = false;
    PluginIni().Get("MAIN", "SkipLegalScreen", &skipLegalScreen);

    if (skipLegalScreen)
    {
        ctx.eip = 0x4186C1;
    }
}

void SkipESRBWarningScreenHook(SafetyHookContext& ctx)
{
    bool skipWarningScreen = false;
    PluginIni().Get("MAIN", "SkipESRBWarningScreen", &skipWarningScreen);

    if (skipWarningScreen)
    {
        ctx.eip = 0x41874D;
    }
}

void SkipIntroHook(SafetyHookContext& ctx)
{
    bool skipIntro = false;
    PluginIni().Get("MAIN", "SkipIntro", &skipIntro);

    if (skipIntro)
    {
        ratl::string_vs<64> startMenu(TheMenuMgr().GetStartMenu());
        XMenIni().Get("INIT", "startMenu", startMenu.c_str(), startMenu.capacity(), TheMenuMgr().GetStartMenu());
        
        TheMenuMgr().OpenMenu(startMenu.c_str(), NULL);
        ctx.eip = 0x418950;
    }
}

SkipIntros::SkipIntros()
{
	//static SafetyHookInline updateGameBeginHook = safetyhook::create_inline(0x418690, UpdateGameBeginHook);
	static SafetyHookMid skipLegalScreenHook = safetyhook::create_mid(0x41869F, SkipLegalScreenHook);
	static SafetyHookMid skipESRBWarningScreenHook = safetyhook::create_mid(0x41872B, SkipESRBWarningScreenHook);
	static SafetyHookMid skipIntroHook = safetyhook::create_mid(0x418893, SkipIntroHook);
}

SkipIntros plugin;
