#include <shared.fxh>

uint4 main(PS_INPUT input) : SV_TARGET
{
    return float4(PickingID, 0, 0, 0);
}