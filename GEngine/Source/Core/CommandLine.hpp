#pragma once

#include <format>
#include <span>
#include <variant>

namespace GEngine::CommandLine {

struct Argument {
    std::variant<bool*, int*, float*> pVar;
    const wchar_t* name;
};

/// Parse command line arguments.
inline void Parse(std::span<const Argument> args) {
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    struct ArgvGuard {
        wchar_t** ptr;
        ~ArgvGuard() {
            if (ptr)
                ::LocalFree(ptr);
        }
    } guard{argv};

    // Convert wide string to UTF-8 narrow string.
    auto toNarrow = [](const wchar_t* w) {
        int len = ::WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        std::string s(len - 1, '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), len, nullptr, nullptr);
        return s;
    };

    for (int i{}; i < argc; ++i) {
        for (const Argument& arg : args) {
            if (::wcscmp(argv[i], arg.name) != 0)
                continue;

            std::visit(
                [&](auto* param) {
                    using T = std::decay_t<decltype(*param)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        *param = true;
                    } else {
                        if (i + 1 < argc) {
                            if constexpr (std::is_same_v<T, int>) {
                                *param = ::_wtoi(argv[i + 1]);
                            } else if constexpr (std::is_same_v<T, float>) {
                                *param = static_cast<float>(::_wtof(argv[i + 1]));
                            } else {
                                static_assert(false, "Unsupported type");
                            }
                            ++i;
                        } else {
                            throw std::runtime_error(std::format("Missing value for argument: {}", toNarrow(arg.name)));
                        }
                    }
                },
                arg.pVar);
            break;
        }
    }
}

} // namespace GEngine::CommandLine
