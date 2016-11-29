#ifndef APIDataPackage_h
#define APIDataPackage_h

namespace Net {

class APIDataPackage {
public:
    size_t appendData(const unsigned char*, size_t) { return 0; }
    bool done() { return false; }
};

} /* Net */

#endif /* APIDataPackage_h */
