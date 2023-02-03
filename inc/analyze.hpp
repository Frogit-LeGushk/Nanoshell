#pragma once
#include "single.hpp"
#include "ppipe.hpp"
#include "boolean.hpp"
#include <variant>
#include <optional>

namespace analyze {

enum class ETypeCmdLine : uint8_t
{
    SINGLE,
    PPIPE,
    BOOLEAN,
    UNKNOWN
};

using task_t        = std::variant<single::Single*,ppipe::Ppipe*,boolean::Boolean*>;
using pairTask_t    = std::pair<task_t, bool>;
using optPairTask_t = std::optional<pairTask_t>;

ETypeCmdLine    analyzeCmdLine  (std::string const& cmdLine) noexcept;
optPairTask_t   createTask      (std::string const& cmdLine, ETypeCmdLine typeCmdLine) noexcept;

} // namespace analyze
