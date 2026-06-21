#pragma once

class McCompactOutQueue {
   public:
    static constexpr size_t MAX_ENTRIES = 15;

    // Add entry to queue, returns true if successful, false if full
    bool push(const McPacket_t& entry, bool priority = false) {
        std::unique_lock<std::mutex> lock(mtx);
        if (queue.size() >= MAX_ENTRIES && !priority) {
            return false;
        }
        if (!priority) {
            queue.push_back(entry);
        } else {
            queue.push_front(entry);
            while (queue.size() > MAX_ENTRIES) {
                queue.pop_back();
            }
        }
        cv.notify_one();
        return true;
    }

    // Remove and get entry from queue, blocks if empty
    McPacket_t pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return (!queue.empty() || stopFlag); });
        McPacket_t entry;
        entry.length = 303;
        if (stopFlag) {
            // Return a default-constructed (empty) entry if stopFlag is set
            return entry;
        }
        entry = queue.front();
        queue.pop_front();
        return entry;
    }

    // Notify one waiting thread to unblock pop() immediately (even if queue is empty)
    void stop_wait() {
        stopFlag = true;
        cv.notify_one();
    }

    // Try to remove and get entry from queue, returns true if successful
    bool try_pop(McPacket_t& entry) {
        std::unique_lock<std::mutex> lock(mtx);
        if (queue.empty()) return false;
        entry = queue.front();
        queue.pop_front();
        return true;
    }

    // Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    // Get current size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

   private:
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::deque<McPacket_t> queue;
    bool stopFlag = false;
};