#ifndef TDF_WRITER_SYNC_BUFFER_HPP
#define TDF_WRITER_SYNC_BUFFER_HPP

#include <deque>
#include "sync_bouded_container.hpp"
// SynchronizedBuffer: Thread-safe, bounded FIFO buffer for producer-consumer scenarios.
//
// Template parameter:
//   T - Type of elements stored in the buffer.
//
// Usage:
//   - Construct with a maximum size.
//   - Use push() to add items (blocks if full).
//   - Use pop() to retrieve items (blocks if empty, returns std::nullopt if closed and empty).
//   - Call close() to signal no more items will be added.
//
// Thread safety:
//   - All public methods are thread-safe.
//   - Condition variables are used for efficient waiting.
//
// Example:
//   SynchronizedBuffer<int> buf(10);
//   buf.push(42);
//   auto item = buf.pop();

template <typename T>
class SynchronizedBuffer : public SyncBoundedContainer<T>
{
    std::deque<T> buffer;
    size_t max_size;
protected:
    inline void insert_into_container(T&& item) override
    {
        buffer.push_back(std::move(item));
    }
    inline T remove_from_container() override
    {
        T item = std::move(buffer.front());
        buffer.pop_front();
        return item;
    }
    inline bool container_can_accept(const T&) const override
    {
        return buffer.size() < max_size;
    }
    inline bool container_can_yield() const override
    {
        return !buffer.empty();
    }
    inline bool container_is_empty() const override
    {
        return buffer.empty();
    }
public:
    explicit SynchronizedBuffer(size_t max_size_) : max_size(max_size_) {}
};

#endif // TDF_WRITER_SYNC_BUFFER_HPP