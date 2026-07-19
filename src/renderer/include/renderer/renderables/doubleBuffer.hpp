#pragma once

#include <atomic>
#include <array>
#include <thread>

namespace renderer {

    /**
     * Wrapper for any datatype, such that the data can be read and updated in parallel, while keeping these versions in sync.
     */
    template<typename T>
    class DoubleBuffer {
    public:
        DoubleBuffer() : frontIdx(0), dataReady(false), writing(false) {}

        DoubleBuffer(const DoubleBuffer&) = delete;
        DoubleBuffer& operator=(const DoubleBuffer&) = delete;

        DoubleBuffer(DoubleBuffer&& other) noexcept : 
            buffers(std::move(other.buffers)),
            frontIdx(other.frontIdx.load()),
            dataReady(other.dataReady.load()),
            writing(other.writing.load())
        {
            other.frontIdx = 0;
            other.dataReady = false;
            other.writing = false;
        }

        DoubleBuffer& operator=(DoubleBuffer&& other) noexcept {
            if (this != &other) {
                buffers = std::move(other.buffers);
                frontIdx = other.frontIdx.load();
                dataReady = other.dataReady.load();
                writing = other.writing.load();
                
                other.frontIdx = 0;
                other.dataReady = false;
                other.writing = false;
            }
            return *this;
        }

        T& startWrite() {
            while (writing.exchange(true)) { std::this_thread::yield(); }
            return buffers[1 - frontIdx];
        }

        void endWrite() {
            dataReady.store(true, std::memory_order_release);
            writing.store(false, std::memory_order_release);
        }

        bool hasNewData() const { 
            return dataReady.load(std::memory_order_acquire); 
        }

        const T& swapAndGetFront() {
            while (writing.load(std::memory_order_acquire)) { 
                std::this_thread::yield(); 
            }
            
            frontIdx = 1 - frontIdx;
            dataReady.store(false, std::memory_order_release);
            return buffers[frontIdx];
        }

    private:
        std::array<T, 2> buffers;
        std::atomic<int> frontIdx;
        std::atomic<bool> dataReady;
        std::atomic<bool> writing;
    };
}