#include "process.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>

using namespace process;


/// Below public interface implementation

Process::Process(argv_t const& argv, stdfds_t const& stdFds, clsfds_t const& clsFds) noexcept
    : argv_(argv), stdfds_(stdFds), clsfds_(clsFds)
{
    Process_();
}

Process::Process(argv_t && argv, stdfds_t const& stdFds, clsfds_t const& clsFds) noexcept
    : argv_(std::move(argv)), stdfds_(stdFds), clsfds_(clsFds)
{
    Process_();
}

Process::~Process() noexcept
{
    if (!isDone_)
        join();
    if (STACK_ != nullptr)
        delete STACK_;
}

int Process::getPid(void) const noexcept
{
    return pid_;
}

bool Process::isDone(bool isAsynk) noexcept
{
    if (isDone_)
        return true;

    int wstatus = 0;
    int wret = 0;

    if ((wret = waitpid(pid_, &wstatus, isAsynk ? WNOHANG : 0)) == -1)
    {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    if (wret == pid_ && (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)))
    {
        isDone_ = true;

        if (WIFEXITED(wstatus))
            status_     = WEXITSTATUS(wstatus);
        else if (WIFSIGNALED(wstatus))
        {
            status_     = WTERMSIG(wstatus);
            isTermBySig_= true;
        }
    }

    return isDone_;
}

bool Process::isTermBySig(void) const noexcept
{
    return isTermBySig_;
}

int Process::join(void) noexcept
{
    if (isDone_)
        return status_;

    int wstatus = 0;

    do
    {
        if (waitpid(pid_, &wstatus, 0) == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }
    while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

    isDone_ = true;

    if (WIFEXITED(wstatus))
        status_     = WEXITSTATUS(wstatus);
    else
    {
        status_     = WTERMSIG(wstatus);
        isTermBySig_= true;
    }

    return status_;
}

void Process::KILL(EKill sig) const noexcept
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

    if (kill(pid_, signal) == -1)
    {
        perror("kill");
        exit(EXIT_FAILURE);
    }
}

Process::argv_t const& Process::getArgv(void) const noexcept
{
    return argv_;
}

bool Process::checkSymMapCallbacks_(std::string const& sym) noexcept
{
    if (Process::mapCallbacks_() == nullptr)
        Process::initMapCallbacks_();

    const auto& mapCallbacks = *Process::mapCallbacks_();
    return mapCallbacks.find(sym) != mapCallbacks.end();
}


/// Below private interface implementation

void Process::Process_(void) noexcept
{
    assert(0 < argv_.size() && argv_.size() <= maxArgc);
    assert(0 < argv_[0].size());

    if (Process::mapCallbacks_() == nullptr)
        Process::initMapCallbacks_();

    const std::string name = argv_[0];

    if (Process::checkSymMapCallbacks_(name))
        ProcessClone_();
    else
        ProcessExec_();
}

void Process::ProcessClone_(void) noexcept
{
    auto& mapCallbacks  = *Process::mapCallbacks_();
    const auto& name    = argv_[0];
    const auto callback = Process::checkSymMapCallbacks_(name)
        ? &mapCallbacks[name]
        : &mapCallbacks[Process::notFoundSym];

    STACK_          = new char [STACK_SIZE_];
    void * arg      = new routineArg_(this, callback);
    const int FLAFS = CLONE_FS | SIGCHLD;

    if ((pid_ = clone(&routine_, STACK_ + STACK_SIZE_, FLAFS, arg)) == -1)
    {
        perror("clone");
        exit(EXIT_FAILURE);
    }
}

void Process::ProcessExec_(void) noexcept
{
    if ((pid_ = fork()) == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid_ == 0)  // child
    {
        setStdFds_();

        if (argv_[0][0] == '/' || argv_[0][0] == '.')
            exec_(argv_, execv);
        else
            exec_(argv_, execvp);
    }
}

void Process::setStdFds_(void) noexcept
{
    // descriptors for std i/o
    for (int i = 0; i < (int)stdfds_.size(); i++)
    {
        if (stdfds_[i] != -1)
        {
            assert(close(i) != -1);
            assert(dup2(stdfds_[i], i) != -1);
        }
    }

    // descriptors which just need to close (for example pipe fd)
    for (int i = 0; i < (int)clsfds_.size(); i++)
    {
        if (clsfds_[i] != -1)
        {
            assert(close(clsfds_[i]) != -1);
        }
    }
}

Process::map_callbacks_t * Process::mapCallbacks_(Process::map_callbacks_t * mapCallback) noexcept
{
    static Process::map_callbacks_t * mapCallback_ = nullptr;

    if (mapCallback != nullptr)
        mapCallback_ = mapCallback;

    return mapCallback_;
}

void Process::initMapCallbacks_(void) noexcept
{
    auto mapCallbacks = new map_callbacks_t();

    std::ifstream input(Process::fileSymSharedLib);
    assert(input.good());

    auto dlCancellationPoint = [](bool condition)
    {
        if (condition)
        {
            std::cerr << dlerror() << std::endl;
            exit(EXIT_FAILURE);
        }
    };

    dlerror();
    void * handle = dlopen(Process::fileSharedLib, RTLD_LAZY);
    dlCancellationPoint(handle == NULL);

    std::string name;
    while (getline(input, name))
    {
        dlerror();
        void * callback = dlsym(handle, name.c_str());
        dlCancellationPoint(callback == NULL);

        const auto callbackItem = std::make_pair(name, (signature_t *)callback);
        const auto [_, isSuccess] = mapCallbacks->insert(callbackItem);
        assert(isSuccess);
    }

    Process::mapCallbacks_(mapCallbacks);
}

char* const* Process::makeExecArgv_(argv_t const& argv) const noexcept
{
    char const ** argv_ = new char const * [maxArgc + 1];

    for (size_t arg = 0; arg < argv.size(); arg++)
        argv_[arg] = argv[arg].c_str();
    argv_[argv.size()] = NULL;

    return const_cast<char* const*>(argv_);
}

int Process::routine_(void * arg) noexcept
{
    auto [process, callback] = *(routineArg_ *)arg;

    process->setStdFds_();
    int status = (*callback)(process->argv_);

    delete (routineArg_ *)arg;
    return status;
}

Process::routineArg_::routineArg_(Process * process, callback_t * callback) noexcept
    : process(process), callback(callback)
{
    assert(process);
    assert(callback);
}


/// Below public namespace function's implementation

Process * process::make_process(std::string const& cmdLine)
{
    assert(cmdLine.size() > 0);

    std::stringstream sstream(cmdLine);
    std::vector<std::string> argv;

    while (!sstream.eof())
    {
        std::string token; sstream >> token;
        argv.push_back(token);
    }

    Process * process = new Process(argv);
    return process;
}











