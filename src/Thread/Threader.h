#ifndef Threader_h
#define Threader_h

#include <thread>
#include <string>
#include "../Common.h"

namespace Thread {

enum class Signals {
    Kill
};

typedef std::thread::id ThreadId;

class Threader {
public:

    virtual const std::string& name() const = 0;
    virtual void join() = 0;
    virtual ThreadId id() const = 0;
    virtual void entryPoint() = 0;
    virtual void kill() = 0;
    virtual void sendSignal(Signals) = 0;
};

ThreadId thisThreadId();
std::shared_ptr<Threader> thisThread();
const std::string& thisThreadName();

} /* Thread */

#endif /* Threader_h */
