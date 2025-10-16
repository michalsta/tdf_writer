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
class QueueElement
{
public:
    size_t index;
    T value;
    QueueElement(size_t idx, T&& val) : index(idx), value(std::move(val)) {}
    QueueElement(const QueueElement&) = delete;
    QueueElement& operator=(const QueueElement&) = delete;
    QueueElement(QueueElement&& other) noexcept : index(other.index), value(std::move(other.value)) {}
    QueueElement& operator=(QueueElement&& other) noexcept
    {
        if(this != &other) {
            index = other.index;
            value = std::move(other.value);
        }
        return *this;
    }
    ~QueueElement() = default;
    // Comparison operator for priority queue (min-heap by index)
    bool operator>(const QueueElement<T>& other) const {
        return index > other.index;
    }
};

// A thread-safe priority queue that stores pairs of (index, value).
// Items are pushed with an associated index and popped strictly in order of increasing index.
// The queue blocks on pop if the next expected index is not available, ensuring no indices are skipped.
template <typename T>
class SyncBoundedPriorityQueue : public SyncBoundedContainer<QueueElement<T>>
{
    std::priority_queue<QueueElement<T>, std::vector<QueueElement<T>>, std::greater<QueueElement<T>>> pq;
    size_t next_index = 0;
    size_t max_size;

protected:
    inline virtual void insert_into_container(std::pair<size_t, T>&& item) override
    {
        pq.push(QueueElement<T>(item.first, std::move(item.second)));
    }
    inline virtual QueueElement<T> remove_from_container() override
    {
        QueueElement<T> elem = std::move(pq.top());
        pq.pop();
        assert(elem.index == next_index);
        ++next_index;
        return {elem.index, std::move(elem.value)};
    }
    inline virtual bool container_can_accept(const QueueElement& item) const override
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