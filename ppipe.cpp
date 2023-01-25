#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <assert.h>
#include <signal.h>

#include "ppipe.hpp"

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

    process1_ = new Process(argv1, stdfds1, clsfds);
    process2_ = new Process(argv2, stdfds2, clsfds);

    // create new thread group for term
    setpgid(process1_->getPid(), process1_->getPid());
    setpgid(process2_->getPid(), process1_->getPid());

    // set foreground thread group for term
    if (isForeground_)
        tcsetpgrp(0, process1_->getPid());
    else
        tcsetpgrp(0, termPid_);

    sigemptyset(&sigset_);
    sigaddset(&sigset_, SIGTTIN);
    sigaddset(&sigset_, SIGTTOU);
}

Ppipe::~Ppipe(void) noexcept
{
    assert(process1_ && process2_);
    delete process1_;
    delete process2_;

    if (!isClosedPipe_)
    {
        assert(close(pipe_[1]) != -1);
        assert(close(pipe_[0]) != -1);
    }

    tcsetpgrp(0, termPid_);
}

std::pair<int,int> Ppipe::getPids(void) const noexcept
{
    assert(process1_ && process2_);
    return std::make_pair(process1_->getPid(), process2_->getPid());
}

std::pair<bool,bool> Ppipe::isTermBySig(void) const noexcept
{
    getStatuses(); // wait
    return std::make_pair(process1_->isTermBySig(), process2_->isTermBySig());
}

std::pair<int,int> Ppipe::getStatuses(void) const noexcept
{
    sigprocmask(SIG_BLOCK, &sigset_, NULL);

    int status1 = process1_->join();

    if (!isClosedPipe_)
    {
        assert(close(pipe_[1]) != -1);
        assert(close(pipe_[0]) != -1);
    }

    int status2 = process2_->join();

    auto statuses = std::make_pair(status1, status2);
    tcsetpgrp(0, termPid_);

    isClosedPipe_ = true;
    sigprocmask(SIG_UNBLOCK, &sigset_, NULL);
    return statuses;
}

void Ppipe::KILL(EKill sig) const noexcept
{
    int signal = SIGKILL;

    switch (sig)
    {
    case Process::EKill::HUP:   signal = SIGHUP;    break;
    case Process::EKill::INT:   signal = SIGINT;    break;
    case Process::EKill::QUIT:  signal = SIGQUIT;   break;
    case Process::EKill::TSTP:  signal = SIGTSTP;   break;
    case Process::EKill::TTIN:  signal = SIGTTIN;   break;
    case Process::EKill::TTOU:  signal = SIGTTOU;   break;
    case Process::EKill::TERM:  signal = SIGTERM;   break;
    }

    // see man 2 kill
    if (kill(-(process1_->getPid()), signal) == -1)
    {
        perror("kill");
        exit(EXIT_FAILURE);
    }
}

bool Ppipe::isDone(bool isAsynk) const noexcept
{
    assert(process1_ && process2_);
    sigprocmask(SIG_BLOCK, &sigset_, NULL);
    bool isDone = process1_->isDone(isAsynk) && process2_->isDone(isAsynk);
    sigprocmask(SIG_UNBLOCK, &sigset_, NULL);

    if (isDone)
        tcsetpgrp(0, termPid_);

    return isDone;
}

bool Ppipe::isSuccess(void) const noexcept
{
    assert(process1_ && process2_);
    const auto [status1, status2] = getStatuses();
    return (status1 == successStatus) && (status2 == successStatus);
}




