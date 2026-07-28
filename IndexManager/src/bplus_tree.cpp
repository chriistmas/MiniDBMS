#include "bplus_tree.h"
#include "page.h"
#include "record.h"
#include <algorithm>
#include <iostream>
#include <cstring>

BPlusTree::BPlusTree(IPageStore& page_store, int max_entries, int root_page_id)
    : page_store_(page_store), max_entries_(max_entries), root_page_id_(root_page_id) {
    if (root_page_id_ == -1) {
        root_page_id_ = CreateEmptyLeaf();
    }
}

int BPlusTree::GetRootPageId() const { return root_page_id_; }

// ---------------------------------------------------------------------
// Helpers de lectura/escritura de nodos
// ---------------------------------------------------------------------

bool BPlusTree::IsLeaf(int page_id) const {
    char raw[PAGE_SIZE];
    page_store_.ReadPage(page_id, raw);
    Page page(page_id);
    std::memcpy(page.GetData(), raw, PAGE_SIZE);

    char buf[PAGE_SIZE];
    int size = 0;
    page.GetRecord(0, buf, size); // slot 0 = meta
    Record meta = Record::Deserialize(buf, size);
    return meta.GetInt(0) == 1;
}

std::vector<BPlusTree::Entry> BPlusTree::ReadEntries(int page_id) const {
    char raw[PAGE_SIZE];
    page_store_.ReadPage(page_id, raw);
    Page page(page_id);
    std::memcpy(page.GetData(), raw, PAGE_SIZE);

    std::vector<Entry> entries;
    int slot_count = page.GetSlotCount();
    for (int slot = 1; slot < slot_count; slot++) { // slot 0 es meta
        char buf[PAGE_SIZE];
        int size = 0;
        if (page.GetRecord(slot, buf, size)) {
            Record r = Record::Deserialize(buf, size);
            Entry e;
            e.key = r.GetInt(0);
            e.a = r.GetInt(1);
            e.b = (r.FieldCount() >= 3) ? r.GetInt(2) : -1;
            entries.push_back(e);
        }
    }
    std::sort(entries.begin(), entries.end(),
              [](const Entry& x, const Entry& y) { return x.key < y.key; });
    return entries;
}

int BPlusTree::ReadLeftmostOrNext(int page_id) const {
    char raw[PAGE_SIZE];
    page_store_.ReadPage(page_id, raw);
    Page page(page_id);
    std::memcpy(page.GetData(), raw, PAGE_SIZE);
    return page.GetNextPageId();
}

void BPlusTree::WriteLeaf(int page_id, const std::vector<Entry>& entries, int next_leaf_id) {
    Page page(page_id);
    page.SetNextPageId(next_leaf_id);

    Record meta;
    meta.AddInt(1); // 1 = hoja
    char meta_buf[PAGE_SIZE];
    int meta_size = meta.Serialize(meta_buf);
    page.InsertRecord(meta_buf, meta_size); // ocupa el slot 0

    for (const Entry& e : entries) {
        Record r;
        r.AddInt(e.key);
        r.AddInt(e.a);
        r.AddInt(e.b);
        char buf[PAGE_SIZE];
        int size = r.Serialize(buf);
        page.InsertRecord(buf, size);
    }
    page_store_.WritePage(page_id, page.GetData());
}

void BPlusTree::WriteInternal(int page_id, const std::vector<Entry>& entries, int leftmost_child) {
    Page page(page_id);
    page.SetNextPageId(leftmost_child); // reutilizado como "hijo mas izquierdo"

    Record meta;
    meta.AddInt(0); // 0 = interno
    char meta_buf[PAGE_SIZE];
    int meta_size = meta.Serialize(meta_buf);
    page.InsertRecord(meta_buf, meta_size); // slot 0

    for (const Entry& e : entries) {
        Record r;
        r.AddInt(e.key);
        r.AddInt(e.a); // right_child_page_id
        char buf[PAGE_SIZE];
        int size = r.Serialize(buf);
        page.InsertRecord(buf, size);
    }
    page_store_.WritePage(page_id, page.GetData());
}

int BPlusTree::CreateEmptyLeaf() {
    int page_id = page_store_.AllocatePage();
    WriteLeaf(page_id, {}, /*next_leaf_id=*/-1);
    return page_id;
}

int BPlusTree::CreateEmptyInternal(int leftmost_child) {
    int page_id = page_store_.AllocatePage();
    WriteInternal(page_id, {}, leftmost_child);
    return page_id;
}

// ---------------------------------------------------------------------
// Busqueda
// ---------------------------------------------------------------------

bool BPlusTree::Search(int key, std::vector<RID>& out_rids) const {
    int current = root_page_id_;
    while (!IsLeaf(current)) {
        std::vector<Entry> entries = ReadEntries(current); // ordenadas por key
        int child = ReadLeftmostOrNext(current);
        for (const Entry& e : entries) {
            if (key < e.key) break;
            child = e.a; // avanzar al hijo derecho de esta clave
        }
        current = child;
    }

    std::vector<Entry> leaf_entries = ReadEntries(current);
    bool found = false;
    for (const Entry& e : leaf_entries) {
        if (e.key == key) {
            out_rids.push_back(RID{e.a, e.b});
            found = true;
        }
    }
    return found;
}

// ---------------------------------------------------------------------
// Insercion
// ---------------------------------------------------------------------

void BPlusTree::Insert(int key, const RID& rid) {
    SplitResult result = InsertRecursive(root_page_id_, key, rid);
    if (result.split) {
        // La raiz se dividio: crear una nueva raiz interna.
        int new_root = CreateEmptyInternal(/*leftmost_child=*/root_page_id_);
        WriteInternal(new_root, {Entry{result.promoted_key, result.new_right_page_id, -1}},
                       root_page_id_);
        root_page_id_ = new_root;
    }
}

BPlusTree::SplitResult BPlusTree::InsertRecursive(int page_id, int key, const RID& rid) {
    if (IsLeaf(page_id)) {
        std::vector<Entry> entries = ReadEntries(page_id);
        entries.push_back(Entry{key, rid.page_id, rid.slot_id});
        std::sort(entries.begin(), entries.end(),
                  [](const Entry& x, const Entry& y) { return x.key < y.key; });

        int next_leaf = ReadLeftmostOrNext(page_id);

        if (static_cast<int>(entries.size()) <= max_entries_) {
            WriteLeaf(page_id, entries, next_leaf);
            return SplitResult{}; // sin division
        }

        // Division de hoja: mitad izquierda se queda, mitad derecha a pagina nueva.
        size_t mid = entries.size() / 2;
        std::vector<Entry> left(entries.begin(), entries.begin() + mid);
        std::vector<Entry> right(entries.begin() + mid, entries.end());

        int right_page_id = page_store_.AllocatePage();
        WriteLeaf(right_page_id, right, next_leaf);
        WriteLeaf(page_id, left, right_page_id); // encadenar hoja izquierda -> derecha

        SplitResult res;
        res.split = true;
        res.promoted_key = right.front().key; // primera clave de la hoja derecha
        res.new_right_page_id = right_page_id;
        return res;
    }

    // Nodo interno: encontrar el hijo correspondiente y descender.
    std::vector<Entry> entries = ReadEntries(page_id);
    int leftmost_child = ReadLeftmostOrNext(page_id);

    int child = leftmost_child;
    size_t child_entry_index = entries.size(); // indice del entry cuyo hijo derecho tomamos
    for (size_t i = 0; i < entries.size(); i++) {
        if (key < entries[i].key) {
            child_entry_index = i;
            break;
        }
        child = entries[i].a;
    }
    (void)child_entry_index;

    SplitResult child_result = InsertRecursive(child, key, rid);
    if (!child_result.split) {
        return SplitResult{}; // el hijo no se dividio, no hay nada que propagar
    }

    // Insertar (promoted_key -> new_right_page_id) en este nodo interno.
    entries.push_back(Entry{child_result.promoted_key, child_result.new_right_page_id, -1});
    std::sort(entries.begin(), entries.end(),
              [](const Entry& x, const Entry& y) { return x.key < y.key; });

    if (static_cast<int>(entries.size()) <= max_entries_) {
        WriteInternal(page_id, entries, leftmost_child);
        return SplitResult{};
    }

    // Division de nodo interno: la clave del medio se PROMUEVE (no se
    // duplica en ningun hijo), el resto se reparte entre izquierda/derecha.
    size_t mid = entries.size() / 2;
    int promoted_key = entries[mid].key;
    int right_leftmost_child = entries[mid].a;

    std::vector<Entry> left(entries.begin(), entries.begin() + mid);
    std::vector<Entry> right(entries.begin() + mid + 1, entries.end());

    int right_page_id = page_store_.AllocatePage();
    WriteInternal(right_page_id, right, right_leftmost_child);
    WriteInternal(page_id, left, leftmost_child);

    SplitResult res;
    res.split = true;
    res.promoted_key = promoted_key;
    res.new_right_page_id = right_page_id;
    return res;
}

// ---------------------------------------------------------------------
// Utilidad de depuracion
// ---------------------------------------------------------------------

void BPlusTree::PrintLeaves() const {
    int current = root_page_id_;
    while (!IsLeaf(current)) {
        current = ReadLeftmostOrNext(current); // bajar siempre por el hijo mas izquierdo
    }

    std::cout << "Hojas del B+Tree (en orden, via encadenamiento de hojas):\n";
    while (current != -1) {
        std::vector<Entry> entries = ReadEntries(current);
        std::cout << "  Pagina " << current << ": ";
        for (const Entry& e : entries) {
            std::cout << "[" << e.key << " -> RID(" << e.a << "," << e.b << ")] ";
        }
        std::cout << "\n";
        current = ReadLeftmostOrNext(current);
    }
}
