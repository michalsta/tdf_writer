#include <semaphore>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <tuple>
#include <vector>
#include <optional>


// A thread-safe priority queue that allows pushing items with an associated index
// and popping them in order of their indices, guaranteeing that items are retrieved
// in the order of their indices, without skipping any indices.
// Pop blocks if the next expected index is not available yet.

template <typename T>
class OrderedQueue
{
    std::mutex mtx;
    std::condition_variable cv;
    using QueueItem = std::pair<size_t, T>; // (index, item)
    struct Compare {
        bool operator()(const QueueItem& a, const QueueItem& b) {
            return std::get<0>(a) > std::get<0>(b); // Min-heap based on index
        }
    };
    std::priority_queue<QueueItem, std::vector<QueueItem>, Compare> pq;
    size_t next_index = 0; // Next expected index to pop
    bool finished = false;
public:

    OrderedQueue() = default;
    ~OrderedQueue() { close(); }

    void push(size_t idx, const T& item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        pq.emplace(std::make_tuple(idx, item));
        cv.notify_all();
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return (!pq.empty() && pq.top().first == next_index) || finished; });
        if(finished && (pq.empty())) return std::nullopt;
        T item = std::get<1>(pq.top());
        pq.pop();
        ++next_index;
        cv.notify_all();
        return item;
    }

    // Closes the queue. Wakes up all waiting threads. Wait until all items are popped.
    void close()
    {
        std::unique_lock<std::mutex> lock(mtx);
        finished = true;
        cv.notify_all();
        cv.wait(lock, [this]() { return pq.empty(); });
    }

    bool is_closed() const
    {
        std::unique_lock<std::mutex> lock(mtx);
        return finished;
    }
};



