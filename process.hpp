#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace process {

struct Process
{
    using argv_t    = std::vector<std::string>;
    using stdfds_t  = std::array<int, 3>;
    using clsfds_t  = std::array<int, 3>;

    static constexpr const size_t maxArgc       = 256;
    static constexpr const stdfds_t defStdFds   = {-1, -1, -1};
    static constexpr const clsfds_t defClsFds   = {-1, -1, -1};

    enum class EKill : uint8_t
    {
        HUP, INT, QUIT, TSTP, TTIN, TTOU, TERM
    };

    explicit Process(argv_t const& argv,
                     stdfds_t const& stdFds = defStdFds,
                     clsfds_t const& clsFds = defClsFds) noexcept;
    explicit Process(argv_t && argv,
                     stdfds_t const& stdFds = defStdFds,
                     clsfds_t const& clsFds = defClsFds) noexcept;

    Process(Process const& process)             = delete;
    Process(Process && process)                 = delete;
    Process operator=(Process const& process)   = delete;
    Process operator=(Process && process)       = delete;

    ~Process() noexcept;

    int             getPid              (void)                  const noexcept;
    bool            isDone              (bool isAsynk = true)   noexcept;
    bool            isTermBySig         (void)                  const noexcept;
    int             join                (void)                  noexcept;
    void            KILL                (EKill sig)             const noexcept;
    argv_t const&   getArgv             (void)                  const noexcept;

private:
    using signature_t       = int(Process::argv_t const&);
    using callback_t        = std::function<signature_t>;
    using map_callbacks_t   = std::unordered_map<std::string, callback_t>;

    static constexpr const char * notFoundSym       = "notFound";
    static constexpr const char * fileSharedLib     = "./map_callbacks.so";
    static constexpr const char * fileSymSharedLib  = "map_callbacks.txt";
    static constexpr const int  STACK_SIZE_         = 2 * 1024 * 1024; // = 2 MiB

    void Process_       (void) noexcept;
    void ProcessClone_  (void) noexcept;
    void ProcessExec_   (void) noexcept;
    void setStdFds_     (void) noexcept;

    static bool             checkSymMapCallbacks_(std::string const& sym)noexcept;
    static map_callbacks_t* mapCallbacks_       (map_callbacks_t * mapCallback = nullptr) noexcept;
    static void             initMapCallbacks_   (void)          noexcept;
    static int              routine_            (void * arg)    noexcept;


    char* const* makeExecArgv_(argv_t const& argv) const noexcept;

    template<typename Exec>
    void exec_(argv_t const& argv, Exec&& exec) noexcept
    {
        const auto exec_argv = makeExecArgv_(argv);

        if (exec(exec_argv[0], exec_argv) == -1)
        {
            perror("exec");
            exit(EXIT_FAILURE);
        }
    }

    struct routineArg_
    {
        explicit routineArg_(Process * process, callback_t * callback) noexcept;
        Process * process;
        callback_t * callback;
    };

private:
    argv_t argv_;
    stdfds_t stdfds_= defStdFds;
    clsfds_t clsfds_= defClsFds;
    int pid_        = -1;
    int status_     = -1;
    bool isDone_    = false;
    bool isTermBySig_ = false;
    char * STACK_   = nullptr;
};

Process * make_process(std::string const& cmdLine);

} // namespace process
