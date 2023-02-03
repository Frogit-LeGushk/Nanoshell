#include "../inc/analyze.hpp"
#include <regex>

using namespace analyze;

namespace {

const std::string patternSpaces =
"^[ ]*";
const std::string patternArgvPrefix_ =
"(([ ]+)((\"[^\"]+\")|([-_\\w\\d.,/]+)))+";
const std::string patternArgvPostfix_ =
"(((\"[^\"]+\")|([-_\\w\\d.,/]+))([ ]+))+";
const std::string patternBackground_ =
"[ ]*( &)?[ ]*$";

const std::string patternSingle =
    patternSpaces                               +
    "((\"[^\"]+\")|([-_\\w\\d.,/]+))"           +
    "(([ ]+)((\"[^\"]+\")|([-_\\w\\d.,/]+)))*"  +
    patternBackground_;

const std::string patternPpipe =
    patternSpaces                       +
    patternArgvPostfix_                 +
    "\\|{1}"                            +
    patternArgvPrefix_                  +
    patternBackground_;

const std::string patternBoolean =
    patternSpaces                       +
    patternArgvPostfix_                 +
    "(&&|\\|\\|)"                       +
    patternArgvPrefix_                  +
    patternBackground_;

} // namespace


ETypeCmdLine analyze::analyzeCmdLine(std::string const& cmdLine) noexcept
{
    static const auto flags =   std::regex_constants::ECMAScript |
                                std::regex_constants::icase;
    static const std::regex regexSingle (patternSingle, flags);
    static const std::regex regexPpipe  (patternPpipe,  flags);
    static const std::regex regexBoolean(patternBoolean,flags);

    auto checkRegEx = [&cmdLine](std::regex const& pattern)
    {
        const auto begin_ = std::sregex_iterator(
            cmdLine.begin(), cmdLine.end(), pattern);
        const auto end_   = std::sregex_iterator();
        const int cnt_    = std::distance(begin_, end_);

        return (bool)(cnt_ == 1);
    };

    if (checkRegEx(regexSingle))
        return ETypeCmdLine::SINGLE;
    else if (checkRegEx(regexPpipe))
        return ETypeCmdLine::PPIPE;
    else if (checkRegEx(regexBoolean))
        return ETypeCmdLine::BOOLEAN;
    else
        return ETypeCmdLine::UNKNOWN;
}

optPairTask_t analyze::createTask(std::string const& cmdLine, ETypeCmdLine typeCmdLine) noexcept
{
    optPairTask_t task_{};

    try
    {
        if (typeCmdLine == ETypeCmdLine::SINGLE)
        {
            std::cout << "SINGLE\n";
            task_ = ::single::make_single(cmdLine);
        }
        else if (typeCmdLine == ETypeCmdLine::PPIPE)
        {
            std::cout << "PPIPE\n";
            task_ = ::ppipe::make_ppipe(cmdLine);
        }
        else if (typeCmdLine == ETypeCmdLine::BOOLEAN)
        {
            std::cout << "BOOLEAN\n";
            task_ = ::boolean::make_boolean(cmdLine);
        }
        else if (typeCmdLine == ETypeCmdLine::UNKNOWN)
        {
            std::cout << "UNKNOWN\n";
            std::cout << "Wrong command's format or bug x_x\n";
        }

        std::cout.flush();
    }
    catch (std::exception const& error)
    {
        ::process::PRINT_ERR(error.what());
        exit(EXIT_FAILURE);
    }

    return task_;
}
