#ifndef APIDataPackage_h
#define APIDataPackage_h

#include <vector>
#include "../Common.h"

namespace Net {

class APIDataPackage {
public:
    const std::vector<unsigned char>& data() const { return data_; }

    size_t appendData(const unsigned char& data, size_t len)
    {
        // TODO Finish
        data_.insert(data_.end(), &data, &data + len);
        return len;
    }

    bool done() { return true; }

private:
    std::vector<unsigned char> data_;

};

} /* Net */

#endif /* APIDataPackage_h */
