#ifndef MemoryAllocator_h
#define MemoryAllocator_h

#include <atomic>
#include <mutex>
#include "../Common.h"


// 2MB sized pages because most architectures/os's page memory in this size. This allows
// us to free() quickly.
static constexpr size_t BUCKET_SIZES = (1 << 21) - 1;

struct MemoryBucket {
public:
    template <typename T>
    static inline T* nextPtr()
    {
        MemoryBucket* bucket = tipBucket_;
        bucket->used_count_.fetch_add(1);
        size_t index = bucket->next_ptr_.fetch_add(sizeof(ItemWrapper<T>));
        if (UNLIKELY(index + sizeof(ItemWrapper<T>) >= BUCKET_SIZES)) {
            tryNewBucket_(sizeof(ItemWrapper<T>));
            return nextPtr<T>();
        }
        return reinterpret_cast<ItemWrapper<T>*>(&(bucket->items_[index]))->ptr(bucket);
    }

    template <typename T>
    static inline void freePtr(void* ptr)
    {
        MemoryBucket* bucket = reinterpret_cast<ItemWrapper<T>*>(ptr)->bucket();
        size_t usedCount = bucket->used_count_.fetch_sub(1) - 1;
        EXPECT_GT(*reinterpret_cast<int*>(&usedCount), -1);
        if (UNLIKELY(usedCount == 0 && MemoryBucket::tipBucket_ != bucket)) {
            delete bucket;
        }
    }

    static inline size_t objCount()
    {
        MemoryBucket* bucket = tipBucket_;
        return bucket->used_count_.load();
    }

    static inline void terminate()
    {
        MemoryBucket* bucket = tipBucket_;
        size_t count = bucket->used_count_.load();
        EXPECT_EQ(count, 0);
        delete bucket;
    }

private:
    template <typename T>
    class ItemWrapper {
    private:
        // This is up top to show that item_ must be the first item in the struct because of the offset.
        // We also use uint8_t because we don't want to initialize the object/struct.
        uint8_t item_[sizeof(T)];
        MemoryBucket* bucket_;

    public:
        inline MemoryBucket* bucket() { return bucket_; }
        inline T* ptr(MemoryBucket* bucket)
        {
            bucket_ = bucket;
            return reinterpret_cast<T*>(&item_);
        }
    };

    // This function tries is designed to create a new bucket, but there's a rare case
    // where two threads may compete for eachother in creating a new bucket, causing both
    // to succeed and both threads may end up using the same bucket causing a memory leak.
    // This function should protect against that edge case using locks.
    static inline void tryNewBucket_(size_t lastRequestSize)
    {
        std::lock_guard<std::mutex> lock(newAllocatorMux_);
        if (MemoryBucket::tipBucket_->next_ptr_.load() + lastRequestSize >= BUCKET_SIZES) {
            MemoryBucket* previousTipBucket = MemoryBucket::tipBucket_;
            MemoryBucket::tipBucket_ = new MemoryBucket;
            // We do this here because it needs to be increased by 1 before we get into this function
            // for speed and race condition reasons we do not do it in this function.
            if (UNLIKELY(previousTipBucket->used_count_.fetch_sub(1) - 1 == 0)) {
                delete previousTipBucket;
            }
        }
    }

    std::atomic<uint32_t> used_count_ = ATOMIC_VAR_INIT(0);
    std::atomic<uint32_t> next_ptr_ = ATOMIC_VAR_INIT(0);
    std::array<uint8_t, BUCKET_SIZES> items_;

    static MemoryBucket* tipBucket_;
    static std::mutex newAllocatorMux_;

};

#define FAST_ALLOCATE(TYPE) \
    public: \
        void* operator new(size_t, void* p) { return p; } \
        void* operator new[](size_t, void* p) { return p; } \
        void* operator new(size_t sz) \
        { \
            return MemoryBucket::nextPtr<TYPE>(); \
        } \
        void operator delete(void* p) \
        { \
            MemoryBucket::freePtr<TYPE>(p); \
        } \
    private:

#ifdef VALGRIND
    #define IFVALGRIND(v) v
#else
    #define IFVALGRIND(v)
#endif

#endif /* MemoryAllocator_h */
