#include "lru_replacer.h"

LRUReplacer::LRUReplacer(size_t num_frames) : capacity_(num_frames) {}

bool LRUReplacer::Victim(int* frame_id) {
    std::lock_guard<std::mutex> lock(latch_);
    if (lru_list_.empty()) {
        return false;
    }
    *frame_id = lru_list_.front();
    lru_list_.pop_front();
    position_.erase(*frame_id);
    return true;
}

void LRUReplacer::Pin(int frame_id) {
    std::lock_guard<std::mutex> lock(latch_);
    auto it = position_.find(frame_id);
    if (it != position_.end()) {
        lru_list_.erase(it->second);
        position_.erase(it);
    }
}

void LRUReplacer::Unpin(int frame_id) {
    std::lock_guard<std::mutex> lock(latch_);
    if (position_.find(frame_id) != position_.end()) {
        return; // ya esta en la lista de candidatos
    }
    lru_list_.push_back(frame_id);
    position_[frame_id] = std::prev(lru_list_.end());
}

size_t LRUReplacer::Size() const {
    std::lock_guard<std::mutex> lock(latch_);
    return lru_list_.size();
}
