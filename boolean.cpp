#include "boolean.hpp"

#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <sstream>

using namespace boolean;


Boolean::Boolean(argv_t const& argv1, argv_t const& argv2, bool isForeground, EOper oper) noexcept
    : argv2_(argv2), isForeground_(isForeground), oper_(oper), termPid_(getpid())
{
    try
    {
        process1_ = new Process(argv1);
    }
    catch (std::bad_alloc const& err)
    {
        process::PRINT_ERR(err.what());
        exit(EXIT_FAILURE);
    }

    setpgid(process1_->getPid(), process1_->getPid());

    if (isForeground_)
        tcsetpgrp(0, process1_->getPid());
    else
        tcsetpgrp(0, termPid_);
}

Boolean::~Boolean(void) noexcept
{
    assert(process1_);
    isSuccess(); // wait

    delete process1_;
    if (process2_ != nullptr)
        delete process2_;

    if (isForeground_)
        tcsetpgrp(0, termPid_);
}

bool Boolean::isDone(bool isAsynk, int * pwstatus) noexcept
{
    assert(process1_);
    const bool isDone1  = process1_->isDone(isAsynk, pwstatus);
    bool isDone2        = false;

    if (isDone1)
    {
        const int status1 = process1_->join();

        if (process2_ != nullptr)
        {
            isDone2 = process2_->isDone(isAsynk, pwstatus);
        }
        else if (isNeedSecondProcess_(status1))
        {
            createSecondProcess_();
            isDone2 = process2_->isDone(isAsynk, pwstatus);
        }
        else
        {
            isDone2 = true;
        }
    }

    isDone_ = isDone1 && isDone2;
    return isDone_;
}

int Boolean::getPid(void) const noexcept
{
    if (process2_)
        return process2_->getPid();
    else
        return process1_->getPid();
}

std::pair<int,int> Boolean::getPairPid(void) const noexcept
{
    if (process2_)
        return std::make_pair(process1_->getPid(), process2_->getPid());
    else
        return std::make_pair(process1_->getPid(), -1);
}

bool Boolean::isSuccess(void) noexcept
{
    assert(process1_);
    const int status1   = process1_->join();
    int status2         = failureStatus;

    if (!isDone_)
    {
        if (isNeedSecondProcess_(status1))
            createSecondProcess_();
        if (process2_)
            status2 = process2_->join();
        isDone_ = true;
    }
    else if (process2_ != nullptr)
    {
        status2 = process2_->join();
    }

    if (oper_ == EOper::AND)
        return (status1 == successStatus) && (status2 == successStatus);
    else
        return (status1 == successStatus) || (status2 == successStatus);
}

std::pair<bool,bool> Boolean::isTermBySig(void) noexcept
{
    isSuccess(); // wait
    const bool isTermBySig2 = process2_ ? process2_->isTermBySig() : false;
    return std::make_pair(process1_->isTermBySig(), isTermBySig2);
}

void Boolean::KILL(EKill sig) const noexcept
{
    // at one time work only one process
    // or process1_ or process2_
    if (process2_ != nullptr)
        process2_->KILL(sig);
    else
        process1_->KILL(sig);
}

bool Boolean::isNeedSecondProcess_(int status) const noexcept
{
    return  ((oper_ == EOper::AND && status == successStatus) ||
             (oper_ == EOper::OR && status == failureStatus)) &&
            (process2_ == nullptr);
}

void Boolean::createSecondProcess_(void) noexcept
{
    try
    {
        process2_ = new Process(std::move(argv2_));
        setpgid(process2_->getPid(), process2_->getPid());
    }
    catch (std::bad_alloc const& err)
    {
        process::PRINT_ERR(err.what());
        exit(EXIT_FAILURE);
    }

    if (isForeground_)
        tcsetpgrp(0, process2_->getPid());
}

std::pair<Boolean *,bool> boolean::make_boolean(std::string const& cmdLine)
{
    assert(cmdLine.size() > 0);

    std::stringstream sstream(cmdLine);
    std::vector<std::string> argv1;
    std::vector<std::string> argv2;

    bool isSecondPart   = false;
    bool isForeground   = true;
    Boolean::EOper oper = Boolean::EOper::AND;

    while (!sstream.eof())
    {
        std::string token; sstream >> token;
        if (token == "")
            continue;
        if (token == "||" || token == "&&")
        {
            isSecondPart = true;
            if (token == "||")
                oper = Boolean::EOper::OR;
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

    Boolean * booleanProcess = new Boolean(argv1, argv2, isForeground, oper);
    return std::make_pair(booleanProcess, isForeground);
}



