#include "../inc/single.hpp"

#include <unistd.h>
#include <cassert>
#include <sstream>

using namespace single;

Single::Single(argv_t const& argv, bool isForeground) noexcept
    : Process(argv), isForeground_(isForeground), termPid_(getpid())
{
    setpgid(getPid(), getPid());

    if (isForeground_)
        tcsetpgrp(0, getPid());
    else
        tcsetpgrp(0, termPid_);
}

Single::~Single(void) noexcept
{
    join();

    if (isForeground_)
        tcsetpgrp(0, termPid_);
}

std::pair<Single *,bool> single::make_single(std::string const& cmdLine)
{
    assert(cmdLine.size() > 0);

    std::stringstream sstream(cmdLine);
    std::vector<std::string> argv;
    bool isForeground = true;

    while (!sstream.eof())
    {
        std::string token; sstream >> token;
        if (token == "")
            continue;
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

        argv.push_back(token);
    }

    Single * singleProcess = new Single(argv, isForeground);
    return std::make_pair(singleProcess, isForeground);
}
