#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <assert.h>
#include <signal.h>

#include "boolean.hpp"

using namespace boolean;


Boolean::Boolean(argv_t const& argv1, argv_t const& argv2, EOper oper, bool isForeground) noexcept
    : argv2_(argv2), oper_(oper), isForeground_(isForeground), termPid_(getpid())
{
    process1_ = new Process(argv1);

    setpgid(process1_->getPid(), process1_->getPid());
    tryProcessForeground_(process1_->getPid());

    Boolean::raiiSigProcMask::initProcMask(*this);
}

Boolean::~Boolean(void) noexcept
{
    assert(process1_);

    isSuccess();

    delete process1_;
    if (process2_)
        delete process2_;

    tryProcessForeground_(termPid_);
}

bool Boolean::isDone(bool isAsynk) const noexcept
{
    assert(process1_);
    raiiSigProcMask procMask(*this);

    bool isDone1 = process1_->isDone(isAsynk);
    bool isDone2 = false;

    if (!isDone1)
    {
        return false;
    }
    else
    {
        if (process2_ != nullptr)
        {
            isDone2 = process2_->isDone(isAsynk);
        }
        else if (isNeedSecondProcess_(process1_->join()))
        {
            createSecondProcess();
            isDone2 = process2_->isDone(isAsynk);
        }
        else
        {
            isDone2 = true;
        }
    }

    isDone_ = isDone1 && isDone2;
    return isDone_;
}

bool Boolean::isSuccess(void) const noexcept
{
    assert(process1_);
    raiiSigProcMask procMask(*this);

    int status1 = process1_->join();
    int status2 = failureStatus;

    if (!isDone_)
    {
        if (isNeedSecondProcess_(status1))
        {
            createSecondProcess();
            status2 = process2_->join();
        }

        isDone_ = true;
    }
    else if (process2_ != nullptr)
    {
        status2 = process2_->join();
    }

    tryProcessForeground_(termPid_);

    if (oper_ == EOper::AND)
        return (status1 == successStatus) && (status2 == successStatus);
    else
        return (status1 == successStatus) || (status2 == successStatus);
}

void Boolean::KILL(EKill sig) const noexcept
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

    const int pid = (process2_ == nullptr) ?
        process1_->getPid() : process2_->getPid();

    if (kill(pid, signal) == -1)
    {
        perror("kill");
        exit(EXIT_FAILURE);
    }
}

bool Boolean::isNeedSecondProcess_(int status) const noexcept
{
    return  ((oper_ == EOper::AND && status == successStatus) ||
             (oper_ == EOper::OR && status == failureStatus)) &&
            (process2_ == nullptr);
}

bool Boolean::tryProcessForeground_(int pid) const noexcept
{
    if (isDone_ || !isForeground_)
    {
        tcsetpgrp(0, termPid_);
        return pid == termPid_;
    }
    else
    {
        tcsetpgrp(0, pid);
        return true;
    }
}

void Boolean::createSecondProcess(void) const noexcept
{
    process2_ = new Process(std::move(argv2_));
    setpgid(process2_->getPid(), process2_->getPid());
    tryProcessForeground_(process2_->getPid());
}

