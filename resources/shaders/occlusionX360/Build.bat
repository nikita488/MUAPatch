fxc /nologo /T vs_3_0 /E mainVS /Zpr /Gfa /D NUM_LIGHTS=0 /Fo occlusion.vs ..\\occlusionX360.hlsl

fxc /nologo /T ps_3_0 /E mainPS /Zpr /Gfa /D NUM_LIGHTS=1 /Fo occlusion_P1.ps ..\\occlusionX360.hlsl
fxc /nologo /T ps_3_0 /E mainPS /Zpr /Gfa /D NUM_LIGHTS=2 /Fo occlusion_P2.ps ..\\occlusionX360.hlsl
fxc /nologo /T ps_3_0 /E mainPS /Zpr /Gfa /D NUM_LIGHTS=3 /Fo occlusion_P3.ps ..\\occlusionX360.hlsl
fxc /nologo /T ps_3_0 /E mainPS /Zpr /Gfa /D NUM_LIGHTS=4 /Fo occlusion_P4.ps ..\\occlusionX360.hlsl

D:\\Dev\\Raven\\shaders2igb\\bin\\Release\\shaders2igb . occlusionX360.igb

Pause