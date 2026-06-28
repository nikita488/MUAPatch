//g_boundsLight0.x = 1.0 / mLightFarPlane 
//g_boundsLight0.y = 0.01
//g_boundsLight0.z = lightIndex (0..3) * 0.25 + 0.125
//g_boundsLight0.w = 1.0

#if !defined(NUM_LIGHTS)
#   error "NUM_LIGHTS must be defined as 1, 2, 3, or 4"
#endif

//bool g_lightEnable0 : register(b2);
//bool g_lightEnable1 : register(b2);
//bool g_lightEnable2 : register(b2);
//bool g_lightEnable3 : register(b2);

float4x4 g_matInvView : register(c0);

float4 g_boundsLight0 : register(c178);
float4 g_boundsLight1 : register(c179);
float4 g_boundsLight2 : register(c180);
//float4 g_boundsLight3 : register(c181);

float4 g_lightPos0 : register(c183);
float4 g_lightPos1 : register(c184);
float4 g_lightPos2 : register(c185);
//float4 g_lightPos3 : register(c186);

//float4 g_viewOrigin : register(c186);
//float4 g_lowAttenutation : register(c186);
float4 g_texelSize0;

float2 g_WorldViewScale : register(c203);

bool g_noShadowLinearFiltering : register(b2);

sampler2D g_ShadowMap : register(s6);
sampler2D g_depthMap : register(s0);

struct VS_IN
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 Position : POSITION;
    float4 TCVV : TEXCOORD0;
};

VS_OUT mainVS(VS_IN IN)
{
    VS_OUT OUT;
    OUT.Position = float4(IN.Position.xyz, 1.0);
    OUT.TCVV.xy = IN.TexCoord.xy;
    OUT.TCVV.zw = g_WorldViewScale.xy * IN.Position.xy;
    return OUT;
}

struct PS_IN
{
    float4 TCVV : TEXCOORD0;
};

struct PS_OUT
{
    float4 Color : Color;
};

float4 SampleShadowMap(float2 uv, float2 offset)
{
    return tex2Dlod(g_ShadowMap, float4(uv + g_texelSize0.xy * offset, 0.0, 0.0));
}

float SampleChebyshev(float2 shadowUV, float referenceDepth)
{			
	if (g_noShadowLinearFiltering)
	{
        return (SampleShadowMap(shadowUV, float2(0, 0)).r >= referenceDepth);
    }
	else
	{	
        float sum = 0.0;
		
        for (float y = -1.5; y <= 1.5; y += 1.0)
        {
            for (float x = -1.5; x <= 1.5; x += 1.0)
            {
                sum += (SampleShadowMap(shadowUV, float2(x, y)).r >= referenceDepth);
            }
        }

        return sum / 16.0;
    }
}

PS_OUT mainPS(PS_IN IN)
{
    PS_OUT OUT;
    OUT.Color = 1.0;

    float depth = tex2D(g_depthMap, IN.TCVV.xy).x;
    float3 posXYZ = depth * float3(IN.TCVV.zw, 1.0);

    float4 wPos = mul(g_matInvView, float4(posXYZ, 1.0));

#if NUM_LIGHTS == 1
	float3 off = wPos.xyz - g_lightPos0.xyz;
	float _signZ = sign(off.z);
	
	off.z += g_boundsLight0.w * _signZ;
	
	float _dist = length(off);
	float _receiverDepth = _dist * g_boundsLight0.x - g_boundsLight0.y;
	
	float _pDen = _signZ * _dist + off.z;
	float _scZ = _signZ / _pDen;
	
	float _sX = off.x * _scZ;
	float _sY = off.y * _scZ;
	
	float pV = g_boundsLight0.z - _sY * 0.125;
	float pU = (_sX + 2.0 - _signZ) * 0.25;
#endif

#if NUM_LIGHTS == 2
	float2 offX = wPos.x - g_lightPos0.xy;
	float2 offY = wPos.y - g_lightPos1.xy;
	float2 offZ = wPos.z - g_lightPos2.xy;
	
	float2 _signZ = sign(offZ);
	
	float2 _dist = sqrt(offX * offX + offY * offY + offZ * offZ);
	float2 _receiverDepth = _dist * g_boundsLight0.xy - g_boundsLight1.xy;
	
	float2 _pDen = _signZ * _dist + offZ;
	float2 _scZ = _signZ / _pDen;
	
	float2 _sX = offX * _scZ;
	float2 _sY = offY * _scZ;
	
	float2 pV = g_boundsLight2.xy - _sY * 0.125;
	float2 pU = (_sX - _signZ + 2.0) * 0.25;
#endif

#if NUM_LIGHTS == 3
	float3 offX = wPos.x - g_lightPos0.xyz;
	float3 offY = wPos.y - g_lightPos1.xyz;
	float3 offZ = wPos.z - g_lightPos2.xyz;
	
	float3 _signZ = sign(offZ);
	
	float3 _dist = sqrt(offX * offX + offY * offY + offZ * offZ);
	float3 _receiverDepth = _dist * g_boundsLight0.xyz - g_boundsLight1.xyz;
	
	float3 _pDen = _signZ * _dist + offZ;
	float3 _scZ = _signZ / _pDen;
	
	float3 _sX = offX * _scZ;
	float3 _sY = offY * _scZ;
	
	float3 pV = g_boundsLight2.xyz - _sY * 0.125;
	float3 pU = (_sX - _signZ + 2.0) * 0.25;
#endif

#if NUM_LIGHTS == 4
	float4 offX = wPos.x - g_lightPos0;
	float4 offY = wPos.y - g_lightPos1;
	float4 offZ = wPos.z - g_lightPos2;
	
	float4 _signZ = sign(offZ);
	
	float4 _dist = sqrt(offX * offX + offY * offY + offZ * offZ);
	float4 _receiverDepth = _dist * g_boundsLight0 - g_boundsLight1;
	
	float4 _pDen = _signZ * _dist + offZ;
	float4 _scZ = _signZ / _pDen;
	
	float4 _sX = offX * _scZ;
	float4 _sY = offY * _scZ;
	
	float4 pV = g_boundsLight2 - _sY * 0.125;
	float4 pU = (_sX - _signZ + 2.0) * 0.25;
#endif

#if NUM_LIGHTS >= 1
	OUT.Color.r = SampleChebyshev(float2(pU.x, pV.x), _receiverDepth.x);
#endif

#if NUM_LIGHTS >= 2
	OUT.Color.g = SampleChebyshev(float2(pU.y, pV.y), _receiverDepth.y);
#endif

#if NUM_LIGHTS >= 3
	OUT.Color.b = SampleChebyshev(float2(pU.z, pV.z), _receiverDepth.z);
#endif

#if NUM_LIGHTS >= 4
	OUT.Color.a = SampleChebyshev(float2(pU.w, pV.w), _receiverDepth.w);
#endif

    return OUT;
}
