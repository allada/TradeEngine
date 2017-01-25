#ifndef MemoryAllocator_h
#define MemoryAllocator_h

#include <atomic>
#include <mutex>
#include "../Common.h"


// 2MB sized pages because most architectures/os's page memory in this size. This allows
// us to free() quickly.
static constexpr size_t SHIFT_BITS = 21;
static constexpr size_t BUCKET_SIZE = (1 << SHIFT_BITS);
static constexpr size_t BUCKET_MASK = BUCKET_SIZE - 1;

struct MemoryBucket {
public:
    template <typename T>
    static T* nextPtr(size_t sz)
    {
        EXPECT_GT(BUCKET_SIZE, sz);
        // Must always be even.
        if ((sz & 1) == 1) {
            sz += 1;
        }
        MemoryBucket* bucket = tipBucket_;
        bucket->used_count_.fetch_add(1);
        // uintptr_t beginPtr = reinterpret_cast<uintptr_t>(bucket->items_.data());
        uintptr_t basePtr = bucket->begin_ptr_;
        size_t used_bytes = bucket->bytes_used_.fetch_add(sz);
        if (UNLIKELY(used_bytes + sz >= BUCKET_SIZE)) {
            tryNewBucket_(sz);
            return nextPtr<T>(sz);
        }
        uintptr_t endPtr = reinterpret_cast<uintptr_t>(bucket->items_.data()) + BUCKET_SIZE;
        uintptr_t desiredPtr = basePtr + used_bytes;
        if (desiredPtr + sz >= endPtr && !bucket->is_over_fence_) {
            {
                std::lock_guard<std::mutex> lock(bucket->over_fence_mux_);
                // Make sure we do not set it wtice because of timing with lock.
                if (!bucket->is_over_fence_) {
                    bucket->bytes_used_.fetch_add(1);
                    bucket->is_over_fence_ = true;
                }
            }
            bucket->used_count_.fetch_sub(1);
            return nextPtr<T>(sz);
        }
        if (desiredPtr + sz > endPtr && desiredPtr < endPtr) {
            // Run again. We are on a fence post.
            bucket->used_count_.fetch_sub(1);
            return nextPtr<T>(sz);
        }
        if (desiredPtr + sz >= endPtr) {
            return reinterpret_cast<T*>(desiredPtr - BUCKET_SIZE);
        }

        return reinterpret_cast<T*>(desiredPtr);
    }

    template <typename T>
    static void freePtr(void* ptr)
    {
        MemoryBucket* bucket = *memoryBucketPointerFromPointer(ptr);
        size_t usedCount = bucket->used_count_.fetch_sub(1) - 1;
        EXPECT_GT(usedCount + 1, 0);
        if (UNLIKELY(usedCount == 0 && tipBucket_ != bucket)) {
            delete bucket;
        }
    }

    static size_t objCount()
    {
        MemoryBucket* bucket = tipBucket_;
        return bucket->used_count_.load();
    }

    static void terminate()
    {
        MemoryBucket* bucket = tipBucket_;
        size_t count = bucket->used_count_.load();
        EXPECT_EQ(count, 0);
        delete bucket;
    }

private:
    MemoryBucket()
    {
        MemoryBucket** bucketPtr = memoryBucketPointerFromPointer(items_.data());
        *bucketPtr = this;
        begin_ptr_ = reinterpret_cast<uintptr_t>(bucketPtr);
        // Offset by 1 to tell memoryBucketPointerFromPointer that it should go to right not left.
        bytes_used_ = sizeof(MemoryBucket*) + 1;
    }

    inline static MemoryBucket** memoryBucketPointerFromPointer(void* ptr) {
        return memoryBucketPointerFromPointer(reinterpret_cast<uintptr_t>(ptr));
    }

    inline static MemoryBucket** memoryBucketPointerFromPointer(uintptr_t ptr)
    {
        // TODO check even ptr problems.
        if ((ptr & 1) == 0) {
            return reinterpret_cast<MemoryBucket**>((ptr & ~BUCKET_MASK) + BUCKET_SIZE);
        } else {
            return reinterpret_cast<MemoryBucket**>((ptr & ~BUCKET_MASK));
        }
    }

    // This function tries is designed to create a new bucket, but there's a rare case
    // where two threads may compete for eachother in creating a new bucket, causing both
    // to succeed and both threads may end up using the same bucket causing a memory leak.
    // This function should protect against that edge case using locks.
    static void tryNewBucket_(size_t lastRequestSize)
    {
        std::lock_guard<std::mutex> lock(newAllocatorMux_);
        // We do another check here because between this function and the call point there may already
        // be a new bucket assigned. This will ensure we do not create a new bucket in such case.
        if (tipBucket_->bytes_used_.load() + lastRequestSize < BUCKET_SIZE)
            return;
        MemoryBucket* previousTipBucket = tipBucket_;
        tipBucket_ = new MemoryBucket;
        // We do this here because it needs to be increased by 1 before we get into this function
        // for speed and race condition reasons we do not do it in this function.
        if (UNLIKELY(previousTipBucket->used_count_.fetch_sub(1) - 1 == 0)) {
            delete previousTipBucket;
        }
    }

    // Very important to have this first to make sure we are on an even pointer.
    std::array<uint8_t, BUCKET_SIZE> items_;

    uintptr_t begin_ptr_;

    std::atomic<int32_t> bytes_used_ = ATOMIC_VAR_INIT(0);
    std::atomic<int32_t> used_count_ = ATOMIC_VAR_INIT(0);
    bool is_over_fence_ = false;
    std::mutex over_fence_mux_;

    // std::atomic<uint32_t> next_ptr_ = ATOMIC_VAR_INIT(0);

    static MemoryBucket* tipBucket_;
    static std::mutex newAllocatorMux_;

};

template <typename T>
struct FastAllocator {
    typedef T               value_type;
    typedef T*              pointer;
    typedef T&              reference;
    typedef const T*        const_pointer;
    typedef const T&        const_reference;
    typedef size_t          size_type;

    FastAllocator() { }
    explicit FastAllocator(const FastAllocator& other)
        : size_(other.size_) { }
    explicit FastAllocator(FastAllocator&& other)
        : size_(other.size_) { }
    template <class U>
    FastAllocator(const FastAllocator<U>& other)
        : size_(other.max_size()) { }

    template <class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        new(p) U (std::forward<Args>(args)...);
    }

    void destroy(T* p)
    {
        p->~T();
    }

    size_type max_size() const noexcept
    {
        return size_;
    }

    size_type max_size(T* p) const noexcept
    {
        return size_;
    }

    T* allocate(size_type n, const_pointer = 0)
    {
        size_ = n;
        return MemoryBucket::nextPtr<T>(n * sizeof(T));
    }

    void deallocate(T* p, size_type size_t)
    {
        size_ = 0;
        return MemoryBucket::freePtr<T>(p);
    }

private:
    size_t size_;

};

#define FAST_ALLOCATE(TYPE) \
    public: \
        void* operator new(size_t, void* p) { return p; } \
        void* operator new[](size_t, void* p) { return p; } \
        void* operator new(size_t sz) \
        { \
            return MemoryBucket::nextPtr<TYPE>(sz); \
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
