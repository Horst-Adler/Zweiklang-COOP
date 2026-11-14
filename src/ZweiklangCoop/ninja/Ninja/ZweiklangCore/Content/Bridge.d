// Own Ninja layer: stock Gothic II DNdR script calls only. No Toolkit/LeGo API.
// ABI 1: Union supplies validated command/value, SELF/OTHER/HERO bindings.
// Union retains authority, transactions, ownership, validation and persistence.
const int Patch_ZweiklangCore_ABI = 1;
var int Patch_ZweiklangCore_Command;
var int Patch_ZweiklangCore_Value;
var int Patch_ZweiklangCore_Result;

func void Patch_ZweiklangCore_Dispatch()
{
    Patch_ZweiklangCore_Result = 0;
    if (Patch_ZweiklangCore_Command == 0)
    {
        if (Patch_ZweiklangCore_Value == 0) { Patch_ZweiklangCore_Result = 1; };
        return;
    };
    if (Patch_ZweiklangCore_Command == 1)
    {
        if (Patch_ZweiklangCore_Value != 0) { return; };
        B_GiveTradeInv(other);
        Patch_ZweiklangCore_Result = 1;
        return;
    };
    if (Patch_ZweiklangCore_Command == 2)
    {
        if (Patch_ZweiklangCore_Value != 0) { return; };
        B_ClearRuneInv(other);
        Patch_ZweiklangCore_Result = 1;
        return;
    };
    if (Patch_ZweiklangCore_Command == 3)
    {
        if ((Patch_ZweiklangCore_Value <= 0) || (Patch_ZweiklangCore_Value > 1000000)) { return; };
        B_GivePlayerXP(Patch_ZweiklangCore_Value);
        Patch_ZweiklangCore_Result = 1;
    };
};
