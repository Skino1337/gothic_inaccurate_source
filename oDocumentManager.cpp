#include "include/oDocumentManager.h"


zCViewDialog::zCViewDialog()
    : opened(0), refCtr(1)
{
}

zCViewDialog::~zCViewDialog()
{
}

void zCViewDialog::operator delete(void* memory)
{
    zCClassDef::ObjectDeleted(static_cast<zCObject*>(memory), classDef);
    ::operator delete(memory);
}

oCViewDocument::oCViewDocument()
    : numPages(0), currentPage(0), marginLeft(), marginRight(), marginFlags(0)
{
}

oCViewDocument::~oCViewDocument()
{
}

void oCViewDocument::SetPages(int pages)
{
    numPages = pages;
}

void oCViewDocument::SetFont(int page, const zSTRING& font)
{
    currentPage = page;
    fontName = font;
}

void oCViewDocument::SetMargins(int page, const zCPosition& left, const zCPosition& right,
    int flags)
{
    currentPage = page;
    marginLeft = left;
    marginRight = right;
    marginFlags = flags;
}

void oCViewDocument::PrintLine(int page, const zSTRING& text)
{
    currentPage = page;
    lines.InsertEnd(text);
}

void oCViewDocument::PrintLines(int page, const zSTRING& text)
{
    PrintLine(page, text);
}

oCViewDocumentMap::oCViewDocumentMap()
    : levelX1(0), levelY1(0), levelX2(0), levelY2(0), updateCounter(0)
{
}

void oCViewDocumentMap::SetLevel(const zSTRING& level)
{
    levelName = level;
}

void oCViewDocumentMap::SetLevelCoords(int x1, int y1, int x2, int y2)
{
    levelX1 = x1;
    levelY1 = y1;
    levelX2 = x2;
    levelY2 = y2;
}

void oCViewDocumentMap::UpdatePosition()
{
    ++updateCounter;
}

oCDocumentManager& oCDocumentManager::operator=(const oCDocumentManager&)
{
    return *this;
}

oCDocumentManager::oCDocumentManager(const oCDocumentManager&)
    : ListDocuments(0), DlgDocumentMain(0)
{
}

oCDocumentManager::oCDocumentManager()
{
    ListDocuments = new zCListSort<oCViewDocument>;
    ListDocuments->data = 0;
    ListDocuments->next = 0;
    DlgDocumentMain = new zCViewDialog;
    zCClassDef::ObjectCreated(DlgDocumentMain, zCViewDialog::classDef);
}

oCDocumentManager::~oCDocumentManager()
{
    if (ListDocuments) {
        ListDocuments->DeleteListDatas();
        delete ListDocuments;
    }
    delete DlgDocumentMain;
    DlgDocumentMain = 0;
}

oCDocumentManager& oCDocumentManager::GetDocumentManager()
{
    static oCDocumentManager manager;
    return manager;
}

void oCDocumentManager::SetPages(int document, int pages)
{
    oCViewDocument* view = GetDocumentView(document);
    if (view)
        view->SetPages(pages);
}

void oCDocumentManager::SetPage(int document, int page, zSTRING& text, int flags)
{
    oCViewDocument* view = GetDocumentView(document);
    if (view)
        view->PrintLine(page, text);
}

void oCDocumentManager::SetFont(int document, int page, zSTRING& font)
{
    oCViewDocument* view = GetDocumentView(document);
    if (view)
        view->SetFont(page, font);
}

void oCDocumentManager::SetMargins(int document, int page, zCPosition& left,
    zCPosition& right, int flags)
{
    oCViewDocument* view = GetDocumentView(document);
    if (view)
        view->SetMargins(page, left, right, flags);
}

void oCDocumentManager::SetLevel(int document, const zSTRING& level)
{
    oCViewDocumentMap* map = dynamic_cast<oCViewDocumentMap*>(GetDocumentView(document));
    if (map)
        map->SetLevel(level);
}

void oCDocumentManager::SetLevelCoords(int document, int x1, int y1, int x2, int y2)
{
    oCViewDocumentMap* map = dynamic_cast<oCViewDocumentMap*>(GetDocumentView(document));
    if (map)
        map->SetLevelCoords(x1, y1, x2, y2);
}

void oCDocumentManager::PrintLine(int document, int page, zSTRING& text)
{
    oCViewDocument* view = GetDocumentView(document);
    if (view)
        view->PrintLine(page, text);
}

void oCDocumentManager::PrintLines(int document, int page, zSTRING& text)
{
    oCViewDocument* view = GetDocumentView(document);
    if (view)
        view->PrintLines(page, text);
}

int oCDocumentManager::CreateDocument()
{
    ListDocuments->DeleteListDatas();
    int id = GetNextAvailableID();
    oCViewDocument* document = new oCViewDocument;
    zCClassDef::ObjectCreated(document, zCViewDialog::classDef);
    zCListSort<oCViewDocument>* entry = new zCListSort<oCViewDocument>;
    entry->data = document;
    entry->next = ListDocuments->next;
    ListDocuments->next = entry;
    return id;
}

int oCDocumentManager::CreateMap()
{
    ListDocuments->DeleteListDatas();
    int id = GetNextAvailableID();
    oCViewDocumentMap* document = new oCViewDocumentMap;
    zCClassDef::ObjectCreated(document, zCViewDialog::classDef);
    zCListSort<oCViewDocument>* entry = new zCListSort<oCViewDocument>;
    entry->data = document;
    entry->next = ListDocuments->next;
    ListDocuments->next = entry;
    return id;
}

void oCDocumentManager::Show(int document)
{
    oCViewDocument* view = GetDocumentView(document);
    if (!view)
        return;
    oCViewDocumentMap* map = dynamic_cast<oCViewDocumentMap*>(view);
    if (map)
        map->UpdatePosition();
    view->opened = 1;
}

int oCDocumentManager::GetNextAvailableID()
{
    int result = 0;
    zCListSort<oCViewDocument>* current = ListDocuments->next;
    while (current) {
        ++result;
        current = current->next;
    }
    return result;
}

oCViewDocument* oCDocumentManager::GetDocumentView(int document)
{
    zCListSort<oCViewDocument>* current = ListDocuments->next;
    int index = 0;
    while (current) {
        if (index == document)
            return current->data;
        ++index;
        current = current->next;
    }
    return 0;
}

void oCDocumentManager::UpdateMap(int document)
{
    oCViewDocumentMap* map = dynamic_cast<oCViewDocumentMap*>(GetDocumentView(document));
    if (map)
        map->UpdatePosition();
}

int oCDocumentManager::HandleEvent(int key)
{
    if (key == 0x0e || key == 0x16 || key == 0x0f || key == 0x1c || key == 0x20d) {
        ListDocuments->DeleteListDatas();
        DlgDocumentMain->opened = 0;
    }
    return 1;
}
