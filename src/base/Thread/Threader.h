#ifndef Threader_h
#define Threader_h

#include "../Common.h"
//#include "ThreadManager.h"
#include <thread>
#include <string>

namespace Thread {
typedef std::thread::id ThreadId;

class Threader {
public:
    //friend struct ThreadManager;
    virtual const std::string& name() const = 0;
    virtual void join() = 0;
    virtual ThreadId id() const = 0;
    virtual void entryPoint() = 0;
    virtual void kill() = 0;
};

} /* Thread */

#endif /* Threader_h */
