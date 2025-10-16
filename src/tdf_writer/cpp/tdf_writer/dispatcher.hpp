#ifndef TDF_WRITER_DISPATCHER_HPP
#define TDF_WRITER_DISPATCHER_HPP

#include <semaphore>
#include <mutex>
#include <thread>
#include <queue>
#include <tuple>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>

#include "sync_buffer.hpp"


template <typename Input_t, typename Output_t>
class Mapper
{

public:
    using InputType = Input_t;
    using OutputType = Output_t;

    virtual Output_t map(const Input_t& input) = 0;
    virtual ~Mapper() = default;
};

template <typename Input_t>
class Reducer
{
public:
    using InputType = Input_t;

    virtual void reduce(const Input_t& input) = 0;
    virtual ~Reducer() = default;
};


template <typename Mapper_t, typename Reducer_t>
class Dispatcher
{
public:
    using MapperType = Mapper_t;
    using ReducerType = Reducer_t;
    using InputType = typename Mapper_t::InputType;
    using IntermediateType = typename Mapper_t::OutputType;
    static_assert(std::is_same<IntermediateType, typename Reducer_t::InputType>::value,
                  "Mapper output type must match Reducer input type");

    Dispatcher(std::unique_ptr<Mapper_t> mapper_,
               std::unique_ptr<Reducer_t> reducer_,
               size_t input_buffer_size = std::thread::hardware_concurrency()+1,
               size_t num_mapper_threads = std::thread::hardware_concurrency())
        : input_buffer(input_buffer_size),
            intermediate_queue(),
            mapper(std::move(mapper_)),
            reducer(std::move(reducer_))
    {
        if(!mapper) {
            throw std::invalid_argument("Mapper cannot be null");
        }
        if(!reducer) {
            throw std::invalid_argument("Reducer cannot be null");
        }
        if(num_mapper_threads == 0) {
            throw std::invalid_argument("Number of mapper threads must be greater than zero");
        }

        // Start mapper threads
        for(size_t i = 0; i < num_mapper_threads; ++i) {
            mapper_threads.emplace_back([this]() {
                while(true) {
                    auto item = input_buffer.pop();
                    if(!item.has_value()) break; // Buffer closed and empty
                    auto& [idx, input] = item.value();
                    IntermediateType intermediate = mapper->map(input);
                    intermediate_queue.push({idx, std::move(intermediate)});
                }
            });
        }

        // Start reducer thread
        reducer_thread = std::thread([this]() {
            while(true) {
                auto item = intermediate_queue.pop();
                if(!item.has_value()) break; // Queue closed and empty
                reducer->reduce(item.value().second);
            }
        });
    }


    void add_input(const InputType& input)
    {
        if(input_buffer.is_closed()) {
            throw std::runtime_error("Cannot add input to closed dispatcher");
        }
        input_buffer.push(std::make_pair(input, next_job_index++));
    }

    void close()
    {
        input_buffer.close();
        for(auto& t : mapper_threads) {
            if(t.joinable()) t.join();
        }
        intermediate_queue.close();
        if(reducer_thread.joinable()) {
            reducer_thread.join();
        }
    }

private:
    SynchronizedBuffer<std::pair<size_t,InputType>> input_buffer;
    SyncBoundedPriorityQueue<IntermediateType> intermediate_queue;
    std::unique_ptr<Mapper_t> mapper;
    std::unique_ptr<Reducer_t> reducer;
    std::vector<std::thread> mapper_threads;
    std::thread reducer_thread;
    size_t next_job_index = 0;
};

#endif // TDF_WRITER_DISPATCHER_HPP