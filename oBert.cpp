#include "include/zConsole.h"
#include "include/oGame.h"
#include "include/zMoving.h"
#include "include/zFile.h"

extern zCOption* zgameoptions;

int EvalBertConsoleCommand(const zSTRING& command, zSTRING& result)
{
    const zSTRING action = command.PickWord(1, zSTR_SKIP, zSTR_SKIP);
    const zSTRING subject = command.PickWord(2, zSTR_SKIP, zSTR_SKIP);
    const zSTRING parameter = command.PickWord(3, zSTR_SKIP, zSTR_SKIP);
    if (action == "SET" && subject == "CLIPPINGFACTOR") {
        float value = parameter.ToFloat();
        if (value < 0.05f)
            value = 0.05f;
        else if (value > 10.0f)
            value = 10.0f;
        if (ogame && ogame->GetWorld()->GetActiveSkyControler())
            ogame->GetWorld()->GetActiveSkyControler()->SetFarZScalability(value);
        result = "ok";
        return 1;
    }
    if (action == "LIST" && subject == "CS" && ogame) {
        ogame->GetWorld()->csPlayer->PrintListOfCutscenes(parameter);
        result = "ok";
        return 1;
    }
    if (action == "SHOW" && subject == "CS" && ogame) {
        zCCSManager* manager = ogame->GetCutsceneManager();
        zSTRING& name = const_cast<zSTRING&>(parameter);
        result = "Started: " + zSTRING(manager->PoolNumPlayed(name));
        return 1;
    }
    if (action == "PLAY" && subject == "VIDEO") {
        gameMan->PlayVideo(parameter);
        result = "ok";
        return 1;
    }
    if (action == "PLAY" && subject == "CS") {
        oCNpc* target = oCNpc::player ? oCNpc::player->GetFocusNpc() : NULL;
        if (!target)
            target = oCNpc::player;
        if (!target)
            return 0;
        oCMsgConversation* message = new oCMsgConversation(6, parameter, target->GetObjectName());
        zCClassDef::ObjectCreated(message, oCMsgConversation::classDef);
        target->GetEM(0)->OnMessage(message, target);
        result = "Started playing cutscene for npc \"" + target->GetObjectName() + "\"";
        return 1;
    }
    if (action == "TOGGLE" && subject == "CUTSCENES" && parameter == "ENABLED") {
        zCCSCutsceneContext::playDisabled = !zCCSCutsceneContext::playDisabled;
        result = "toggled cutscenes";
        return 1;
    }
    if (action == "TOGGLE" && subject == "CS" && parameter == "LIST" && ogame) {
        zCCSPlayer* player = ogame->GetWorld()->csPlayer;
        player->debugInfoOnScreen = !player->debugInfoOnScreen;
        result = "toggled cutscene-debuginfos";
        return 1;
    }
    return 0;
}

void InitBertSettings()
{
    zSTRING script = zoptions->ParmValue("PARSE");
    const zSTRING world = zoptions->ParmValue("WORLD");
    const zSTRING player = zoptions->ParmValue("PLAYER");
    if (!script.IsEmpty()) {
        zFILE* file = zfactory->CreateZFile(script);
        if (file) {
            file->Open(0);
            script.Upper();
            if (script.Search("SRC") >= 0)
                script.DeleteRight(4);
            if (zgameoptions)
                zgameoptions->WriteString("FILES", "Game", script);
            else
                zoptions->WriteString("internal", "gameScript", script);
            file->Close();
            delete file;
        }
    }
    if (!world.IsEmpty()) {
        if (zgameoptions)
            zgameoptions->WriteString("SETTINGS", "World", world);
        else
            zoptions->WriteString("internal", "gamePath", world);
    }
    if (!player.IsEmpty()) {
        if (zgameoptions)
            zgameoptions->WriteString("SETTINGS", "Player", player);
        else
            zoptions->WriteString("internal", "playerInstanceName", player);
    }
    gLogStatistics = zoptions->ReadBool(zOPT_SEC_INTERNAL, "logStatistics", 0);
}

void oBert_StartUp()
{
    zcon.Register("SET CLIPPINGFACTOR",
        "Setting the clipping-factor. Default is 1. Usually check 0.1 ... 2.0");
    zcon.Register("LIST CS", "List running cutscenes.");
    zcon.Register("LIST CS STATES", "List running cutscenes including their current status.");
    zcon.Register("LIST CS PROPS", "List running cutscenes including status and properties.");
    zcon.Register("LIST CS HISTORY", "List all cutscenes in the history-pool.");
    zcon.Register("SHOW CS",
        "Show information of a cutscene (add cutscene-name including extension)");
    zcon.Register("PLAY CS", "Play a cutscene (add cutscene-name including extension)");
    zcon.Register("PLAY VIDEO", "Playing a videofile");
    zcon.Register("TOGGLE CS LIST", "Toggle list of running cutscenes on screen");
    zcon.Register("TOGGLE CUTSCENES ENABLED", "Enable or disable cutscene-system");
    zcon.Register("HERO EXPORT", "Exports your hero to the given file.");
    zcon.Register("HERO IMPORT", "Imports your hero from the given file.");
    zcon.AddEvalFunc(EvalBertConsoleCommand);
    InitBertSettings();
}

void oBert_ShutDown()
{
}

void ShowMoreInfos(oCNpc* npc, oCNpc* other)
{
    if (!npc)
        return;
    zCView* view = new zCView(0, 0, 0x2000, 0x2000, 1);
    screen->InsertItem(view, 0);
    view->InsertBack("menu_ingame.tga");
    const int width = screen->anx(800);
    const int height = screen->any(600);
    view->SetPos((0x2000 - width) / 2, (0x2000 - height) / 2);
    view->SetSize(width, height);
    view->DefPrintwin(1500, 1500, 6700, 6700);
    view->ClrFlags(0x200);
    zSTRING text = zSTRING("Statistics for [") + npc->GetName() + "]\n";
    text += zSTRING("Guild: ") + npc->GetGuildName() + "\n";
    zSTRING line;
    line.Sprintf("Hitpoints: %d/%d\n", npc->GetAttribute(0), npc->GetAttribute(1));
    text += line;
    line.Sprintf("Mana: %d/%d\n", npc->GetAttribute(2), npc->GetAttribute(3));
    text += line;
    line.Sprintf("Strength: %d\n", npc->GetAttribute(4));
    text += line;
    line.Sprintf("Dexterity: %d\n", npc->GetAttribute(5));
    text += line;
    if (other) {
        zSTRING attitude = "neutral";
        if (npc->IsHostile(other))
            attitude = "hostile";
        else if (npc->IsAngry(other))
            attitude = "angry";
        else if (npc->IsFriendly(other))
            attitude = "friendly";
        text += zSTRING("Attitude to me: ") + attitude + "\n";
    }
    view->Printwin(text);
    view->Render();
    zrenderer->Vid_Blit(1, NULL, NULL);
    const unsigned long startTime = sysGetTime();
    while (!zinput->KeyPressed(1) && sysGetTime() - startTime < 5000)
        zinput->ProcessInputEvents();
    screen->RemoveItem(view);
    delete view;
    zinput->ClearKeyBuffer();
    zinput->ResetRepeatKey(1);
}

extern zTIMER ztimer;

static void PrintResult(const zSTRING& text)
{
    if (ogame && ogame->array_view[0])
        ogame->array_view[0]->PrintTimed(100, 300, text, 3000.0f);
}

static void ScaleNpcs(const zVEC3& scale)
{
    if (!ogame || !ogame->GetWorld() || !ogame->GetWorld()->voblist_npcs)
        return;
    for (zCListSort<oCNpc>* entry = ogame->GetWorld()->voblist_npcs->next; entry; entry = entry->next) {
        if (entry->data != oCNpc::player)
            entry->data->GetModel()->SetModelScale(scale);
    }
}

void HandleResultString(zSTRING result)
{
    result.Upper();
    unsigned long hash = 0;
    for (int i = 0; i < result.Length(); ++i)
        hash += (unsigned char)result[i] * i * 11;
    zerr->Message(zSTRING("B: TEST: Result: ") + result);
    switch (hash) {
    case 0x226:
        ztimer.factorMotion = 1.0f;
        oCNpc::player->GetModel()->timeScale = 1.0f;
        oCNpc::godmode = 0;
        Game_ToggleTestMode(0);
        ogame->game_frameinfo = 0;
        ogame->game_showtime = 0;
        PrintResult("WHAT WAS THE QUESTION?");
        break;
    case 0xb16:
        ShowMoreInfos(oCNpc::player->GetFocusNpc(), oCNpc::player);
        break;
    case 0x12a6:
        oCNpc::player->SetAttribute(0, oCNpc::player->GetAttribute(1));
        oCNpc::player->SetAttribute(2, oCNpc::player->GetAttribute(3));
        PrintResult("TAKI makes you feal fine");
        break;
    case 0x1f95:
        ogame->game_showtime = !ogame->game_showtime;
        PrintResult(ogame->game_showtime ? "CLOCK TICK TOCK" : "CLOCK OFF");
        break;
    case 0x1fcc:
        ogame->game_frameinfo = !ogame->game_frameinfo;
        break;
    case 0x1fed:
        ogame->SwitchCamToNextNpc();
        PrintResult("FROM EARTH TO NEXT NPC");
        break;
    case 0x20a8:
    case 0x47f9:
        oCNpc::player->PutInInv("ITMI_NUGGET", 1);
        PrintResult("some more ore ...");
        break;
    case 0x23c0:
        if (oCNpc::player->GetFocusNpc())
            oCNpc::player->GetFocusNpc()->GetModel()->timeScale = 0.4f;
        break;
    case 0x2ec0:
        zrenderer->SetPolyDrawMode(zRND_DRAW_WIRE);
        PrintResult("MAITAI-Mode");
        break;
    case 0x323b:
        Game_ToggleTestMode(1);
        PrintResult("MARVIN-MODE");
        break;
    case 0x327d:
        ztimer.factorMotion = 0.4f;
        PrintResult("CLERKS");
        break;
    case 0x5766:
        oCNpc::godmode = !oCNpc::godmode;
        PrintResult(oCNpc::godmode ? "GUARDIAN-MODE" : "GUARDIAN-MODE off");
        break;
    case 0x5913:
        gameMan->ShowRealPlayTime();
        break;
    case 0x764b:
        ztimer.factorMotion = 3.0f;
        PrintResult("SOUTHPARK");
        break;
    case 0x8d8a:
        if (oCNpc::player->GetFocusNpc())
            oCNpc::player->GetFocusNpc()->GetModel()->timeScale = 3.0f;
        PrintResult("HIGHLANDER");
        break;
    case 0x8db6:
        zrenderer->SetPolyDrawMode(zRND_DRAW_FLAT);
        PrintResult("CAIPI-Mode");
        break;
    case 0x2100:
        if (ogame && ogame->GetWorld()) {
            oCNpc* lares = dynamic_cast<oCNpc*>(ogame->GetWorld()->SearchVobByName("VLK_449_LARES"));
            if (lares && lares->IsInInv("ITMI_ORNAMENT_ADDON_VATRAS", 1)) {
                lares->RemoveFromInv("ITMI_ORNAMENT_ADDON_VATRAS", 1);
                oCNpc::player->PutInInv("ITMI_ORNAMENT_ADDON_VATRAS", 1);
                PrintResult("Ornament von Lares erhalten");
            }
        }
        break;
    case 0x2f44:
        if (ogame && ogame->GetWorld() && ogame->GetWorld()->voblist_npcs) {
            for (zCListSort<oCNpc>* entry = ogame->GetWorld()->voblist_npcs->next; entry;
                entry = entry->next) {
                if (entry->data != oCNpc::player)
                    entry->data->GetModel()->fatness = -6.0f;
            }
        }
        break;
    case 0x46d0:
        ScaleNpcs(zVEC3(0.2f, 1.3f, 1.3f));
        PrintResult("GROMMIT-AFTER-CAR-ACCIDENT-MODE");
        break;
    case 0x5648:
        if (ogame && ogame->GetWorld() && ogame->GetWorld()->voblist_npcs) {
            for (zCListSort<oCNpc>* entry = ogame->GetWorld()->voblist_npcs->next; entry;
                entry = entry->next) {
                if (entry->data != oCNpc::player)
                    entry->data->GetModel()->fatness = 6.0f;
            }
        }
        PrintResult("GARFIELD-MODE");
        break;
    case 0x58dc:
        ScaleNpcs(zVEC3(5.0f, 5.0f, 5.0f));
        PrintResult("GODZILLA MUTATION");
        break;
    case 0x59ce:
        if (ogame && ogame->GetWorld()) {
            zCSkyControler_Outdoor* sky = dynamic_cast<zCSkyControler_Outdoor*>(
                ogame->GetWorld()->GetActiveSkyControler());
            if (sky)
                sky->SetRainFXWeight(0.1f, 0.0f);
        }
        PrintResult("MOBY DICKS ELEMENT");
        break;
    case 0x5a52:
        ScaleNpcs(zVEC3(0.4f, 0.4f, 0.4f));
        PrintResult("KINGKONG-POINT-OF-VIEW");
        break;
    case 0x118ac:
        if (!oCNpc::player->IsInInv("ITSC_SUMMONMUD", 1)) {
            oCNpc::player->PutInInv("ITSC_SUMMONMUD", 1);
            PrintResult("THE QUEEN IS DEAD");
        }
        break;
    }
}

static zCArray<zSTRING> statisticsLog;
static int statisticsLastDay;

void ClearLogStatistics()
{
    statisticsLog.numInArray = 0;
    zerr->Message("B: LogStatistics: Cleared.");
}

bool LogStatistics(oCNpc* npc)
{
    if (!ogame || !npc)
        return false;
    int day;
    int hour;
    int minute;
    ogame->GetTime(day, hour, minute);
    if (statisticsLastDay == day)
        return false;
    statisticsLastDay = day;
    const int minutes = (day * 24 + hour - 8) * 60 + minute;
    zSTRING text;
    text += zSTRING(minutes) + "\t";
    text += zSTRING(day) + "\t";
    text += zSTRING(hour) + "\t";
    text += zSTRING(minute) + "\t";
    text += zSTRING(npc->level) + "\t";
    text += zSTRING(npc->experience_points) + "\t";
    text += zSTRING(npc->learn_points) + "\t";
    text += npc->GetGuildName() + "\t";
    text += zSTRING(npc->GetAttribute(4)) + "\t";
    text += zSTRING(npc->GetAttribute(5)) + "\t";
    text += zSTRING(npc->GetAttribute(1)) + "\t";
    text += zSTRING(npc->GetAttribute(3)) + "\t";
    const int talents[] = { 1, 2, 3, 4, 7, 5, 6, 8, 11 };
    for (int i = 0; i < 9; ++i)
        text += zSTRING(npc->GetTalentSkill(talents[i])) + "\t";
    text += "\n";
    statisticsLog.InsertEnd(text);
    return false;
}

bool LogStatisticsWrite(const zSTRING& path)
{
    zFILE_FILE file(path + "log.sav");
    if (file.Create())
        return false;
    file.Write("minutes\tday\thour\tminute\tlevel\texperience\tlearnpoints\tguild\tstrength\t"
        "dexterity\tmax.hitpoints\tmax.mana\t1handed\t2handed\tbow\tcrossbow\tcircle\t"
        "picklock\tpickpocket\tsneak\tacrobatic\t\n");
    for (int i = 0; i < statisticsLog.numInArray; ++i)
        file.Write(statisticsLog.parray[i]);
    file.Close();
    return true;
}

bool LogStatisticsRead(const zSTRING& path)
{
    ClearLogStatistics();
    zFILE_FILE file(path + "log.sav");
    if (file.Open(false))
        return false;
    zSTRING line;
    file.Read(line);
    while (!file.Eof()) {
        file.Read(line);
        if (line.Length() > 0)
            statisticsLog.InsertEnd(line + "\n");
    }
    file.Close();
    return true;
}
