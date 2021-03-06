#ifndef Common_h
#define Common_h

#include <memory>

#define IS_DEBUG 1

typedef int FileDescriptor;

#include <chrono>
extern size_t PROFILER_ACCUM;

#define PROFILE_START_() \
    auto _temp_profiler_start_time = std::chrono::high_resolution_clock::now();
#define PROFILE_END_() \
    PROFILER_ACCUM += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - _temp_profiler_start_time).count();

#define PROFILE_START() \
    { PROFILE_START_()
#define PROFILE_END() \
    PROFILE_END() }

#define STATIC_ONLY(Type) \
    private:              \
        Type() = delete;  \
        Type(const Type&) = delete;                             \
        Type& operator=(const Type&) = delete;                  \
        void* operator new(size_t) = delete;                    \
        void* operator new(size_t, void*) = delete;             \
    public:

template <typename T>
std::unique_ptr<T> WrapUnique(T* ptr) {
  return std::unique_ptr<T>(ptr);
}

template<typename T>
struct is_unique_ptr 
    : std::false_type {
};
template<typename T>
struct is_unique_ptr<std::unique_ptr<T>> 
    : std::true_type {
};

// Syntactic sugar to make Callback<void()> easier to declare since it
// will be used in a lot of APIs with delayed execution.
using Closure = std::function<void()>;

#if defined(DEBUG_OUTPUT)
    #define TERMINAL_COLOR
#endif

#ifdef DEBUG_OUTPUT
    #define DBG(...) WARNING("DBG: %s in %s:%d", format(__VA_ARGS__).c_str(), __FILE__, __LINE__)
    #define DEBUG(msg, ...) \
        fprintf(OUTPUT_STREAM, (TerminalColor::colorizeTerminal("Thread %s: ") + msg + "\n").c_str(), debugthisThreadName_().c_str(), ##__VA_ARGS__);

    inline const std::string format() {
        return "";
    }
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

    // Functions below this are for debugging only!

    // stacktrace.h (c) 2008, Timo Bingmann from http://idlebox.net/
    // published under the WTFPL v2.0

    #include <stdio.h>
    #include <stdlib.h>
    #include <execinfo.h>
    #include <cxxabi.h>

    /** Print a demangled stack backtrace of the caller function to FILE* out. */
    inline static void print_stacktrace(FILE *out = stderr, unsigned int max_frames = 63)
    {
        fprintf(out, "stack trace:\n");

        // storage array for stack trace address data
        void* addrlist[max_frames+1];

        // retrieve current stack addresses
        int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

        if (addrlen == 0) {
            fprintf(out, "  <empty, possibly corrupt>\n");
            return;
        }

        // resolve addresses into strings containing "filename(function+address)",
        // this array must be free()-ed
        char** symbollist = backtrace_symbols(addrlist, addrlen);

        // allocate string which will be filled with the demangled function name
        size_t funcnamesize = 256;
        char* funcname = (char*)malloc(funcnamesize);

        // iterate over the returned symbol lines. skip the first, it is the
        // address of this function.
        for (int i = 1; i < addrlen; i++) {
            char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

            // find parentheses and +address offset surrounding the mangled name:
            // ./module(function+0x15c) [0x8048a6d]
            for (char *p = symbollist[i]; *p; ++p) {
                if (*p == '(') {
                    begin_name = p;
                } else if (*p == '+') {
                    begin_offset = p;
                } else if (*p == ')' && begin_offset) {
                    end_offset = p;
                    break;
                }
            }

            if (begin_name && begin_offset && end_offset && begin_name < begin_offset) {
                *begin_name++ = '\0';
                *begin_offset++ = '\0';
                *end_offset = '\0';

                // mangled name is now in [begin_name, begin_offset) and caller
                // offset in [begin_offset, end_offset). now apply
                // __cxa_demangle():

                int status;
                char* ret = abi::__cxa_demangle(begin_name,
                                funcname, &funcnamesize, &status);
                if (status == 0) {
                    funcname = ret; // use possibly realloc()-ed string
                    fprintf(out, "  %s : %s+%s\n",
                    symbollist[i], funcname, begin_offset);
                } else {
                // demangling failed. Output function name as a C function with
                // no arguments.
                fprintf(out, "  %s : %s()+%s\n",
                    symbollist[i], begin_name, begin_offset);
                }
            } else {
                // couldn't parse the line? print the whole line.
                fprintf(out, "  %s\n", symbollist[i]);
            }
        }

        free(funcname);
        free(symbollist);
    }

#else
    #ifndef DEBUG
        #define DEBUG(...)
    #endif
#endif

#ifdef TERMINAL_COLOR
    const std::string& debugthisThreadName_();
    class TerminalColor {
    public:
        enum Color {
            RED      = 31,
            GREEN    = 32,
            BLUE     = 34,
            DEFAULT  = 39
        };
        static std::string colorizeTerminal(const std::string& data);
        static void registerThread();
    };

    #define REGISTER_THREAD_COLOR() \
        TerminalColor::registerThread();

    #ifndef DEBUG
        #define DEBUG(msg, ...) \
            fprintf(OUTPUT_STREAM, (TerminalColor::colorizeTerminal("Thread %s: ") + msg + "\n").c_str(), debugthisThreadName_().c_str(), ##__VA_ARGS__);
    #endif

    #ifndef WARNING
        #define WARNING(msg, ...) \
            fprintf(OUTPUT_STREAM, (TerminalColor::colorizeTerminal("Thread %s: ") + msg + "\n").c_str(), debugthisThreadName_().c_str(), ##__VA_ARGS__);
    #endif
#else
    #define REGISTER_THREAD_COLOR()
#endif

#define OUTPUT_STREAM stderr

#if defined(IS_TEST)
    #define VIRTUAL_FOR_TEST virtual
    #include "gtest/gtest.h"
#else
    #define VIRTUAL_FOR_TEST
#endif

template<typename L> inline const char* fmtLookupTable() { return "%p"; };

template<typename T>
inline static const char* fmt(T)
{
    return fmtLookupTable<typename std::decay<T>::type>();
}

#if !IS_DEBUG || defined(IS_TEST)
    #ifndef WARNING
        #define WARNING(...)
    #endif

    #ifndef EXPECT_GT
        #define EXPECT_GT(...)
    #endif
    #ifndef EXPECT_EQ
        #define EXPECT_EQ(...)
    #endif
    #ifndef EXPECT_NE
        #define EXPECT_NE(...)
    #endif

    #ifndef EXPECT_TRUE
        #define EXPECT_TRUE(...)
    #endif
    #ifndef EXPECT_FALSE
        #define EXPECT_FALSE(...)
    #endif

    #define EXPECT_MAIN_THREAD()
    #define EXPECT_IO_THREAD()
    #define EXPECT_UI_THREAD()
#else
    #ifndef WARNING
        #define WARNING(...)
    #endif

    inline std::string CHAR_TO_STRING_(const char* data)
    {
        return std::string(data);
    }

    template<typename ...Ints>
    inline std::string CHAR_TO_STRING_(const char* data, Ints*... args)
    {
        return std::string(data) + CHAR_TO_STRING_(args...);
    }

    #ifndef EXPECT_GT
        #define EXPECT_GT(v1, v2) if ((v1) < (v2)) { WARNING(CHAR_TO_STRING_("EXPECT FAIL ", ::fmt(v1), " > ", ::fmt(v2), " in %s:%d"), v1, v2, __FILE__, __LINE__); }
    #endif
    #ifndef EXPECT_EQ
        #define EXPECT_EQ(v1, v2) if ((v1) != (v2)) { WARNING(CHAR_TO_STRING_("EXPECT FAIL ", ::fmt(v1), " == ", ::fmt(v2), " in %s:%d"), v1, v2, __FILE__, __LINE__); }
    #endif
    #ifndef EXPECT_NE
        #define EXPECT_NE(v1, v2) if ((v1) == (v2)) { WARNING(CHAR_TO_STRING_("EXPECT FAIL ", ::fmt(v1), " != ", ::fmt(v2), " in %s:%d"), v1, v2, __FILE__, __LINE__); }
    #endif
    #ifndef EXPECT_TRUE
        #define EXPECT_TRUE(v) if (!(v)) { WARNING("EXPECT TRUE in %s:%d", __FILE__, __LINE__); }
    #endif
    #ifndef EXPECT_FALSE
        #define EXPECT_FALSE(v) if ((v)) { WARNING("EXPECT FAIL in %s:%d", __FILE__, __LINE__); }
    #endif

    #define EXPECT_MAIN_THREAD() EXPECT_EQ(static_cast<int>(std::hash<std::thread::id>()(::Threading::ThreadManager::mainThreadId())), static_cast<int>(std::hash<std::thread::id>()(::Threading::ThreadManager::thisThreadId())))
    bool isIoThread();
    bool isUiThread();
    #define EXPECT_IO_THREAD() if (!isIoThread()) { EXPECT_TRUE(false); WARNING("Expected IO Thread"); }
    #define EXPECT_UI_THREAD() if (!isUiThread()) { EXPECT_TRUE(false); WARNING("Expected UI Thread"); }

#endif

#endif /* Common_h */