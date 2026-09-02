#pragma once

#define GE_DEFAULT_COPY(ClassName)                                                                                     \
    ClassName(const ClassName&) = default;                                                                             \
    ClassName& operator=(const ClassName&) = default;

#define GE_DEFAULT_MOVE(ClassName)                                                                                     \
    ClassName(ClassName&&) noexcept = default;                                                                         \
    ClassName& operator=(ClassName&&) noexcept = default;

#define GE_DEFAULT_COPY_AND_MOVE(ClassName)                                                                            \
    GE_DEFAULT_COPY(ClassName)                                                                                         \
    GE_DEFAULT_MOVE(ClassName)

#define GE_NO_COPY(ClassName)                                                                                          \
    ClassName(const ClassName&) = delete;                                                                              \
    ClassName& operator=(const ClassName&) = delete;

#define GE_NO_MOVE(ClassName)                                                                                          \
    ClassName(ClassName&&) = delete;                                                                                   \
    ClassName& operator=(ClassName&&) = delete;

#define GE_NO_COPY_NO_MOVE(ClassName)                                                                                  \
    GE_NO_COPY(ClassName)                                                                                              \
    GE_NO_MOVE(ClassName)
