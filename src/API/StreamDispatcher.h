#ifndef StreamDispatcher_h
#define StreamDispatcher_h

#include "../Threading/ThreadManager.h"
#include "DataPackage.h"
#include "Engine/ProcessOrderTask.h"
#include "../Common.h"

namespace API {

class StreamDispatcher {
    FAST_ALLOCATE(StreamDispatcher)
public:
    typedef size_t Hash;

    static inline Hash hash(const void* ptr, size_t len, Hash previousHash = 0)
    {
        return static_cast<Hash>(::crc32(ptr, len, static_cast<uint32_t>(previousHash)));
    }

    inline void processData(Hash originHash, unsigned char* begin, size_t len)
    {
        unsigned char* end = begin + len;
        while (end > begin) {
            std::unique_ptr<DataPackage> package(new DataPackage);
            begin += package->appendData(begin, end - begin);

            EXPECT_TRUE(package->isDone() || begin == end); // API Data Package has not consumed all the data and is not done

            if (UNLIKELY(!package->isDone())) {
                WARNING("Got incomplete order, dropping data");
                return;
            }
            processPackage(std::move(package));
        }
    }

    VIRTUAL_FOR_TEST inline void processPackage(std::unique_ptr<DataPackage> package)
    {
        EXPECT_NE(package.get(), nullptr);
        if (UNLIKELY(!package->quickVerify())) {
            return;
        }
        Threading::uiThread()->addTask(WrapUnique(new Engine::ProcessOrderTask(std::move(package))));
    }

};

} /* API */

#endif /* StreamDispatcher_h */
