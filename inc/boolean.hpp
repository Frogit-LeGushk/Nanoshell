#pragma once
#include "process.hpp"

namespace boolean {

class Boolean
{
    using argv_t    = ::process::Process::argv_t;
    using Process   = ::process::Process;
    using EKill     = ::process::Process::EKill;
    static constexpr const int successStatus = ::process::Process::successStatus;
    static constexpr const int failureStatus = ::process::Process::failureStatus;

public:
    enum class EOper : uint8_t { AND, OR };

    Boolean(argv_t const& argv1, argv_t const& argv2,
            bool isForeground = true, EOper oper = EOper::AND) noexcept;
    ~Boolean(void) noexcept;

    int                     getPid(void)                const noexcept;
    std::pair<int,int>      getPairPid(void)            const noexcept;
    void KILL               (EKill sig = EKill::INT)    const noexcept;
    bool isDone             (bool isAsynk = true,
                             int * pwstatus = nullptr)  noexcept;
    bool isSuccess          (void)                      noexcept;
    std::pair<bool,bool>    isTermBySig(void)           noexcept;

private:
    bool isNeedSecondProcess_   (int status)const noexcept;
    void createSecondProcess_   (void)      noexcept;

private:
    argv_t          argv2_;
    const   bool    isForeground_;
    const   EOper   oper_;
    const   int     termPid_;
    Process * process1_ = nullptr;
    Process * process2_ = nullptr;
    bool isDone_        = false;
};

std::pair<Boolean *,bool> make_boolean(std::string const& cmdLine);

} // namespace boolean

