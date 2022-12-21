#pragma once

#include "primitives.h"

#include <future>

namespace engine {

    namespace thread {

        enum ThreadPriority {
            LOWEST, NORMAL, HIGHEST
        };

        struct ThreadFormat {
                ThreadPriority priority;
                const char* name;

                ThreadFormat(ThreadPriority newPriority, const char* newName)
                : priority(newPriority), name(newName) {}
        };

        void setThreadFormat(u32 id, std::thread& thread, const ThreadFormat& format);
    }
}
