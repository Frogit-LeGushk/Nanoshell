#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <array>

#include <unistd.h>
#include <cassert>
#include <termios.h>

#include <signal.h>
#include <stdlib.h>

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
        DEFAULT = 0,    YELLOW,         BLUE
    };

    using strview_t         = std::string_view;
    using array_colors_t    = std::array<strview_t, (int)EColors::BLUE + 1>;

    static constexpr const array_colors_t colorsEscapeSeq_ =
    {
        "\033[0m",      "\033[1;33m",   "\033[1;34m"
    };

    static constexpr const strview_t delEscapeSeq_   = "\10\33[K";
    static constexpr const strview_t bellEscapeSeq_  = "\7";
    static constexpr const strview_t leftEscapeSeq_  = "\033[1D";
    static constexpr const strview_t rightEscapeSeq_ = "\033[1C";

    static constexpr const int ASCII_BEGIN_ = 33;
    static constexpr const int ASCII_END_   = 126;

public:
    static constexpr const strview_t exitCmd = "exit";

struct SmartCmdLine
{
    SmartCmdLine(Shell * shell) : shell_(shell)
        { assert(shell_); }
    std::string const& getCmdLine(void) const noexcept
        { return shell_->cmdLineBuffer_; }
    ~SmartCmdLine(void) noexcept
    {
        shell_->cmdLineBuffer_.resize(0);
        shell_->pos_ = 0;
    }
private:
    Shell * shell_;
};

public:
    Shell(void)     noexcept;
    ~Shell(void)    noexcept;

    void            printPreviewMessage(void)   const noexcept;
    SmartCmdLine    getSmartCmdLine(void)       noexcept;

private:
    char getChar_               (void)                              const noexcept;
    void applyColor_            (EColors color, bool isFlush = true)const noexcept;
    void printHelloMessage_     (void) const noexcept;
    void printGoodbuyMessage_   (void) const noexcept;

private:
    char        ch_;
    size_t      pos_ = 0;
    std::string cmdLineBuffer_;
};
