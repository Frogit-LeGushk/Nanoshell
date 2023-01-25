#pragma once

#include <vector>
#include <string>
#include <signal.h>

#include "process.hpp"

namespace boolean {

class Boolean
{
    using argv_t    = ::process::Process::argv_t;
    using Process   = ::process::Process;
    using EKill     = ::process::Process::EKill;
    static constexpr const int successStatus = 0;
    static constexpr const int failureStatus = 1;

public:
    enum class EOper : uint8_t
    {
        AND, OR
    };

    Boolean(argv_t const& argv1, argv_t const& argv2,
            EOper oper, bool isForeground = true) noexcept;
    ~Boolean(void) noexcept;

    bool isDone     (bool isAsynk = true)   const noexcept;
    bool isSuccess  (void)                  const noexcept;
    void KILL       (EKill sig)             const noexcept;

private:
    bool isNeedSecondProcess_   (int status)    const noexcept;
    bool tryProcessForeground_  (int pid)       const noexcept;
    void createSecondProcess    (void)          const noexcept;

struct raiiSigProcMask
{
    explicit raiiSigProcMask(Boolean const& boolean) noexcept : boolean_(boolean)
        { sigprocmask(SIG_BLOCK, &boolean_.sigset_, NULL); }
    ~raiiSigProcMask(void) noexcept
        { sigprocmask(SIG_UNBLOCK, &boolean_.sigset_, NULL); }

    static void initProcMask(Boolean& boolean) noexcept
    {
        sigemptyset(&boolean.sigset_);
        sigaddset(&boolean.sigset_, SIGTTIN);
        sigaddset(&boolean.sigset_, SIGTTOU);
    }
private:
    Boolean const& boolean_;
};

public:
    mutable argv_t  argv2_;
    const   EOper   oper_;
    const   bool    isForeground_;
    const   int     termPid_;

    Process * process1_         = nullptr;
    mutable Process * process2_ = nullptr;
    mutable bool isDone_        = false;
    sigset_t sigset_;
};

} // namespace boolean

