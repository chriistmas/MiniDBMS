#include "buffer_pool_manager.h"
#include <cstring>

BufferPoolManager::BufferPoolManager(DiskManager& disk_manager, size_t pool_size)
    : disk_manager_(disk_manager),
      pool_size_(pool_size),
      frames_(pool_size),
      replacer_(pool_size) {
    for (size_t i = 0; i < pool_size_; i++) {
        free_list_.push_back(static_cast<int>(i));
    }
}

BufferPoolManager::~BufferPoolManager() {
    FlushAllPages();
}

int BufferPoolManager::FindAvailableFrame() {
    if (!free_list_.empty()) {
        int frame_id = free_list_.front();
        free_list_.pop_front();
        return frame_id;
    }

    int victim_frame_id;
    if (!replacer_.Victim(&victim_frame_id)) {
        return -1; // Pool lleno y todas las paginas estan pineadas
    }

    Frame& victim = frames_[victim_frame_id];
    if (victim.is_dirty) {
        disk_manager_.WritePage(victim.page.GetPageId(), victim.page.GetData());
        victim.is_dirty = false;
    }
    page_table_.erase(victim.page.GetPageId());
    victim.in_use = false;
    return victim_frame_id;
}

Page* BufferPoolManager::FetchPage(int page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        // HIT: la pagina ya esta en RAM
        Frame& frame = frames_[it->second];
        if (frame.pin_count == 0) {
            replacer_.Pin(it->second); // ya no es candidata a reemplazo
        }
        frame.pin_count++;
        hit_count_++;
        return &frame.page;
    }

    // MISS: hay que traerla del disco
    int frame_id = FindAvailableFrame();
    if (frame_id == -1) {
        return nullptr;
    }

    Frame& frame = frames_[frame_id];
    disk_manager_.ReadPage(page_id, frame.page.GetData());
    frame.page.SetPageId(page_id);
    frame.pin_count = 1;
    frame.is_dirty = false;
    frame.in_use = true;

    page_table_[page_id] = frame_id;
    miss_count_++;
    return &frame.page;
}

bool BufferPoolManager::UnpinPage(int page_id, bool is_dirty) {
    std::lock_guard<std::mutex> lock(latch_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;
    }

    Frame& frame = frames_[it->second];
    if (frame.pin_count <= 0) {
        return false;
    }

    if (is_dirty) {
        frame.is_dirty = true;
    }

    frame.pin_count--;
    if (frame.pin_count == 0) {
        replacer_.Unpin(it->second); // ahora si es candidata a reemplazo
    }
    return true;
}

bool BufferPoolManager::FlushPage(int page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;
    }

    Frame& frame = frames_[it->second];
    disk_manager_.WritePage(page_id, frame.page.GetData());
    frame.is_dirty = false;
    return true;
}

void BufferPoolManager::FlushAllPages() {
    std::lock_guard<std::mutex> lock(latch_);
    for (auto& entry : page_table_) {
        Frame& frame = frames_[entry.second];
        if (frame.is_dirty) {
            disk_manager_.WritePage(entry.first, frame.page.GetData());
            frame.is_dirty = false;
        }
    }
}

Page* BufferPoolManager::NewPage(int* page_id_out) {
    std::lock_guard<std::mutex> lock(latch_);

    int frame_id = FindAvailableFrame();
    if (frame_id == -1) {
        *page_id_out = -1;
        return nullptr;
    }

    int new_page_id = disk_manager_.AllocatePage();

    Frame& frame = frames_[frame_id];
    frame.page = Page(new_page_id); // pagina vacia, inicializada en memoria
    frame.pin_count = 1;
    frame.is_dirty = true; // aun no existe en disco con este contenido
    frame.in_use = true;

    page_table_[new_page_id] = frame_id;
    *page_id_out = new_page_id;
    return &frame.page;
}

bool BufferPoolManager::DeletePage(int page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        Frame& frame = frames_[it->second];
        if (frame.pin_count > 0) {
            return false; // esta en uso, no se puede eliminar
        }
        replacer_.Pin(it->second); // sacarla de la lista de candidatos
        frame.in_use = false;
        frame.is_dirty = false;
        page_table_.erase(it);
        free_list_.push_back(it->second);
    }

    disk_manager_.DeallocatePage(page_id);
    return true;
}

size_t BufferPoolManager::GetPoolSize() const { return pool_size_; }

int BufferPoolManager::GetNumPages() const { return disk_manager_.GetNumPages(); }
