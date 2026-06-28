fxc /nologo /T vs_3_0 /E mainVS /Zpr /Gfa /Fo gamma.vs ..\\gamma.hlsl
fxc /nologo /T ps_3_0 /E mainPS /Zpr /Gfa /Fo gamma.ps ..\\gamma.hlsl

D:\\Dev\\Raven\\shaders2igb\\bin\\Release\\shaders2igb . gamma.igb
Pause