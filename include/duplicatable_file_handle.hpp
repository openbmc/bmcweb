// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include <boost/beast/core/file_posix.hpp>

struct DuplicatableFileHandle
{
    boost::beast::file_posix fileHandle;

    DuplicatableFileHandle() = default;
    DuplicatableFileHandle(DuplicatableFileHandle&&) noexcept = default;
    // Overload copy constructor, because posix doesn't have dup(), but linux
    // does
    DuplicatableFileHandle(const DuplicatableFileHandle& other)
    {
        fileHandle.native_handle(dup(other.fileHandle.native_handle()));
    }
    DuplicatableFileHandle& operator=(const DuplicatableFileHandle& other)
    {
        if (this == &other)
        {
            return *this;
        }
        fileHandle.native_handle(dup(other.fileHandle.native_handle()));
        return *this;
    }
    DuplicatableFileHandle&
        operator=(DuplicatableFileHandle&& other) noexcept = default;
    ~DuplicatableFileHandle() = default;
};
