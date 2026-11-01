#ifndef ODOCUMENTMANAGER_H
#define ODOCUMENTMANAGER_H

#include "zSession.h"

struct zCPosition {
    int X;
    int Y;
};

class zCViewDialog : public zCObject {
public:

    zCViewDialog();
    virtual ~zCViewDialog();
    static void operator delete(void* memory);

    int opened;
    int refCtr;

    static zCClassDef* classDef;
};

class oCViewDocument : public zCViewDialog {
public:

    oCViewDocument();
    virtual ~oCViewDocument();
    virtual void SetPages(int pages);
    virtual void SetFont(int page, const zSTRING& font);
    virtual void SetMargins(int page, const zCPosition& left, const zCPosition& right, int flags);
    virtual void PrintLine(int page, const zSTRING& text);
    virtual void PrintLines(int page, const zSTRING& text);

    int numPages;
    int currentPage;
    zSTRING fontName;
    zCPosition marginLeft;
    zCPosition marginRight;
    int marginFlags;
    zCArray<zSTRING> lines;
};

class oCViewDocumentMap : public oCViewDocument {
public:

    oCViewDocumentMap();
    virtual void SetLevel(const zSTRING& level);
    virtual void SetLevelCoords(int x1, int y1, int x2, int y2);
    virtual void UpdatePosition();

    zSTRING levelName;
    int levelX1;
    int levelY1;
    int levelX2;
    int levelY2;
    int updateCounter;
};

template <class T>
class zCListSort {
public:

    ~zCListSort()
    {
        delete next;
        next = 0;
    }

    void Insert(T* value)
    {
        zCListSort<T>* node = new zCListSort<T>;
        node->data = value;
        node->next = next;
        next = node;
    }

    void InsertSort(T* value)
    {
        zCListSort<T>* node = new zCListSort<T>;
        node->data = value;
        node->next = 0;
        zCListSort<T>* current = this;
        while (current->next && compare_func && compare_func(value, current->next->data) >= 0)
            current = current->next;
        node->next = current->next;
        current->next = node;
    }

    void DeleteListDatas()
    {
        delete data;
        data = 0;
        if (next) {
            next->DeleteListDatas();
            delete next;
            next = 0;
        }
    }

    int GetNumInList() const
    {
        int count = 0;
        const zCListSort<T>* node = next;
        while (node) {
            ++count;
            node = node->next;
        }
        return count;
    }

    int (*compare_func)(T* first, T* second);
    T* data;
    zCListSort<T>* next;
};

class oCDocumentManager : public zCInputCallback {
public:

    oCDocumentManager& operator=(const oCDocumentManager&);
    oCDocumentManager(const oCDocumentManager&);
    oCDocumentManager();
    ~oCDocumentManager();
    int CreateDocument();
    int CreateMap();
    void SetPages(int document, int pages);
    void SetPage(int document, int page, zSTRING& text, int flags);
    void SetFont(int document, int page, zSTRING& font);
    void SetMargins(int document, int page, zCPosition& left, zCPosition& right, int flags);
    void SetLevel(int document, const zSTRING& level);
    void SetLevelCoords(int document, int x1, int y1, int x2, int y2);
    void PrintLine(int document, int page, zSTRING& text);
    void PrintLines(int document, int page, zSTRING& text);
    void Show(int document);
    int GetNextAvailableID();
    oCViewDocument* GetDocumentView(int document);
    void UpdateMap(int document);
    static oCDocumentManager& GetDocumentManager();
    virtual int HandleEvent(int key);

    zCListSort<oCViewDocument>* ListDocuments;
    zCViewDialog* DlgDocumentMain;
};

#endif
