#include <iostream>
#include <string>
#include <string_view>
#include <array>

#include <unistd.h>
#include <cassert>
#include <termios.h>
#include <signal.h>
#include <stdlib.h>

#include "shell.hpp"


Shell::Shell(void) noexcept
{
    // block signals from driver teminal's (CTRL + C and CTRL + D)
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTSTP);
    assert(sigprocmask(SIG_BLOCK, &sigset, NULL) == 0);

    printHelloMessage_();
}

Shell::~Shell(void) noexcept
{
    printGoodbuyMessage_();
}

void Shell::printPreviewMessage(void) const noexcept
{
    char bufLogin[256] = {0};
    assert(getlogin_r(bufLogin, sizeof(bufLogin)) == 0);

    char bufCwd[BUFSIZ] = {0};
    assert(getcwd(bufCwd, BUFSIZ));

    applyColor_(EColors::YELLOW, false);
    std::cout << bufLogin << "@" << bufCwd << "$ ";
    applyColor_(EColors::DEFAULT, true);
}

Shell::SmartCmdLine Shell::getSmartCmdLine(void) noexcept
{
    while ((ch_ = getChar_()) != (uint8_t)ESpecialAscii::ENTER)
    {
        if ((ASCII_BEGIN_ <= ch_ && ch_ <= ASCII_END_) ||
            (ch_ == (uint8_t)ESpecialAscii::SPACE))
        {
            std::cout << ch_;
            std::cout.flush();

            char buf[2] = {ch_, '\0'};
            cmdLineBuffer_.insert((pos_++), buf);
        }
        else if (ch_ == (uint8_t)ESpecialAscii::BACKSPACE)
        {
            if (pos_ > 0)
            {
                std::cout << delEscapeSeq_;
                std::cout.flush();
                cmdLineBuffer_.erase((pos_--) - 1, 1);
            }
            else
            {
                std::cout << bellEscapeSeq_;
                std::cout.flush();
            }
        }
        else if (ch_ == (uint8_t)ESpecialAscii::CTRL_D)
        {
            cmdLineBuffer_ = exitCmd;
            pos_ = 0;
            break;
        }
        else
        {
            std::cout << "Unknown symbol" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    std::cout << std::endl;
    return this;
}

char Shell::getChar_(void) const noexcept
{
    char buf = 0;

    struct termios old;
    assert(tcgetattr(0, &old) == 0);

    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    assert(tcsetattr(0, TCSANOW, &old) == 0);

    assert(read(0, &buf, 1) != -1);

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    assert(tcsetattr(0, TCSADRAIN, &old) == 0);

    return buf;
}

void Shell::applyColor_(EColors color, bool isFlush) const noexcept
{
    std::cout << colorsEscapeSeq_[(uint8_t)color];
    if (isFlush)
        std::cout.flush();
}

void Shell::printHelloMessage_(void) const noexcept
{
    applyColor_(EColors::BLUE, false);
    std::cout << "How to use:\n";
    applyColor_(EColors::DEFAULT, true);
}

void Shell::printGoodbuyMessage_(void) const noexcept
{
    applyColor_(EColors::BLUE, false);
    std::cout << "[See you, space cowboy ...]\n";
    applyColor_(EColors::DEFAULT, true);
}


