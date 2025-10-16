#ifndef TDF_WRITER_ORDERED_QUEUE_HPP
#define TDF_WRITER_ORDERED_QUEUE_HPP

#include <queue>
#include "sync_bouded_container.hpp"

// A thread-safe priority queue that allows pushing items with an associated index
// and popping them in order of their indices, guaranteeing that items are retrieved
// in the order of their indices, without skipping any indices.
// Pop blocks if the next expected index is not available yet.

template <typename T>
class OrderedQueue : public SyncBoundedContainer<std::pair<size_t, T>>
{
    struct Compare {
        bool operator()(const std::pair<size_t, T>& a, const std::pair<size_t, T>& b) const {
            return a.first > b.first; // Min-heap based on the index
        }
    };

    std::priority_queue<std::pair<size_t, T>, std::vector<std::pair<size_t, T>>, Compare> pq;
    size_t next_index = 0;
    size_t max_size;
protected:
    inline void insert_into_container(std::pair<size_t, T>&& item) override
    {
        pq.push(std::move(item));
    }
    inline std::pair<size_t, T> remove_from_container() override
    {
        std::pair<size_t, T> item = std::move(pq.top());
        pq.pop();
        return item;
    }
    inline bool container_is_full() const override
    {
        return pq.size() >= max_size;
    }
    inline bool container_is_empty() const override
    {
        return pq.empty() || pq.top().first != next_index;
    }
};

#endif // TDF_WRITER_ORDERED_QUEUE_HPP