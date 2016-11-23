#ifndef Common_h
#define Common_h

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

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

namespace TerminalColor {
    enum Color {
        RED      = 31,
        GREEN    = 32,
        BLUE     = 34,
        DEFAULT  = 39
    };

    static std::unordered_map<std::thread::id, Color> registeredTerminalColors;
    inline std::string colorizeTerminal(const std::string& data)
    {
        if (!registeredTerminalColors.count(std::this_thread::get_id()))
            return data;
        fprintf(stderr, "1\n");
        Color color = registeredTerminalColors.at(std::this_thread::get_id());
        return "\033[" + std::to_string(color) + data;
    }

    inline void registerThread(Color color)
    {
        registeredTerminalColors.emplace(std::this_thread::get_id(), color);
    }
}

#define DEBUG(msg, ...) \
    fprintf(stdout, (TerminalColor::colorizeTerminal("Thread %d: ") + msg + "\n").c_str(), std::this_thread::get_id(), ##__VA_ARGS__);

#define WARNING(msg, ...) \
    fprintf(stderr, (TerminalColor::colorizeTerminal("Thread %d: ") + msg + "\n").c_str(), std::this_thread::get_id(), ##__VA_ARGS__);

// Syntactic sugar to make Callback<void()> easier to declare since it
// will be used in a lot of APIs with delayed execution.
using Closure = std::function<void()>;

#define ASSERT(condition, msg, ...) if (condition) WARNING("ASSERT FAIL: \"%s\" in %s:%d", format(msg, ##__VA_ARGS__).c_str(), __FILE__, __LINE__)
#define ASSERT_EQ(v1, v2, msg, ...) if (v1 != v2) WARNING("ASSERT FAIL %d == %d: \"%s\" in %s:%d", v1, v2, format(msg, ##__VA_ARGS__).c_str(), __FILE__, __LINE__)
#define ASSERT_NE(v1, v2, msg, ...) if (v1 == v2) WARNING("ASSERT FAIL %d != %d: \"%s\" in %s:%d", v1, v2, format(msg, ##__VA_ARGS__).c_str(), __FILE__, __LINE__)

#include <string>
#include <cstdarg>
inline const std::string format(const char* fmt, ...){
    int size = 512;
    char* buffer = 0;
    buffer = new char[size];
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt, vl);
    if(size<=nsize){ //fail delete buffer and try again
        delete[] buffer;
        buffer = 0;
        buffer = new char[nsize+1]; //+1 for /0
        nsize = vsnprintf(buffer, size, fmt, vl);
    }
    std::string ret(buffer);
    va_end(vl);
    delete[] buffer;
    return ret;
}

typedef int FileDescriptor;

#endif /* Common_h */