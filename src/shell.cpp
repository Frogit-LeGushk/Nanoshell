#include "../inc/shell.hpp"
#include <iostream>
#include <variant>
#include <sstream>
#include <unistd.h>
#include <assert.h>
#include <termios.h>
#include <signal.h>

using namespace shell;

namespace {

bool IS_SIGCHILD_EVENT = false;
void sigChildHandler(int sig) { (void)sig; IS_SIGCHILD_EVENT = true; }

const char * helloMessage =
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "                   How to use                                      \n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "Step one. Type one of the next commands in format:                 \n"
    "1. <cmd> [argv]... [&]                                             \n"
    "2. <cmd> [argv]... | <cmd> [argv]... [&]                           \n"
    "3. <cmd> [argv]... && <cmd> [argv]... [&]                          \n"
    "4. <cmd> [argv]... || <cmd> [argv]... [&]                          \n"
    "- NOTE: key 'ARR_LEFT' and 'ARR_RIGHT' don't work, sorry ;)        \n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "- [&] means launch in background                                   \n"
    "- | means create pipe                                              \n"
    "- && means launch processes with logical \"and\" operation         \n"
    "- || means launch processes with logical \"or\" operation          \n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "Step two. Just press \"enter\"                                     \n"
    "- wait until task is done or press CTRL + C to terminate them      \n"
    "- wait until task is done or press CTRL + D to stop task           \n"
    "- NOTE: CTRL + C don't wort with background tasks                  \n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "Step three. Control your tasks                                     \n"
    "1. type cmd 'jobs' to look all tasks (see 'N' in first column)     \n"
    "2. type cmd 'fg <N>' or 'bg <N>' to run (previously stopped) task  \n"
    "   in foreground or background respectively                        \n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "Step four. Enter \"exit\" command or press CTRL + D combination    \n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    "                   Goog luck (^-^)                                 \n"
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

const char * goodbuyMessage =
    "[See you, space cowboy ...]\n";

} // namespace


Shell::Shell(void) noexcept
{
    printMessage_(helloMessage, EColors::BLUE);

    sigemptyset(&sigset1_);
    sigaddset(&sigset1_, SIGINT);
    sigaddset(&sigset1_, SIGTSTP);
    sigaddset(&sigset1_, SIGTTIN);
    sigaddset(&sigset1_, SIGTTOU);
    assert(sigprocmask(SIG_BLOCK, &sigset1_, NULL) == 0);

    sigemptyset(&sigset2_);
    sigaddset(&sigset2_, SIGCHLD);
    assert(sigprocmask(SIG_UNBLOCK, &sigset2_, NULL) == 0);

    struct sigaction ssigact;
    ssigact.sa_handler = &sigChildHandler;
    ssigact.sa_flags = 0;
    assert(sigaction(SIGCHLD, &ssigact, NULL) == 0);
}

Shell::~Shell(void) noexcept
{
    for (size_t idx = 0; idx < tasks_.size(); idx++)
    if (tasks_[idx].state != EStateTask::DONE)
    {
        if (tasks_[idx].type == ::analyze::ETypeCmdLine::SINGLE)
        {
            auto singleProcess = std::get<single::Single*>(tasks_[idx].task);
            singleProcess->KILL(::process::Process::EKill::HUP);
        }
        else if (tasks_[idx].type == ::analyze::ETypeCmdLine::PPIPE)
        {
            auto ppipeProcess = std::get<ppipe::Ppipe*>(tasks_[idx].task);
            ppipeProcess->KILL(::process::Process::EKill::HUP);
        }
        else if (tasks_[idx].type == ::analyze::ETypeCmdLine::BOOLEAN)
        {
            auto booleanProcess = std::get<boolean::Boolean*>(tasks_[idx].task);
            booleanProcess->KILL(::process::Process::EKill::HUP);
        }
    }

    printMessage_(goodbuyMessage, EColors::BLUE);
    assert(sigprocmask(SIG_UNBLOCK, &sigset1_, NULL) == 0);
}

Shell::SmartCmdLine::SmartCmdLine(Shell * shell) noexcept
    : shell_(shell)
{
    assert(shell_);
    assert(sigprocmask(SIG_UNBLOCK, &shell_->sigset1_, NULL) == 0);
}

Shell::SmartCmdLine::~SmartCmdLine(void) noexcept
{
    assert(sigprocmask(SIG_BLOCK, &shell_->sigset1_, NULL) == 0);
    shell_->waitTasks_();
    tcsetpgrp(0, getpid());
    shell_->cmdLine_.resize(0);
}

std::string const& Shell::SmartCmdLine::getCmdLine(void) const noexcept
{
    std::string * copyBuffer = nullptr;

    try
    {
        copyBuffer = new std::string(shell_->cmdLine_);
    }
    catch (std::bad_alloc const& err)
    {
        process::PRINT_ERR(err.what());
        exit(EXIT_FAILURE);
    }

    return *copyBuffer;
}

void Shell::printPreviewMessage(void) const noexcept
{
    char bufLogin[256] = {0};
    assert(getlogin_r(bufLogin, sizeof(bufLogin)) == 0);

    char bufCwd[BUFSIZ] = {0};
    assert(getcwd(bufCwd, BUFSIZ));

    std::string const& message = std::string(bufLogin) + "@" + bufCwd + "$ ";
    printMessage_(message, EColors::YELLOW);
}

Shell::SmartCmdLine Shell::getSmartCmdLine(void) noexcept
{
    auto isAsciiChar = [this](char mychar)
    {
        return  (ASCII_BEGIN_ <= mychar && mychar <= ASCII_END_) ||
                (mychar == (uint8_t)ESpecialAscii::SPACE);
    };

    while ((char_ = getChar_()) != (uint8_t)ESpecialAscii::ENTER)
    {
        if (isAsciiChar(char_))
        {
            std::cout << char_;
            cmdLine_.push_back(char_);
        }
        else if (char_ == (uint8_t)ESpecialAscii::BACKSPACE)
        {
            if (cmdLine_.size())
            {
                std::cout << delEscapeSeq_;
                cmdLine_.pop_back();
            }
            else
                std::cout << bellEscapeSeq_;
        }
        else if (char_ == (uint8_t)ESpecialAscii::CTRL_D)
        {
            cmdLine_ = exitCmd;
            break;
        }

        std::cout.flush();
    }

    std::cout << std::endl;
    return SmartCmdLine(this);
}

void Shell::addTaskItem(TaskItem item) noexcept
{
    if (item.isForeground)
        fgTaskIdx_ = tasks_.size();

    try
    {
        tasks_.push_back(item);
    }
    catch (std::bad_alloc const& err)
    {
        process::PRINT_ERR(err.what());
        exit(EXIT_FAILURE);
    }
}

void Shell::jobs(void) const noexcept
{
    auto printTaskInfo = [](size_t idx, TaskItem const& item)
    {
        std::cout << "[" << idx << "]";

        std::cout << " pid: ";
        if (item.type == ::analyze::ETypeCmdLine::SINGLE)
        {
            auto singleProcess = std::get<single::Single*>(item.task);
            std::cout << "[" << singleProcess->getPid() << "]";

            if (item.state == EStateTask::DONE)
                std::cout << ", isTermBySig " << (singleProcess->isTermBySig() ? "+" : "-");
        }
        else if (item.type == ::analyze::ETypeCmdLine::PPIPE)
        {
            auto ppipeProcess = std::get<ppipe::Ppipe*>(item.task);
            auto [pid1, pid2] = ppipeProcess->getPid();
            std::cout << "[" << pid1 << ", " << pid2 << "], ";

            if (item.state == EStateTask::DONE)
            {
                auto [isTermSig1, isTermSig2] = ppipeProcess->isTermBySig();
                std::cout   << "isTermBySig: "
                            << "["
                            << (isTermSig1 ? "+" : "-") << ", "
                            << (isTermSig2 ? "+" : "-")
                            << "]";
            }
        }
        else if (item.type == ::analyze::ETypeCmdLine::BOOLEAN)
        {
            auto booleanProcess = std::get<boolean::Boolean*>(item.task);
            auto [pid1, pid2] = booleanProcess->getPairPid();
            std::cout << "[" << pid1 << ", " << pid2 << "], ";
            if (item.state == EStateTask::DONE)
                std::cout << ", isSuccess: " << (booleanProcess->isSuccess() ? "+" : "-");
        }

        std::cout << ", isForeground: " << (item.isForeground ? "+" : "-");
        std::cout << ", type: ";

        if (item.type == analyze::ETypeCmdLine::SINGLE)
            std::cout << "SINGLE";
        else if (item.type == analyze::ETypeCmdLine::PPIPE)
            std::cout << "PPIPE";
        else if (item.type == analyze::ETypeCmdLine::BOOLEAN)
            std::cout << "BOOLEAN";

        std::cout << ", state: ";

        if (item.state == EStateTask::RUN)
            std::cout << "RUN";
        else if (item.state == EStateTask::STOPPED)
            std::cout << "STOPPED";
        else if (item.state == EStateTask::RUN_STOPPED)
            std::cout << "RUN_STOPPED";
        else if (item.state == EStateTask::DONE)
            std::cout << "DONE";
        else if (item.state == EStateTask::UNKNOWN)
            std::cout << "UNKNOWN";

        std::cout << ", cmdLine: " << item.cmdLine << "\n";
    };

    try
    {
        for (size_t idx = 0; idx < tasks_.size(); idx++)
            printTaskInfo(idx, tasks_[idx]);
        std::cout.flush();
    }
    catch (std::exception const& err)
    {
        process::PRINT_ERR(err.what());
        exit(EXIT_FAILURE);
    }
}

void Shell::applyColor_(EColors color, bool isFlush) const noexcept
{
    std::cout << colorsEscapeSeq_[(uint8_t)color];
    if (isFlush)
        std::cout.flush();
}

void Shell::printMessage_(std::string const& message, EColors color) const noexcept
{
    try
    {
        applyColor_(color, false);
        std::cout << message;
        applyColor_(EColors::DEFAULT, true);
    }
    catch (std::exception const& err)
    {
        process::PRINT_ERR(err.what());
        exit(EXIT_FAILURE);
    }
}

char Shell::getChar_(void) noexcept
{
    char buf = 0;

    struct termios old;
    assert(tcgetattr(0, &old) == 0);

    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    assert(tcsetattr(0, TCSANOW, &old) == 0);

    ssize_t readed = 0;

    while (readed != 1)
    {
        errno = 0;

        assert(sigprocmask(SIG_UNBLOCK, &sigset2_, NULL) == 0);
        readed = read(0, &buf, 1);
        assert(sigprocmask(SIG_BLOCK, &sigset2_, NULL) == 0);

        if (IS_SIGCHILD_EVENT)
        {
            IS_SIGCHILD_EVENT = false;
            std::cout << std::endl;
            cmdLine_.resize(0);
            waitTasks_();
            printPreviewMessage();
        }
        else if (readed == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    assert(tcsetattr(0, TCSADRAIN, &old) == 0);

    return buf;
}

void Shell::waitTasks_(void) noexcept
{
    auto checkUnary = [this](TaskItem& taskItem, bool isAsynk,
                         bool& isChanged, auto process)
    {
        int wstatus = 0;
        bool isDone = process->isDone(isAsynk, &wstatus);
        int pid = process->getPid();

        assert(sigprocmask(SIG_BLOCK, &sigset2_, NULL) == 0);

        if (isDone)
            taskItem.state = EStateTask::DONE;
        else if (WIFSTOPPED(wstatus))
        {
            taskItem.state = EStateTask::STOPPED;
            std::cout << "[" << pid << "] is stopped\n";
        }
        else if (WIFCONTINUED(wstatus))
        {
            taskItem.state = EStateTask::RUN;
            std::cout << "[" << pid << "] is continued\n";
        }
        else
            isChanged = false;

        std::cout.flush();
        assert(sigprocmask(SIG_UNBLOCK, &sigset2_, NULL) == 0);
    };

    auto checkBinary = [this](TaskItem& taskItem, bool isAsynk,
                          bool& isChanged, auto process)
    {
        int wstatus1 = 0, wstatus2 = 0;
        bool isDone = process->isDone(isAsynk, {&wstatus1, &wstatus2});
        auto [pid1, pid2] = process->getPid();

        assert(sigprocmask(SIG_BLOCK, &sigset2_, NULL) == 0);

        if (isDone)
            taskItem.state = EStateTask::DONE;
        // from RUN to other state
        else if (WIFSTOPPED(wstatus1) && WIFSTOPPED(wstatus2))
        {
            taskItem.state = EStateTask::STOPPED;
            std::cout << "[" << pid1 << ", " << pid2 << "] is stopped\n";
        }
        else if (taskItem.state == EStateTask::RUN &&
                 (WIFSTOPPED(wstatus1) || WIFSTOPPED(wstatus2)))
        {
            taskItem.state = EStateTask::RUN_STOPPED;
            if (WIFSTOPPED(wstatus1))
                std::cout << "[" << pid1 << "] is stopped\n";
            else
                std::cout << "[" << pid2 << "] is stopped\n";
        }
        // from RUN_STOPPED to other state
        else if (taskItem.state == EStateTask::RUN_STOPPED &&
                 (WIFSTOPPED(wstatus1) || WIFSTOPPED(wstatus2)))
        {
            taskItem.state = EStateTask::STOPPED;
            if (WIFSTOPPED(wstatus1))
                std::cout << "[" << pid1 << "] is stopped\n";
            else
                std::cout << "[" << pid2 << "] is stopped\n";
        }
        else if (taskItem.state == EStateTask::RUN_STOPPED &&
                 (WIFCONTINUED(wstatus1) || WIFCONTINUED(wstatus2)))
        {
            taskItem.state = EStateTask::RUN;
            if (WIFCONTINUED(wstatus1))
                std::cout << "[" << pid1 << "] is continued\n";
            else
                std::cout << "[" << pid2 << "] is continued\n";
        }
        // from STOPPED to other state
        else if (WIFCONTINUED(wstatus1) && WIFCONTINUED(wstatus2))
        {
            taskItem.state = EStateTask::RUN;
            std::cout << "[" << pid1 << ", " << pid2 << "] is continued\n";
        }
        else if (taskItem.state == EStateTask::STOPPED &&
                 (WIFCONTINUED(wstatus1) || WIFCONTINUED(wstatus2)))
        {
            taskItem.state = EStateTask::RUN_STOPPED;
            if (WIFCONTINUED(wstatus1))
                std::cout << "[" << pid1 << "] is continued\n";
            else
                std::cout << "[" << pid2 << "] is continued\n";
        }
        // is't changed state
        else
            isChanged = false;

        std::cout.flush();
        assert(sigprocmask(SIG_UNBLOCK, &sigset2_, NULL) == 0);
    };

    auto checkState = [this, &checkUnary, &checkBinary](TaskItem& taskItem,
                                                        bool isAsynk = true) {
        bool isChanged = true;

        if (taskItem.type == ::analyze::ETypeCmdLine::SINGLE)
        {
            auto singleProcess = std::get<single::Single*>(taskItem.task);
            checkUnary(taskItem, isAsynk, isChanged, singleProcess);
        }
        else if (taskItem.type == ::analyze::ETypeCmdLine::PPIPE)
        {
            auto ppipeProcess = std::get<ppipe::Ppipe*>(taskItem.task);
            checkBinary(taskItem, isAsynk, isChanged, ppipeProcess);
        }
        else if (taskItem.type == ::analyze::ETypeCmdLine::BOOLEAN)
        {
            auto booleanProcess = std::get<boolean::Boolean*>(taskItem.task);
            checkUnary(taskItem, isAsynk, isChanged, booleanProcess);
        }

        return isChanged;
    };

    auto asynkWaitTasks = [this, &checkState](void)
    {
        bool isStateChanged = true;
        while (isStateChanged || IS_SIGCHILD_EVENT)
        {
            isStateChanged = false;
            IS_SIGCHILD_EVENT = false;

            for (size_t idx = 0; idx < tasks_.size(); idx++)
                if (tasks_[idx].state != EStateTask::DONE)
                    isStateChanged = checkState(tasks_[idx], true) || isStateChanged;
        }
    };

    assert(sigprocmask(SIG_UNBLOCK, &sigset2_, NULL) == 0);

    asynkWaitTasks();
    if (fgTaskIdx_ != -1)
    {
        assert(fgTaskIdx_ < tasks_.size());
        TaskItem& taskItem = tasks_[fgTaskIdx_];

        while (taskItem.state == EStateTask::RUN)
        {
            checkState(taskItem, false);
            asynkWaitTasks();
        }

        fgTaskIdx_ = -1;
    }

    assert(sigprocmask(SIG_BLOCK, &sigset2_, NULL) == 0);
}

bool Shell::isControlFlowCmd(void) const
{
    std::stringstream sstream(cmdLine_);
    std::vector<std::string> tokens;

    while (!sstream.eof())
    {
        std::string token; sstream >> token;
        if (token == "")
            continue;
        tokens.push_back(token);
    }

    if (tokens.size() != 2)
        return false;
    if (tokens[0] != shell::Shell::fgCmd &&
        tokens[0] != shell::Shell::bgCmd)
        return false;

    auto isNumber = [](std::string const& s)
    {
        for (char ch : s)
            if (std::isdigit(ch) == 0)
                return false;
        return true;
    };

    return isNumber(tokens[1]);
}

void Shell::fg(size_t idx) noexcept
{
    if (idx >= tasks_.size()) return;

    if (tasks_[idx].state != EStateTask::DONE)
    {
        fgTaskIdx_ = idx;
        tasks_[idx].isForeground = true;

        if (tasks_[idx].type == ::analyze::ETypeCmdLine::SINGLE)
        {
            auto singleProcess = std::get<single::Single*>(tasks_[idx].task);
            int pid = singleProcess->getPid();
            tcsetpgrp(0, pid);
            singleProcess->KILL(::process::Process::EKill::CONT);
        }
        else if (tasks_[idx].type == ::analyze::ETypeCmdLine::PPIPE)
        {
            auto ppipeProcess = std::get<ppipe::Ppipe*>(tasks_[idx].task);
            auto [pid, _] = ppipeProcess->getPid();
            tcsetpgrp(0, pid);
            ppipeProcess->KILL(::process::Process::EKill::CONT);
        }
        else if (tasks_[idx].type == ::analyze::ETypeCmdLine::BOOLEAN)
        {
            auto booleanProcess = std::get<boolean::Boolean*>(tasks_[idx].task);
            int pid = booleanProcess->getPid();
            tcsetpgrp(0, pid);
            booleanProcess->KILL(::process::Process::EKill::CONT);
        }
    }
}

void Shell::bg(size_t idx) noexcept
{
    if (idx >= tasks_.size()) return;

    if (tasks_[idx].state != EStateTask::DONE)
    {
        tasks_[idx].isForeground = false;

        if (tasks_[idx].type == ::analyze::ETypeCmdLine::SINGLE)
        {
            auto singleProcess = std::get<single::Single*>(tasks_[idx].task);
            singleProcess->KILL(::process::Process::EKill::CONT);
        }
        else if (tasks_[idx].type == ::analyze::ETypeCmdLine::PPIPE)
        {
            auto ppipeProcess = std::get<ppipe::Ppipe*>(tasks_[idx].task);
            ppipeProcess->KILL(::process::Process::EKill::CONT);
        }
        else if (tasks_[idx].type == ::analyze::ETypeCmdLine::BOOLEAN)
        {
            auto booleanProcess = std::get<boolean::Boolean*>(tasks_[idx].task);
            booleanProcess->KILL(::process::Process::EKill::CONT);
        }
    }
}


