
sampler2D g_BlitTex0 : register(s0);

struct VS_IN
{
    float3 ObjPos : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 ProjPos : POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUT mainVS(VS_IN In)
{
    VS_OUT Out;

    Out.ProjPos = float4(In.ObjPos, 1.0);
    Out.TexCoord = In.TexCoord;
    return Out;
}

struct PS_IN
{
    float2 TexCoord : TEXCOORD0;
};

float3 X360GammaApprox(float3 x)
{
    float A = 0.541901f;
    float B = 1.13465f;
    float C = 13.53054f;
    float D = 6.56649f;
    float E = 0.311465f;

    x = max(0.0f, x);
    float3 f1 = A * x;
    float3 f2 = pow(x, B) * (1.0f - exp2(-C * x));
    float3 f3 = saturate(x * D + E);
    return lerp(f1, f2, f3);
}

float4 mainPS(PS_IN In) : COLOR
{
    float4 color = tex2D(g_BlitTex0, In.TexCoord);
    
    color.rgb = X360GammaApprox(color.rgb);
    return color;
}
