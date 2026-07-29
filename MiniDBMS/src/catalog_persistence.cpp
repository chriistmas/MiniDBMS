#include "catalog_persistence.h"
#include "page.h"
#include <cstring>

namespace {
constexpr int kCatalogMagic = 0x4D444231; // "MDB1"
constexpr int kHeaderBytes = static_cast<int>(sizeof(int)) * 2; // magic + length
}

bool CatalogPersistence::Save(IPageStore& page_store, const Catalog& catalog) {
    std::string text = catalog.Serialize();
    int length = static_cast<int>(text.size());

    if (length > PAGE_SIZE - kHeaderBytes) {
        return false; // catalogo demasiado grande para una sola pagina
    }

    char buffer[PAGE_SIZE];
    std::memset(buffer, 0, PAGE_SIZE);

    int magic = kCatalogMagic;
    std::memcpy(buffer, &magic, sizeof(int));
    std::memcpy(buffer + sizeof(int), &length, sizeof(int));
    std::memcpy(buffer + kHeaderBytes, text.data(), length);

    page_store.WritePage(0, buffer);
    return true;
}

bool CatalogPersistence::Load(IPageStore& page_store, Catalog& out_catalog) {
    char buffer[PAGE_SIZE];
    page_store.ReadPage(0, buffer);

    int magic = 0, length = 0;
    std::memcpy(&magic, buffer, sizeof(int));
    std::memcpy(&length, buffer + sizeof(int), sizeof(int));

    if (magic != kCatalogMagic || length <= 0 || length > PAGE_SIZE - kHeaderBytes) {
        return false; // Pagina 0 vacia o sin un catalogo reconocible
    }

    std::string text(buffer + kHeaderBytes, length);
    out_catalog = Catalog::Deserialize(text);
    return true;
}
