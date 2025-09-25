#include "dispatcher.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <random>
#include <cassert>



class simpleTestDispatcher
{
public:
    class simpleMapper : public Mapper<int, int>
    {
    public:
        int map(const int& input) override
        {
            auto sleep_duration = std::chrono::milliseconds(rand() % 100);
            std::cout << "Mapping: " << input << " (sleep " << sleep_duration.count() << " ms)" << std::endl;
            std::this_thread::sleep_for(sleep_duration);
            return input;
        }
    };

    class simpleReducer : public Reducer<int>
    {
        int counter = 0;
    public:
        void reduce(const int& input) override
        {
            assert(input == counter++);
            std::cout << "Reduced: " << input << std::endl;
        }
    };

    void run()
    {
        auto mapper = std::make_unique<simpleMapper>();
        auto reducer = std::make_unique<simpleReducer>();

        Dispatcher<simpleMapper, simpleReducer> dispatcher(std::move(mapper), std::move(reducer), 10, 100);


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