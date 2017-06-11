#include "Common.h"
#include "Threading/ThreadManager.h"

size_t PROFILER_ACCUM = 0;

#ifdef TERMINAL_COLOR
    #include "Threading/Threader.h"
    #include <unordered_map>

    static const std::string resetColor()
    {
        return "\033[0;" + std::to_string(TerminalColor::DEFAULT) + 'm';
    }

    const std::string& debugthisThreadName_()
    {
        return Threading::thisThreadName();
    }

    static constexpr TerminalColor::Color colors_[3] = { TerminalColor::RED, TerminalColor::GREEN, TerminalColor::BLUE };
    static int counter_ = 0;
    static std::unordered_map<std::thread::id, TerminalColor::Color> registeredTerminalColors;

    std::string TerminalColor::colorizeTerminal(const std::string& data)
    {
        if (registeredTerminalColors.count(std::this_thread::get_id()) == 0) {
            return data;
        }
        Color color = registeredTerminalColors.at(std::this_thread::get_id());
        return "\033[1;" + std::to_string(color) + 'm' + data + resetColor();
    }

    void TerminalColor::registerThread()
    {
        registeredTerminalColors.emplace(std::this_thread::get_id(), colors_[counter_++ % sizeof(colors_)]);
    }
#endif

template<> const char* fmtLookupTable<int>()
{
    return "%d";
}

template<> const char* fmtLookupTable<long>()
{
    return "%lu";
}

template<> const char* fmtLookupTable<unsigned long>()
{
    return "%lu";
}

template<> const char* fmtLookupTable<char>()
{
    return "%d";
}

template<> const char* fmtLookupTable<unsigned long long>()
{
    return "%llu";
}

template<> const char* fmtLookupTable<void*>()
{
    return "%p";
}
#if IS_DEBUG || defined(IS_TEST)
    bool isIoThread() {
        return Threading::ioThread().get() == Threading::ThreadManager::thisThread().get();
    }

    bool isUiThread() {
        return Threading::uiThread().get() == Threading::ThreadManager::thisThread().get();
    }
#endif