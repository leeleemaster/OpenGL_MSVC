#include "platform/NativeFileDialog.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>

#include <array>
#include <stdexcept>
#include <string>

namespace dentalviz {

std::optional<std::filesystem::path> chooseMeshFile()
{
    std::array<wchar_t, 32'768> pathBuffer{};
    constexpr wchar_t filter[] =
        L"Dental meshes (*.stl;*.obj)\0*.stl;*.obj\0"
        L"STL meshes (*.stl)\0*.stl\0"
        L"OBJ meshes (*.obj)\0*.obj\0"
        L"All files (*.*)\0*.*\0";

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = pathBuffer.data();
    dialog.nMaxFile = static_cast<DWORD>(pathBuffer.size());
    dialog.lpstrDefExt = L"stl";
    dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&dialog) != FALSE) {
        return std::filesystem::path(pathBuffer.data());
    }

    const DWORD errorCode = CommDlgExtendedError();
    if (errorCode == 0) {
        return std::nullopt;
    }
    throw std::runtime_error(
        "Windows file dialog failed with code " + std::to_string(errorCode) + '.');
}

} // namespace dentalviz
