#include "job_system.h"

#include <iostream>
#include <chrono>
#include <string>
#include <vector>

void print(const char* msg) {
    std::cout << msg << " thread id=" << std::this_thread::get_id() << std::endl;
}

void simpleThread() {
    print("Simple thread");
    std::thread thread(print, "Simple thread");
    thread.join();
    print("Simple thread");
}

class Callable final {
public:
    void operator()() const {
        print("Background task in");
    }
};

void callableThread() {
    std::thread callableThread { Callable() };
    callableThread.detach();
}

struct Func {
    int& i;

    Func(int& _i) : i(_i) {}

    void operator()() {
        for (int j = 0 ; j < 100 ; j++) {
            print("Func()");
            std::cout << i++ << std::endl;
        }
    }
};

class ThreadGuard final {

public:
    explicit ThreadGuard(std::thread& thread) : m_Thread(thread) {}
    ~ThreadGuard() {
        if (m_Thread.joinable()) {
            m_Thread.join();
        }
    }

    // not copyable
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

private:
    std::thread& m_Thread;
};

class ScopeThread final {

public:
    explicit ScopeThread(std::thread thread) : m_Thread(std::move(thread)) {
        if (!m_Thread.joinable()) {
            throw std::logic_error("Unavailable thread in scope!");
        }
    }

    ~ScopeThread() {
        m_Thread.join();
    }

    ScopeThread(const ScopeThread&) = delete;
    ScopeThread& operator=(const ScopeThread&) = delete;

private:
    std::thread m_Thread;
};

std::vector<int> values;
std::mutex mutexValues;

void addValue(int value) {
    std::lock_guard<std::mutex> lock(mutexValues);
    values.emplace_back(value);
}

bool hasValue(int value) {
    std::lock_guard<std::mutex> lock(mutexValues);
    return std::find(values.begin(), values.end(), value) != values.end();
}

using namespace std;

void Spin(float milliseconds) {
    milliseconds /= 1000.0f;
    chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
    double ms = 0;
    while (ms < milliseconds)
    {
        chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
        chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
        ms = time_span.count();
    }
}

struct timer {
    string name;
    chrono::high_resolution_clock::time_point start;

    explicit timer(const string& name) : name(name), start(chrono::high_resolution_clock::now()) {}
    ~timer() {
        auto end = chrono::high_resolution_clock::now();
        cout << name << ": " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " milliseconds" << endl;
    }
};

int main() {
    auto& jobSystem = engine::core::JobSystem::get();

    {
        auto t = timer("Single thread test ");
        Spin(100);
        Spin(100);
        Spin(100);
        Spin(100);
    }

    {
        int cores = 1;
        auto t = timer("Jobs single execution test ");
        for (int i = 0 ; i < cores * 2 ; i++) {
            jobSystem.execute([] { Spin(100); });
        }
        jobSystem.wait();
    }

    struct Data {
        float m[16];
        void update(u32 value) {
            for (int i = 0; i < 16; ++i) {
                m[i] += float(value + i);
            }
        }
    };
    u32 dataCount = 100000000; // 100 million iterations
    Data* dataSet = new Data[dataCount];

    {
        auto t = timer("Single thread test ");
        for (uint32_t i = 0; i < dataCount; ++i) {
            dataSet[i].update(i);
        }
    }

    {
        auto t = timer("Jobs multi execution test ");
        const uint32_t groupSize = 1000;
        jobSystem.execute(dataCount, groupSize, [&dataSet](engine::core::JobArgs args) {
            dataSet[args.index].update(1);
        });
        jobSystem.wait();
    }

    delete[] dataSet;

    return 0;
}
