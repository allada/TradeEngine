#ifndef TaskerGroup_h
#define TaskerGroup_h

#include "../Threading/Tasker.h"
#include "../Common.h"

namespace Threading {

class TaskerGroup : public virtual Threading::Tasker {
public:
    TaskerGroup(size_t sz = 0)
    {
        EXPECT_IO_THREAD();
        tasks_.reserve(sz);
    }

    void append(std::unique_ptr<Tasker> tasker) {
        tasks_.push_back(std::move(tasker));
    }

    void run() override {
        EXPECT_UI_THREAD();
        for (auto&& task : tasks_) {
            task->run();
        }
    }

private:
    std::vector<std::unique_ptr<Threading::Tasker>> tasks_;

};

}

#endif /* TaskerGroup_h */
