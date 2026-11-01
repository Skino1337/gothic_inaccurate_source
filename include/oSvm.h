#ifndef OSVM_H
#define OSVM_H

class zSTRING;

class oCSVM {
public:

    oCSVM();
    ~oCSVM();
    void InitByScript(int index);
    int  GetOU(const zSTRING& name);

    int      number;
    int      classindex;
    zSTRING* entry;
};

class oCSVMManager {
public:

    oCSVMManager();
    ~oCSVMManager();
    int GetOU(const zSTRING& name, int voice);

    int    svm_max;
    oCSVM* sv_module;
};

#endif
