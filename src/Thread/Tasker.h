#ifndef Tasker_h
#define Tasker_h

#include "Threader.h"

namespace Thread {

class Tasker {
public:
    virtual void run() = 0;
};

} /* Thread */

#endif /* Tasker_h */
