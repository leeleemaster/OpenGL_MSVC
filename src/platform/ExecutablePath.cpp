#include "platform/ExecutablePath.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <stdexcept>
#include <string>

namespace dentalviz {

std::filesystem::path executableDirectory()
{
    std::wstring pathBuffer(512, L'\0');
    while (true) {
        SetLastError(ERROR_SUCCESS);
        const DWORD copiedCharacters = GetModuleFileNameW(
            nullptr,
            pathBuffer.data(),
            static_cast<DWORD>(pathBuffer.size()));
        if (copiedCharacters == 0) {
            throw std::runtime_error(
                "Windows could not determine the DentalViz executable path.");
        }
        if (copiedCharacters < pathBuffer.size()) {
            pathBuffer.resize(copiedCharacters);
            return std::filesystem::path(pathBuffer).parent_path();
        }
        pathBuffer.resize(pathBuffer.size() * 2);
    }
}

} // namespace dentalviz
