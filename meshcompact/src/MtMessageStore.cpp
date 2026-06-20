#include "MtMessageStore.hpp"

MtMessageStore::MtMessageStore(std::size_t cacheSize)
    : cacheSize_(cacheSize) {
}

void MtMessageStore::addListener(MessageCallback cb) {
    listeners_.push_back(std::move(cb));
}

void MtMessageStore::addMessage(MessageEntry entry) {
    // entry is moved into the deque (PSRAM)
    messages_.push_back(std::move(entry));
    while (messages_.size() > cacheSize_) {
        messages_.pop_front();
    }
    MessageEntry& stored = messages_.back();
    for (auto& listener : listeners_) {
        if (listener) listener(stored);
    }
}

// Option A: Visitor Pattern (Safest, controlled scope)
void MtMessageStore::traverseHistory(VisitorCallback visitor) const {
    if (!visitor) return;

    for (const auto& entry : messages_) {
        bool continueIterating = visitor(entry);
        if (!continueIterating) break;
    }
}

// Option B: Standard Iterators
MtMessageStore::ConstIterator MtMessageStore::begin() const {
    return messages_.begin();
}

MtMessageStore::ConstIterator MtMessageStore::end() const {
    return messages_.end();
}

void MtMessageStore::setCacheSize(std::size_t size) {
    cacheSize_ = size;
    while (messages_.size() > cacheSize_) {
        messages_.pop_front();
    }
}