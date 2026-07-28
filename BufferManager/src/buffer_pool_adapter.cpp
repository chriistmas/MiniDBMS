#include "buffer_pool_adapter.h"
#include "page.h"
#include <cstring>

BufferPoolAdapter::BufferPoolAdapter(BufferPoolManager& bpm) : bpm_(bpm) {}

void BufferPoolAdapter::ReadPage(int page_id, char* page_data) {
    Page* page = bpm_.FetchPage(page_id);
    std::memcpy(page_data, page->GetData(), PAGE_SIZE);
    bpm_.UnpinPage(page_id, /*is_dirty=*/false);
}

void BufferPoolAdapter::WritePage(int page_id, const char* page_data) {
    Page* page = bpm_.FetchPage(page_id);
    std::memcpy(page->GetData(), page_data, PAGE_SIZE);
    bpm_.UnpinPage(page_id, /*is_dirty=*/true);
}

int BufferPoolAdapter::AllocatePage() {
    int page_id = -1;
    Page* page = bpm_.NewPage(&page_id);
    if (page != nullptr) {
        bpm_.UnpinPage(page_id, /*is_dirty=*/true);
    }
    return page_id;
}

void BufferPoolAdapter::DeallocatePage(int page_id) {
    bpm_.DeletePage(page_id);
}

int BufferPoolAdapter::GetNumPages() const {
    return bpm_.GetNumPages();
}
