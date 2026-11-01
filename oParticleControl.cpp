#include "include/oParticleControl.h"
#include "include/oCollisionObjectMiscChar.h"
#include "include/zMorphMesh.h"

oCParticleControl::oCParticleControl()
    : partFX(0), pfxVob(0), pfxVobID(0), pfxName("a.pfx"), IsInEditMode(0), rnd_world(0),
      part_edit(0)
{
}

oCParticleControl::oCParticleControl(zCWorld* world)
    : partFX(new zCParticleFX), pfxVob(new zCVob), pfxVobID(0), pfxName("a.pfx"), IsInEditMode(0),
      rnd_world(world), part_edit(0)
{
    zCClassDef::ObjectCreated(partFX, zCParticleFX::classDef);
    zCClassDef::ObjectCreated(pfxVob, zCVob::classDef);
    pfxVob->SetVobName("Particle_Test_Vob");
    pfxVob->SetCollDetStat(0);
    pfxVob->SetCollDetDyn(0);
    rnd_world->AddVob(pfxVob);
}

oCParticleControl::~oCParticleControl()
{
    delete part_edit;
    part_edit = NULL;
    partFX = NULL;
    pfxVob = NULL;
    rnd_world = NULL;
}

zCVob* oCParticleControl::GetPFXVob()
{
    return pfxVob;
}

void oCParticleControl::SelectActivePFX()
{
    zrenderer->Vid_Clear(zCOLOR(0, 0, 0, 255), 3);
    zrenderer->Vid_Blit(1, 0, 0);
    zrenderer->Vid_Clear(zCOLOR(0, 0, 0, 255), 3);
    zrenderer->Vid_Blit(1, 0, 0);
    int x = 0;
    int y = 0;
    for (int i = 0; i < zCParticleFX::s_emitterPresetList.numInArray; ++i)
    {
        screen->Print(x, y, zSTRING(i) + ": " +
            zCParticleFX::s_emitterPresetList[i]->particleFXName);
        y += 200;
        if (i % 36 == 35)
        {
            x += 2048;
            y = 0;
        }
    }
    int selected;
    do
    {
        zSTRING input;
        screen->Input(0, 7900, 2000, input);
        selected = atoi(input);
    } while (selected < 0 || selected >= zCParticleFX::s_emitterPresetList.numInArray);
    pfxName = zCParticleFX::s_emitterPresetList[selected]->particleFXName;
    while (!zinput->KeyPressed(0x37) && !zinput->KeyPressed(0x2B) && !zinput->KeyPressed(0x1C))
    {
        zCInputCallback::GetInput();
        sysEvent();
    }
}

void oCParticleControl::EditActivePFX()
{
    if (!pfxVob || !zCParticleFX::s_pfxParser)
        return;
    delete part_edit;
    part_edit = new zCConsole;
    part_edit->SetParser(zCParticleFX::s_pfxParser);
    part_edit->SetPos(0, 0);
    part_edit->EditInstance(pfxName, partFX->emitter);
    zCParticleEmitter* emitter = zCParticleFX::SearchParticleEmitter(pfxName);
    *emitter = *partFX->emitter;
    emitter->UpdateInternals();
    partFX->emitter->UpdateInternals();
}

void oCParticleControl::SetStartPos(zVEC3& position)
{
    startpos = position;
}

void oCParticleControl::StartActivePFX(zCVob* vob, zSTRING& nodeName)
{
    if (!rnd_world)
        return;
    DeleteLastPFX();
    zCModel* model = zDYNAMIC_CAST<zCModel>(vob->visual);
    zCModelNodeInst* node = model ? model->SearchNode(nodeName) : NULL;
    if (!node)
        return;
    partFX = new zCParticleFX;
    pfxVob = new zCVob;
    zCClassDef::ObjectCreated(partFX, zCParticleFX::classDef);
    zCClassDef::ObjectCreated(pfxVob, zCVob::classDef);
    pfxVob->SetVobName("Particle_Test_Vob");
    pfxVob->SetCollDetStat(0);
    pfxVob->SetCollDetDyn(0);
    rnd_world->AddVob(pfxVob);
    model->AttachChildVobToNode(pfxVob, node);
    model->UpdateAttachedVobs();
    pfxVob->SetVisual(partFX);
    partFX->SetAndStartEmitter(pfxName, 0);
    partFX->emitterIsOwned = 0;
}

void oCParticleControl::StartActivePFX()
{
    if (!rnd_world)
        return;
    DeleteLastPFX();
    partFX = new zCParticleFX;
    pfxVob = new zCVob;
    zCClassDef::ObjectCreated(partFX, zCParticleFX::classDef);
    zCClassDef::ObjectCreated(pfxVob, zCVob::classDef);
    pfxVob->SetVobName("Particle_Test_Vob");
    pfxVob->SetCollDetStat(0);
    pfxVob->SetCollDetDyn(0);
    pfxVob->Move(startpos.n[0], startpos.n[1], startpos.n[2]);
    rnd_world->AddVob(pfxVob);
    pfxVob->SetVisual(partFX);
    partFX->SetAndStartEmitter(pfxName, 0);
    partFX->emitterIsOwned = 0;
}

void oCParticleControl::CreateNewPFX(zSTRING& name)
{
    name.Upper();
    zoptions->ChangeDir(1);
    zFILE* file = zfactory->CreateZFile("System\\Pfx\\PfxInst.d");
    file->Open(0);
    file->Append();
    file->Write(zSTRING("\nINSTANCE ") + name + "(C_PARTICLEFX)\n");
    file->Write("{\n");
    file->Write("};\n");
    file->Close();
    delete file;
    zCParticleFX::ParseParticleFXScript();
}

void oCParticleControl::SetPFXName(const zSTRING& name)
{
    pfxName = name;
    if (partFX)
        partFX->SetEmitter(zCParticleFX::SearchParticleEmitter(name), 0);
}

void oCParticleControl::SetWorld(zCWorld* world)
{
    if (pfxVob)
    {
        rnd_world->RemoveVob(pfxVob);
        pfxVob->Release();
        pfxVob = NULL;
    }
    rnd_world = world;
}

void oCParticleControl::BeginEditActive(void(*changedFunc)(const zSTRING&))
{
    if (!pfxVob || !zCParticleFX::s_pfxParser)
        return;
    delete part_edit;
    part_edit = new zCConsole;
    part_edit->SetParser(zCParticleFX::s_pfxParser);
    part_edit->SetPos(0, 0);
    part_edit->EditInstance(pfxName, partFX->emitter);
    part_edit->SetChangedFunc(changedFunc);
}

void oCParticleControl::DeleteLastPFX()
{
    if (!pfxVob)
        return;
    zCWorld* world = pfxVob->homeWorld;
    if (world)
    {
        if (pfxVob->refCtr >= 2)
            world->RemoveVob(pfxVob);
        pfxVob->Release();
        pfxVob = new zCVob;
        zCClassDef::ObjectCreated(pfxVob, zCVob::classDef);
        partFX = new zCParticleFX;
        zCClassDef::ObjectCreated(partFX, zCParticleFX::classDef);
    }
}

void oCParticleControl::UpdateInternals()
{
    if (partFX)
        partFX->emitter->UpdateInternals();
}

void oCParticleControl::EndEditActive()
{
    zCParticleEmitter* emitter = zCParticleFX::SearchParticleEmitter(pfxName);
    *emitter = *partFX->emitter;
    emitter->UpdateInternals();
    partFX->emitter->UpdateInternals();
}
