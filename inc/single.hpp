#pragma once
#include "process.hpp"

namespace single {

struct Single : public process::Process
{
    Single(argv_t const& argv, bool isForeground = true) noexcept;
    ~Single(void) noexcept;

private:
    const bool  isForeground_;
    const int   termPid_;
};

std::pair<Single *,bool>  make_single(std::string const& cmdLine);

} // namespace single
