#include "DataPackage.h"

#include <endian.h>
#include "DataPackage.h"
#include "StreamDispatcher.h"
#include "CreateOrderPackage.h"

using namespace API;

StreamDispatcher::Hash checksum_(const unsigned char* data, size_t len)
{
    return static_cast<StreamDispatcher::Hash>(htole32(reinterpret_cast<const uint32_t&>(data[static_cast<int>(DataPackage::ProtocolPositions::CHECKSUM)])));
}

StreamDispatcher::Hash computeChecksum_(const unsigned char* data, size_t len)
{
    return StreamDispatcher::hash(data, len);
}

bool verifyChecksum_(const unsigned char* data, size_t len)
{
    return checksum_(data, len) == computeChecksum_(data + DataPackage::HEADER_SIZE, len - DataPackage::HEADER_SIZE);
}

uint8_t version_(const unsigned char* data)
{
    return reinterpret_cast<const uint8_t&>(data[static_cast<int>(DataPackage::ProtocolPositions::PROTOCOL_VERSION)]);
}

bool DataPackage::setData(const unsigned char* data, size_t len)
{
    if (len <= HEADER_SIZE) {
        WARNING("Header packet too small. Dropping packet!");
        return false;
    }
    if (version_(data) != DataPackage::ACTIVE_PROTOCOL_VERSION) {
        WARNING("Failed because got wrong version. Dropping packet! Got: %lu Expected: %lu", version_(data), DataPackage::ACTIVE_PROTOCOL_VERSION);
        return false;
    }
    if (!verifyChecksum_(data, len)) {
        WARNING("Failed Checksum check. Dropping packet!");
        return false;
    }
    const unsigned char* end = data + len;
    data += HEADER_SIZE;

    while (data < end) {
        action_type_t actionType = *reinterpret_cast<const action_type_t*>(data);
        switch (actionType) {
            case action_type_t::CREATE_ORDER: {
                std::unique_ptr<CreateOrderPackage> action = WrapUnique(new CreateOrderPackage);

                if (data + CreateOrderPackage::PACKAGE_SIZE > end) {
                    EXPECT_TRUE(false);
                    WARNING("Buffer Overflow from socket prevented: %lu > %lu", data + CreateOrderPackage::PACKAGE_SIZE, end);
                    break;
                }
                if (!action->setData(data, CreateOrderPackage::PACKAGE_SIZE)) {
                    data += CreateOrderPackage::PACKAGE_SIZE;
                    break;
                }
                data += CreateOrderPackage::PACKAGE_SIZE;
                actions_.push_back(std::move(action));
                break;
            }
            case action_type_t::REMOVE_ORDER:
                EXPECT_TRUE(false);
                WARNING("Remove order not implemented yet");
                return false;
            default:
                EXPECT_TRUE(false);
                WARNING("Got package with unknown type: %u", actionType);
                return false;
        }
    }

    return true;
}
