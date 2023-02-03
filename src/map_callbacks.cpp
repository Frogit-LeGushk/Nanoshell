#include "../inc/process.hpp"

#include <iostream>
#include <assert.h>
#include <unistd.h>
#include <cstring>

using namespace process;

extern "C"
{

int notFound(Process::argv_t const& argv) noexcept
{
    assert(argv.size());
    std::cout << argv[0] << ": command not found" << std::endl;
    return 1;
}

int noop(Process::argv_t const& argv)
{
    (void) argv;
    return 0;
}

int cd(Process::argv_t const& argv) noexcept
{
    assert(argv.size() == 2);
    return chdir(argv[1].c_str());
}

int pwd(Process::argv_t const& argv) noexcept
{
    assert(argv.size() == 1);
    char buffer[BUFSIZ] = {0};
    assert(getcwd(buffer, BUFSIZ));
    std::cout << buffer << std::endl;
    return 0;
}

}
