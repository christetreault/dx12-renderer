cbuffer objectCB : register(b0)
{
   float4x4 M;
};

cbuffer passCB : register(b1)
{
   float4x4 V;
   float4x4 inverseV;
   float4x4 P;
   float4x4 inverseP;
   float4x4 PV;
   float4x4 inversePV;
   float4 eye;
   float nearZ;
   float farZ;
   float totalTime;
   float deltaTime;
};

struct VertexIn
{
   float4 pos : POSITION;
   float4 color : COLOR;
};

struct PixelIn
{
   float4 pos : SV_POSITION;
   float4 color : COLOR;
};

float mapRange(float s, float sMin, float sMax,
               float tMin, float tMax)
{
   return tMin + (((s - sMin) * (tMax - tMin)) / (sMax - sMin));
}

PixelIn vertexShader(VertexIn vin)
{
   PixelIn pin;

   float squimbler = cos(totalTime);
   squimbler = mapRange(squimbler, 0.0f, 1.0f, 0.3f, 0.7f);

   float4x4 PVM = mul(M, PV);
   float4 colorLow = normalize(mul(vin.color, PVM));
   float4 colorHigh = 2 * colorLow;

   pin.pos = mul(vin.pos, PVM);
   pin.color = lerp(colorLow, colorHigh, squimbler);

   return pin;
}