#ifndef TDF_WRITER_SYNC_BOUNDED_CONTAINER_HPP
#define TDF_WRITER_SYNC_BOUNDED_CONTAINER_HPP

#include <mutex>
#include <condition_variable>
#include <queue>
#include <limits>
#include <optional>
#include <stdexcept>
#include <cassert>


// A thread-safe bounded container.
template <typename T>
class SyncBoundedContainer
{
    std::mutex mtx;
    std::condition_variable cv_can_accept;
    std::condition_variable cv_can_remove;
    bool finished = false;

protected:
    virtual void insert_into_container(T&& item) = 0;
    virtual T remove_from_container() = 0;
    virtual bool container_can_accept(const T&) const = 0;
    virtual bool container_can_yield() const = 0;
    virtual bool container_is_empty() const = 0;

public:
    SyncBoundedContainer() = default;
    virtual ~SyncBoundedContainer() = default;

    SyncBoundedContainer(const SyncBoundedContainer&) = delete;
    SyncBoundedContainer& operator=(const SyncBoundedContainer&) = delete;
    SyncBoundedContainer(SyncBoundedContainer&&) = delete;
    SyncBoundedContainer& operator=(SyncBoundedContainer&&) = delete;

    void push(T&& item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv_can_accept.wait(lock, [this, &item]() { return container_can_accept(item) || finished; });
        if(finished) {
            throw std::runtime_error("Push to a closed container");
        }
        insert_into_container(std::move(item));
        cv_can_remove.notify_one();
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv_can_remove.wait(lock, [this]() { return container_can_yield() || finished; });
        if(finished && container_is_empty()) {
            return std::nullopt;
        }
        T item = remove_from_container();
        cv_can_accept.notify_one();
        return std::move(item);
    }

    void close()
    {
        std::unique_lock<std::mutex> lock(mtx);
        finished = true;
        cv_can_accept.notify_all();
        cv_can_remove.notify_all();
    }

    bool is_closed()
    {
        std::unique_lock<std::mutex> lock(mtx);
        return finished;
    }
};


template <typename T>
class PriorityQueue
{
    struct ComparePair {
        bool operator()(const std::pair<size_t, T>& a, const std::pair<size_t, T>& b) const
        {
            return a.first > b.first; // Min-heap based on the first element (priority)
        }
    };
    std::priority_queue<std::pair<size_t, T>,
                        std::vector<std::pair<size_t, T>>,
                        ComparePair> pq;
public:
    PriorityQueue() = default;
    ~PriorityQueue() = default;
    void push(size_t priority, T&& item)
    {
        std::pair<size_t, T> p(priority, std::move(item));
        pq.push(std::move(p));
    }
    std::pair<size_t, T> pop()
    {
        auto item = std::move(const_cast<std::pair<size_t, T>&>(pq.top()));
        pq.pop();
        return item;
    }
    const std::pair<size_t, T>& top() const
    {
        return pq.top();
    }
    bool empty() const
    {
        return pq.empty();
    }
    size_t size() const
    {
        return pq.size();
    }
};


// A thread-safe priority queue that stores pairs of (index, value).
// Items are pushed with an associated index and popped strictly in order of increasing index.
// The queue blocks on pop if the next expected index is not available, ensuring no indices are skipped.
template <typename T>
class SyncBoundedPriorityQueue : public SyncBoundedContainer<std::pair<size_t, T>>
{
    using QueueElement = std::pair<size_t, T>;
    PriorityQueue<T> pq;
    size_t next_index = 0;
    size_t max_size;

protected:
    inline virtual void insert_into_container(std::pair<size_t, T>&& item) override
    {
        pq.push(item.first, std::move(item.second));
    }
    inline virtual std::pair<size_t, T> remove_from_container() override
    {
        assert(pq.top().first == next_index);
        ++next_index;
        return pq.pop();
    }
    inline virtual bool container_can_accept(const std::pair<size_t, T>& item) const override
    {
        return pq.size() < max_size || item.first < pq.top().first;
    }
    inline virtual bool container_can_yield() const override
    {
        return (!pq.empty()) && (pq.top().first == next_index);
    }
    inline virtual bool container_is_empty() const override
    {
        return pq.empty();
    }
public:
    explicit SyncBoundedPriorityQueue(size_t max_size_ = std::numeric_limits<size_t>::max()) : max_size(max_size_) {}
    ~SyncBoundedPriorityQueue() override = default;
};

#endif // TDF_WRITER_SYNC_BOUNDED_CONTAINER_HPP