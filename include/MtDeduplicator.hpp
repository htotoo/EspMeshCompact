#ifndef MTDEDUPLICATOR_HPP
#define MTDEDUPLICATOR_HPP

#include <stdint.h>
#include <unordered_set>
#include <deque>
#include <mutex>

class MtDeduplicator {
   public:
    bool check(uint32_t msgid) {  // return true when not a dupe
        std::lock_guard<std::mutex> lock(mutex_);
        if (msgids_.find(msgid) != msgids_.end()) {
            return true;
        }
        if (msgids_.size() >= limit) {
            // Remove the oldest inserted msgid
            msgids_order_.pop_front();
            msgids_.erase(msgids_order_.front());
        }
        msgids_order_.push_back(msgid);
        msgids_.insert(msgid);
        return false;
    }

    void setLimit(uint16_t newLimit) {
        limit = newLimit;  // will apply on new msg
    }

   private:
    uint16_t limit = 50;
    std::unordered_set<uint32_t> msgids_;
    std::deque<uint32_t> msgids_order_;
    std::mutex mutex_;
};

#endif  // MTDEDUPLICATOR_HPP