#include "MemoryAllocator.h"

MemoryBucket* MemoryBucket::tipBucket_ = new MemoryBucket;
std::mutex MemoryBucket::newAllocatorMux_;
