#ifndef Tasker_h
#define Tasker_h

#include "Threader.h"
#include "../Common.h"

namespace Threading {

class Tasker {
public:
    virtual ~Tasker() { };
    virtual void run() = 0;

};

} /* Thread */

#endif /* Tasker_h */
