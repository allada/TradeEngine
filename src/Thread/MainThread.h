#ifndef MainThread_h
#define MainThread_h

#include "Threader.h"

namespace Thread {

static const std::string& name_("Main");

// Base class for main thread.
class MainThread : public virtual Threader {
public:
    MainThread()
        : threadId_(thisThreadId()) { }

    const std::string& name() const override { return name_; }
    void join() override { WARNING("Cannot kill main thread"); }
    ThreadId id() const override { return threadId_; }
    void kill() override { WARNING("Cannot kill main thread"); }

    void entryPoint() override { WARNING("Cannot entryPoint() main thread"); }
    void sendSignal(Signals) override { WARNING("Cannot sendSignal() to main thread"); }

private:
    ThreadId threadId_;
};

} /* Thread */

#endif /* MainThread_h */
