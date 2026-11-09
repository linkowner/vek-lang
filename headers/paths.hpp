#pragma once

// Path resolution helpers for the VekLang toolchain.
//
// Module and output paths follow the source file being compiled, not the
// current working directory. Stdlib modules are searched next to the main
// source first, then next to the compiler executable itself.

#include <string>
#include <filesystem>

#if defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <cstdint>
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace paths {
namespace fs = std::filesystem;

// Directory of the running executable. Falls back to the CWD when the
// platform does not provide a way to determine it.
inline std::string exe_dir() {
#if defined(__linux__)
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return fs::path(buf).parent_path().string();
    }
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        std::error_code ec;
        fs::path resolved = fs::canonical(buf, ec);
        if (!ec) return resolved.parent_path().string();
        return fs::path(buf).parent_path().string();
    }
#elif defined(_WIN32)
    char buf[4096];
    DWORD len = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    if (len > 0 && len < sizeof(buf)) {
        return fs::path(buf).parent_path().string();
    }
#endif
    return fs::current_path().string();
}

// Directory containing the given file path. Returns "." for bare filenames
// so it can always be joined with relative paths.
inline std::string dir_of(const std::string& path) {
    fs::path parent = fs::path(path).parent_path();
    return parent.empty() ? std::string(".") : parent.string();
}

} // namespace paths
