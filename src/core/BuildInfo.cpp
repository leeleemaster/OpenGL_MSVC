#include "core/BuildInfo.h"

#ifndef DENTALVIZ_VERSION
#define DENTALVIZ_VERSION "unknown"
#endif

namespace dentalviz
{
std::string_view projectVersion() noexcept
{
    return DENTALVIZ_VERSION;
}
} // namespace dentalviz
