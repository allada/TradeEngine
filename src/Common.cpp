#include "Common.h"

#include "Thread/Threader.h"

#if IS_DEBUG
    const std::string& thisThreadName()
    {
        return Thread::thisThread()->name();
    }

    static constexpr TerminalColor::Color colors_[3] = { TerminalColor::RED, TerminalColor::GREEN, TerminalColor::BLUE };
    static int counter_ = 0;
    static std::unordered_map<std::thread::id, TerminalColor::Color> registeredTerminalColors;

    std::string TerminalColor::colorizeTerminal(const std::string& data)
    {
        if (!registeredTerminalColors.count(std::this_thread::get_id()))
            return data;
        Color color = registeredTerminalColors.at(std::this_thread::get_id());
        fprintf(stderr, "%d\n", color);
        return "\033[" + std::to_string(color) + data;
    }

    void TerminalColor::registerThread()
    {
        Color color = colors_[counter_ % sizeof(colors_)];
        ++counter_;
        registeredTerminalColors.emplace(std::this_thread::get_id(), color);
    }
#endif
