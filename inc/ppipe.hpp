#pragma once
#include "process.hpp"

namespace ppipe {

class Ppipe
{
    using pipe_t    = int[2];
    using argv_t    = ::process::Process::argv_t;
    using Process   = ::process::Process;
    using EKill     = ::process::Process::EKill;
    static constexpr const int successStatus = ::process::Process::successStatus;
    static constexpr const int failureStatus = ::process::Process::failureStatus;

public:
    Ppipe(argv_t const& argv1, argv_t const& argv2, bool isForeground = true) noexcept;
    ~Ppipe(void) noexcept;

    std::pair<int,int>      getPid(void)                const noexcept;
    void                    KILL(EKill sig = EKill::INT)const noexcept;
    bool                    isDone(bool isAsynk = true,
                                   std::pair<int*,int*> pwstatus
                                   = {nullptr, nullptr}) noexcept;
    bool                    isSuccess(void)             noexcept;
    std::pair<bool,bool>    isTermBySig(void)           noexcept;
    std::pair<int,int>      join(void)                  noexcept;

private:
    const bool isForeground_;
    const int termPid_;
    pipe_t pipe_        = {-1, -1};
    Process * process1_ = nullptr;
    Process * process2_ = nullptr;
    bool isClosedPipe_  = false;
};

std::pair<Ppipe *,bool> make_ppipe(std::string const& cmdLine);

} // namespace pipe
