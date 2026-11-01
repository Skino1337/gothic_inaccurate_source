#include "include/oSvm.h"
#include "include/oGame.h"
#include "include/oCollisionObjectMiscChar.h"

oCSVMManager::oCSVMManager()
    : svm_max(0), sv_module(NULL)
{
    zCPar_Symbol* symbol = parser.GetSymbol("SVM_MODULES");
    if (!symbol)
    {
        zerr.Warning("U: SVM:  No Voice-Modules found.");
        return;
    }
    symbol->GetValue(svm_max, 0);
    sv_module = new oCSVM[svm_max];
    for (int i = 0; i < svm_max; ++i)
        sv_module[i].InitByScript(i);
}

oCSVMManager::~oCSVMManager()
{
    delete[] sv_module;
    sv_module = NULL;
}

int oCSVMManager::GetOU(const zSTRING& name, int voice)
{
    if (voice >= svm_max)
    {
        zerr.Warning("U: SVM: Voice-number too high (" + zSTRING(voice) + ")");
        return -1;
    }
    if (name[0] != '$')
    {
        int result = ogame->GetCutsceneManager()->LibValidateOU((zSTRING&)name);
        if (result < 0)
            zerr.Warning("U: SVM: Output-Unit \"" + name + "\" not found.");
        return result;
    }
    zSTRING ouName = name.Copied(1, name.Length() - 1);
    ouName.Upper();
    ouName.TrimRight(' ');
    ouName.TrimLeft(' ');
    int result = sv_module[voice].GetOU(ouName);
    if (result < 0)
        zerr.Warning("U: SVM: Output-Unit \"" + name + "\" not found (SVM_" +
            zSTRING(voice) + ")");
    return result;
}

oCSVM::oCSVM()
    : number(0), classindex(0), entry(NULL)
{
}

oCSVM::~oCSVM()
{
    delete[] entry;
    entry = NULL;
}

void oCSVM::InitByScript(int index)
{
    zSTRING moduleName = "SVM_" + zSTRING(index);
    int moduleIndex = parser.GetIndex(moduleName);
    if (moduleIndex < 0)
    {
        zerr.Warning("U: SVM: Voice Module not found :" + moduleName);
        return;
    }
    classindex = parser.GetIndex("C_SVM");
    if (classindex < 0)
    {
        zerr.Warning("U: SVM: Voice Module Class (C_SVM) not found.");
        return;
    }
    number = parser.GetSymbol(classindex)->ele & 0xFFF;
    entry = new zSTRING[number];
    parser.CreateInstance(moduleIndex, entry);
}

int oCSVM::GetOU(const zSTRING& name)
{
    for (int i = 0; i < number; ++i)
    {
        zCPar_Symbol* symbol = parser.GetSymbol(classindex + i + 1);
        if (!symbol)
            continue;
        zSTRING symbolName = symbol->name.Copied(6, symbol->name.Length() - 6);
        if (symbolName == name)
        {
            zCCSManager* manager = ogame->GetCutsceneManager();
            if (manager && !entry[i].IsEmpty())
                return manager->LibValidateOU(entry[i]);
            return -1;
        }
    }
    return -1;
}
