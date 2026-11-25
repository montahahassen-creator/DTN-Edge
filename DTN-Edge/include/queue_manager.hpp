#pragma once

#include "storage.hpp"
#include <vector>

class QueueManager {
public:
    explicit QueueManager(Storage& storage) : storage_(storage) {}

    std::vector<Message> next_batch(int max_items = 5) {
        return storage_.pending(max_items);
    }

private:
    Storage& storage_;
};
