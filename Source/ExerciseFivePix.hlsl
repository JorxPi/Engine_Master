cbuffer Material : register(b1)
{
    float4 colour;
    bool hasColourTex;
};

Texture2D colourTex : register(t0);
SamplerState colourSampler : register(s0);

float4 main(float2 coords : TEXCOORD) : SV_TARGET
{
    return hasColourTex ? colourTex.Sample(colourSampler, coords) * colour : colour;
}