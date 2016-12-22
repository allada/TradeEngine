#ifndef StreamDispatcher_h
#define StreamDispatcher_h

#include "../Threading/ThreadManager.h"
#include "DataPackage.h"
#include "Engine/ProcessOrderTask.h"
#include "../Common.h"

namespace API {

class StreamDispatcher {
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
            std::unique_ptr<DataPackage> package;
            if (partial_packages_.count(originHash)) {
                package = std::move(partial_packages_[originHash]);
            } else {
                package = WrapUnique(new DataPackage);
            }
            begin += package->appendData(begin, end - begin);

            EXPECT_TRUE(package->isDone() || begin == end); // API Data Package has not consumed all the data and is not done

            if (package->isDone()) {
                // TODO Send to IO thread?
                processPackage(std::move(package));
            } else {
                EXPECT_EQ(partial_packages_.count(originHash), 0);
                partial_packages_.emplace(originHash, std::move(partial_packages_[originHash]));
            }
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

private:
    std::unordered_map<Hash, std::unique_ptr<DataPackage>> partial_packages_;

};

} /* API */

#endif /* StreamDispatcher_h */
