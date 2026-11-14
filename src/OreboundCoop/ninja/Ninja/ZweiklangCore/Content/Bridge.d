// Own Ninja layer: stock Gothic II DNdR script calls only. No Toolkit/LeGo API.
// ABI 2: Union supplies validated command/value, SELF/OTHER/HERO bindings.
// Union retains authority, transactions, ownership, validation and persistence.
const int Patch_ZweiklangCore_ABI = 2;
var int Patch_ZweiklangCore_Command;
var int Patch_ZweiklangCore_Value;
var int Patch_ZweiklangCore_Result;

var int Patch_ZweiklangCore_Language;
var string Patch_ZweiklangCore_CavalornActive;
var string Patch_ZweiklangCore_CavalornDone;
var string Patch_ZweiklangCore_CavalornLetter;

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
    if (Patch_ZweiklangCore_Command == 4)
    {
        if ((Patch_ZweiklangCore_Value < 0) || (Patch_ZweiklangCore_Value > 1)) { return; };
        Patch_ZweiklangCore_Language = Patch_ZweiklangCore_Value;
        // BEGIN GENERATED LOCALIZED JOURNAL TEXT
        if (Patch_ZweiklangCore_Language == 1)
        {
            Patch_ZweiklangCore_CavalornActive = "Co-op: Wir helfen Cavalorn gegen die Banditen bei der Hoehle.";
            Patch_ZweiklangCore_CavalornDone = "Co-op: Die Banditen sind besiegt und Cavalorn hat die Aufgabe abgeschlossen.";
            Patch_ZweiklangCore_CavalornLetter = "Co-op: Cavalorn hat uns auf Vatras in Khorinis hingewiesen. Diese Folgequest ist noch kein gemeinsamer Testfall.";
        }
        else
        {
            Patch_ZweiklangCore_CavalornActive = "Co-op: We are helping Cavalorn fight the bandits near the cave.";
            Patch_ZweiklangCore_CavalornDone = "Co-op: The bandits are defeated and Cavalorn has completed the quest.";
            Patch_ZweiklangCore_CavalornLetter = "Co-op: Cavalorn mentioned Vatras in Khorinis. This follow-up quest is not yet a shared test case.";
        };
        // END GENERATED LOCALIZED JOURNAL TEXT
        Patch_ZweiklangCore_Result = 1;
    };

};
