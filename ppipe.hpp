#pragma once

#include <vector>
#include <string>

#include "process.hpp"

namespace ppipe {

class Ppipe
{
    using argv_t    = ::process::Process::argv_t;
    using pipe_t    = int[2];
    using Process   = ::process::Process;
    using EKill     = ::process::Process::EKill;
    static constexpr const int successStatus = 0;

public:
    Ppipe(argv_t const& argv1, argv_t const& argv2, bool isForeground = true) noexcept;
    ~Ppipe(void) noexcept;

    std::pair<int,int>      getPids(void)       const noexcept;
    std::pair<bool,bool>    isTermBySig(void)   const noexcept;
    std::pair<int,int>      getStatuses(void)   const noexcept;
    void                    KILL(EKill sig)     const noexcept;

    bool                    isDone(bool isAsynk = true) const noexcept;
    bool                    isSuccess(void)     const noexcept;

private:
    bool isForeground_;
    int termPid_;
    pipe_t pipe_ = {-1, -1};
    Process * process1_ = nullptr;
    Process * process2_ = nullptr;
    sigset_t sigset_;
    mutable bool isClosedPipe_ = false;
};

} // namespace pipe
