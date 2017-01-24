#ifndef StreamDispatcher_h
#define StreamDispatcher_h

#include "../Threading/ThreadManager.h"
#include "DataPackage.h"
#include "../includes/crc32.h"
#include "../Common.h"

namespace API {

class StreamDispatcher {
    FAST_ALLOCATE(StreamDispatcher)
public:
    typedef uint32_t Hash;

    static Hash hash(const void* ptr, size_t len, Hash previousHash = 0)
    {
        return static_cast<Hash>(::crc32(ptr, len, static_cast<uint32_t>(previousHash)));
    }

    void processData(Hash originHash, unsigned char* begin, size_t len)
    {
        std::unique_ptr<DataPackage> package(new DataPackage);
        if (!package->setData(begin, len)) {
            return; // appendData wants to drop packet.
        }

        processPackage(std::move(package));
    }

    VIRTUAL_FOR_TEST void processPackage(std::unique_ptr<DataPackage> package)
    {
        EXPECT_NE(package.get(), nullptr);

        std::unique_ptr<Threading::Tasker> task = package->makeTask();
        if (UNLIKELY(!task.get())) {
            return;
        }

        Threading::uiThread()->addTask(std::move(task));
    }

};

} /* API */

#endif /* StreamDispatcher_h */
