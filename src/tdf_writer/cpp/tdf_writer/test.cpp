#include "dispatcher.hpp"
#include "file_collector.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <random>
#include <cassert>



class simpleTestDispatcher
{
public:
    class simpleMapper : public Mapper<int, SimpleBuffer<char>>
    {
    public:
        SimpleBuffer<char> map(const int& input) override
        {
            auto sleep_duration = std::chrono::milliseconds(rand() % 100);
            std::cout << "Mapping: " << input << " (sleep " << sleep_duration.count() << " ms)" << std::endl;
            std::this_thread::sleep_for(sleep_duration);
            std::vector<char> output;
            output.push_back(static_cast<char>(input % 256));
            SimpleBuffer<char> buffer(output.data(), output.size());
            return std::move(buffer);
        }
    };

    /*class simpleReducer : public Reducer<int>
    {
        int counter = 0;
    public:
        void reduce(const int& input) override
        {
            assert(input == counter++);
            std::cout << "Reduced: " << input << std::endl;
        }
    };*/

    void run()
    {
        auto mapper = std::make_unique<simpleMapper>();
        auto reducer = std::make_unique<FileCollector>("output.bin");

        Dispatcher<simpleMapper, FileCollector> dispatcher(std::move(mapper), std::move(reducer), 10, 100);


        for(int i = 0; i < 1000; ++i) {
            std::cout << "Adding input: " << i << std::endl;
            dispatcher.add_input(i);
        }

        dispatcher.close();
    }
};

int main()
{
    simpleTestDispatcher test;
    test.run();
    return 0;
}