#ifndef DataPackage_h
#define DataPackage_h

#include <vector>
#include <algorithm>
#include "DataPackageType.h"
#include "../Threading/TaskerGroup.h"
#include "../Common.h"

namespace API {

class DataPackage {
public:
    static constexpr auto HEADER_SIZE = 6;
    static constexpr auto ACTIVE_PROTOCOL_VERSION = 1;

    enum class ProtocolPositions {
        PROTOCOL_VERSION = 0, // +1
        CHECKSUM         = 1, // +4
        // If this is modified remember to update HEADER_SIZE to biggest value + 1.
    };

    enum class action_type_t : uint8_t {
        CREATE_ORDER = 0,
        // MODIFY_ORDER = 1, // not used.
        REMOVE_ORDER = 2
    };

    DataPackage() { }

    bool setData(const unsigned char* data, size_t len);

    std::unique_ptr<Threading::TaskerGroup> makeTask()
    {
        EXPECT_GT(actions_.size(), 0);

        std::unique_ptr<Threading::TaskerGroup> group = WrapUnique(new Threading::TaskerGroup(actions_.size()));
        for (auto&& action : actions_) {
            group->append(action->makeTask());
        }
        return group;
    }

private:
    std::vector<std::unique_ptr<DataPackageType>> actions_;

};

} /* API */

#endif /* DataPackage_h */
