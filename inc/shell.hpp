#pragma once
#include "analyze.hpp"
#include <string_view>
#include <array>

namespace shell {

class Shell
{
    enum class ESpecialAscii : uint8_t
    {
        BACKSPACE   = 127,
        ENTER       = 10,
        SPACE       = 32,
        CTRL_D      = 4
    };

    enum class EColors : uint8_t
    {
        DEFAULT = 0,
        YELLOW,
        BLUE
    };

    enum class EStateTask : uint8_t
    {
        RUN,
        STOPPED,
        DONE,
        RUN_STOPPED, // only for pipe
        UNKNOWN
    };

    using strview_t     = std::string_view;
    using array_colors_t= std::array<strview_t, (int)EColors::BLUE + 1>;

    static constexpr const array_colors_t colorsEscapeSeq_ =
    {
        "\033[0m",
        "\033[1;33m",
        "\033[1;34m"
    };

    static constexpr const strview_t delEscapeSeq_   = "\10\33[K";
    static constexpr const strview_t bellEscapeSeq_  = "\7";
    static constexpr const strview_t leftEscapeSeq_  = "\033[1D";
    static constexpr const strview_t rightEscapeSeq_ = "\033[1C";

    static constexpr const int ASCII_BEGIN_ = 33;
    static constexpr const int ASCII_END_   = 126;

public:
    static constexpr const strview_t exitCmd = "exit";
    static constexpr const strview_t jobsCmd = "jobs";
    static constexpr const strview_t fgCmd = "fg";
    static constexpr const strview_t bgCmd = "bg";

    Shell(void)     noexcept;
    ~Shell(void)    noexcept;

struct SmartCmdLine
{
    SmartCmdLine(Shell * shell) noexcept;
    ~SmartCmdLine(void) noexcept;
    std::string const&  getCmdLine  (void) const noexcept;
private:
    Shell * shell_;
    mutable bool isRetrive_ = false;
};

struct TaskItem
{
    analyze::task_t         task;
    bool                    isForeground;
    std::string const&      cmdLine;
    analyze::ETypeCmdLine   type;
    EStateTask              state = EStateTask::RUN;
};

    void                printPreviewMessage(void)   const noexcept;
    SmartCmdLine        getSmartCmdLine(void)       noexcept;
    void                addTaskItem(TaskItem item)  noexcept;
    void                jobs(void)                  const noexcept;
    bool                isControlFlowCmd(void)      const;
    void                fg(size_t idx)              noexcept;
    void                bg(size_t idx)              noexcept;

private:
    void applyColor_    (EColors color, bool isFlush = true) const noexcept;
    void printMessage_  (std::string const& message, EColors color) const noexcept;
    char getChar_       (void)                      noexcept;
    void waitTasks_     (void)                      noexcept;

private:
    char        char_;
    size_t      pos_ = 0;
    std::string cmdLine_;
    sigset_t    sigset1_;
    sigset_t    sigset2_;
    std::vector<TaskItem> tasks_;
    size_t fgTaskIdx_ = -1;
};

} // namespace shell
