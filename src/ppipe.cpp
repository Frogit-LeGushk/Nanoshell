#include "../inc/ppipe.hpp"

#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <sstream>
#include <signal.h>

using namespace ppipe;

Ppipe::Ppipe(argv_t const& argv1, argv_t const& argv2, bool isForeground) noexcept
    : isForeground_(isForeground), termPid_(getpid())
{
    if (pipe(pipe_) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    auto stdfds1 = Process::defStdFds;
    stdfds1[1] = stdfds1[2] = pipe_[1]; // set stdout and stderr

    auto stdfds2 = Process::defStdFds;
    stdfds2[0] = pipe_[0]; // set stdin

    auto clsfds = Process::defClsFds;
    clsfds[0] = pipe_[0]; // set fd for close in new thread
    clsfds[1] = pipe_[1];

    try
    {
        process1_ = new Process(argv1, stdfds1, clsfds);
        process2_ = new Process(argv2, stdfds2, clsfds);
    }
    catch (std::bad_alloc const& err)
    {
        process::PRINT_ERR(err.what());
        exit(EXIT_FAILURE);
    }

    // create new thread group for term
    setpgid(process1_->getPid(), process1_->getPid());
    setpgid(process2_->getPid(), process1_->getPid());

    // set foreground thread group for term
    if (isForeground_)
        tcsetpgrp(0, process1_->getPid());
    else
        tcsetpgrp(0, termPid_);
}

Ppipe::~Ppipe(void) noexcept
{
    assert(process1_ && process2_);
    join();

    delete process1_;
    delete process2_;

    if (isForeground_)
        tcsetpgrp(0, termPid_);
}

std::pair<int,int> Ppipe::getPid(void) const noexcept
{
    assert(process1_ && process2_);
    return std::make_pair(process1_->getPid(), process2_->getPid());
}

std::pair<bool,bool> Ppipe::isTermBySig(void) noexcept
{
    join(); // wait
    return std::make_pair(process1_->isTermBySig(), process2_->isTermBySig());
}

std::pair<int,int> Ppipe::join(void) noexcept
{
    int status1 = process1_->join();

    if (!isClosedPipe_)
    {
        assert(close(pipe_[1]) != -1);
        assert(close(pipe_[0]) != -1);
        isClosedPipe_ = true;
    }

    int status2 = process2_->join();
    return std::make_pair(status1, status2);
}

void Ppipe::KILL(EKill sig) const noexcept
{
    process1_->KILL(sig);
    process2_->KILL(sig);
}

bool Ppipe::isDone(bool isAsynk, std::pair<int*,int*> pwstatus) noexcept
{
    assert(process1_ && process2_);
    return process1_->isDone(isAsynk, pwstatus.first) &&
           process2_->isDone(isAsynk, pwstatus.second);
}

bool Ppipe::isSuccess(void) noexcept
{
    assert(process1_ && process2_);
    const auto [status1, status2] = join();
    return (status1 == successStatus) && (status2 == successStatus);
}

std::pair<Ppipe *,bool> ppipe::make_ppipe(std::string const& cmdLine)
{
    assert(cmdLine.size() > 0);

    std::stringstream sstream(cmdLine);
    std::vector<std::string> argv1;
    std::vector<std::string> argv2;

    bool isSecondPart   = false;
    bool isForeground   = true;

    while (!sstream.eof())
    {
        std::string token; sstream >> token;
        if (token == "")
            continue;
        if (token == "|")
        {
            isSecondPart = true;
            continue;
        }
        if (token == "&")
        {
            isForeground = false;
            break;
        }
        if (token[0] == '"')
        {
            token.pop_back();
            token.erase(0, 1);
        }

        if (isSecondPart)
            argv2.push_back(token);
        else
            argv1.push_back(token);

    }

    Ppipe * ppipeProcess = new Ppipe(argv1, argv2, isForeground);;
    return std::make_pair(ppipeProcess, isForeground);
}


