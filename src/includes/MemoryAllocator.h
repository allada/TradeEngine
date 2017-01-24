#ifndef MemoryAllocator_h
#define MemoryAllocator_h

#include <atomic>
#include <mutex>
#include "../Common.h"


// 2MB sized pages because most architectures/os's page memory in this size. This allows
// us to free() quickly.
static constexpr size_t BUCKET_SIZES = (1 << 21);

struct MemoryBucket {
public:
    template <typename T>
    static T* nextPtr(size_t sz)
    {
        EXPECT_GT(BUCKET_SIZES, sz);
        MemoryBucket* bucket = tipBucket_;
        sz += sizeof(MemoryBucket*);
        bucket->used_count_.fetch_add(1);
        size_t index = bucket->next_ptr_.fetch_add(sz);
        if (UNLIKELY(index + sz >= BUCKET_SIZES)) {
            tryNewBucket_(sz);
            return nextPtr<T>(sz);
        }
        return reinterpret_cast<ItemWrapper<T>*>(&(bucket->items_[index]))->ptr(bucket);
    }

    template <typename T>
    static void freePtr(void* ptr)
    {
        MemoryBucket* bucket = reinterpret_cast<ItemWrapper<T>*>(reinterpret_cast<ssize_t>(ptr) - sizeof(MemoryBucket*))->bucket();
        size_t usedCount = bucket->used_count_.fetch_sub(1) - 1;
        EXPECT_GT(usedCount + 1, 0);
        if (UNLIKELY(usedCount == 0 && MemoryBucket::tipBucket_ != bucket)) {
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
    template <typename T>
    class ItemWrapper {
    private:
        MemoryBucket* bucket_;
        // This is up top to show that item_ must be the first item in the struct because of the offset.
        // We also use uint8_t because we don't want to initialize the object/struct.
        uint8_t item_;

    public:
        MemoryBucket* bucket() { return bucket_; }
        T* ptr(MemoryBucket* bucket)
        {
            bucket_ = bucket;
            return reinterpret_cast<T*>(&item_);
        }
    };

    // This function tries is designed to create a new bucket, but there's a rare case
    // where two threads may compete for eachother in creating a new bucket, causing both
    // to succeed and both threads may end up using the same bucket causing a memory leak.
    // This function should protect against that edge case using locks.
    static void tryNewBucket_(size_t lastRequestSize)
    {
        std::lock_guard<std::mutex> lock(newAllocatorMux_);
        // We do another check here because between this function and the call point there may already
        // be a new bucket assigned. This will ensure we do not create a new bucket in such case.
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
