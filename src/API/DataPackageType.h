#ifndef DataPackageType_h
#define DataPackageType_h

#include "../Threading/Tasker.h"
#include "../Common.h"

namespace API {

class DataPackageType {
public:
    virtual ~DataPackageType() { }
    virtual std::unique_ptr<Threading::Tasker> makeTask() = 0;

};

} /* API */

#endif /* DataPackageType_h */
