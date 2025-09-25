#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>


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
class SynchronizedBuffer
{
    std::deque<T> buffer;                  // Underlying container for items.
    mutable std::mutex mtx;                // Mutex for synchronizing access (mutable for const methods).
    std::condition_variable cv;            // Condition variable for blocking/waking threads.
    bool closed = false;                   // Indicates if buffer is closed for new items.
    const size_t max_size;                 // Maximum number of items buffer can hold.

public:
    // Constructor: sets the buffer's maximum size.
    SynchronizedBuffer(size_t max_size_) : max_size(max_size_) {
        if (max_size_ == 0) {
            throw std::invalid_argument("SynchronizedBuffer: max_size must be greater than zero");
        }
    }

    // Adds an item to the buffer.
    // Throws std::runtime_error if called after the buffer is closed.
    void push(const T& item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return buffer.size() < max_size || closed; });
        if (closed) throw std::runtime_error("Push to closed buffer");
        buffer.push_back(item);
        cv.notify_all();
    }

    // Removes and returns the front item from the buffer.
    // Blocks if buffer is empty and not closed.
    // Returns std::nullopt if buffer is empty and closed.
    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !buffer.empty() || closed; });
        if(buffer.empty() && closed) return std::nullopt;
        T item = buffer.front();
        buffer.pop_front();
        cv.notify_all();
        return item;
    }

    // Closes the buffer.
    // Wakes up all waiting threads.
    // Waits until the buffer is emptied before returning.
    void close()
    {
        std::unique_lock<std::mutex> lock(mtx);
        closed = true;
        cv.notify_all();
        cv.wait(lock, [this]() { return buffer.empty(); });
    }

    bool is_closed() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return closed;
    }

    // Destructor: does not call close().
    // Users must call close() before destruction to avoid undefined behavior.
    ~SynchronizedBuffer() = default;
};