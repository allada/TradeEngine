#ifndef Allocator_h
#define Allocator_h

#include <functional>
#include <memory>

#define STATIC_ONLY(Type) \
    private:              \
        Type() = delete;  \
        Type(const Type&) = delete;                             \
        Type& operator=(const Type&) = delete;                  \
        void* operator new(size_t) = delete;                    \
        void* operator new(size_t, void*) = delete;             \
    public:

template <typename T>
inline std::unique_ptr<T> WrapUnique(T* ptr) {
  return std::unique_ptr<T>(ptr);
}

class Callback;
// Syntactic sugar to make Callback<void()> easier to declare since it
// will be used in a lot of APIs with delayed execution.
using Closure = std::function<void()>;

#define ASSERT(condition, msg) if (condition) fprintf(stderr, "ASSERT FAIL: %s\n", msg)

#endif /* Allocator_h */