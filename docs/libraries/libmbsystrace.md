# libmbsystrace

libmbsystrace is a library for tracing and manipulating kernel interactions of a process. This includes things like making system calls and delivery of signals. Tracing of mixed ABI programs is supported. Thus, libmbsystrace supports tracing:

* native ABI programs (eg. 64-bit program on 64-bit host)
* compatible ABI programs (eg. 32-bit programs on 64-bit hosts)
* mixed ABI programs (eg. an assembly program that executes both x96_64's `syscall` and x86_32's `sysenter`)

Userspace tracing, like stepping through assembly instructions are not supported by the library.

## Example

This example will trace and print out the syscalls made by a forked process that prints `Hello, world!\n`.

```cpp
#include <cstdio>
#include <cstdlib>

#include <mbsystrace/hooks.h>
#include <mbsystrace/tracer.h>
#include <mbsystrace/tracee.h>

int main(int argc, char *argv[])
{
    Hooks hooks;

    hooks.syscall_entry = [](auto tracee, auto &info) -> SysCallEntryAction {
        printf("[%d] Entering syscall: %s\n", tracee->tid, info.syscall.name());
        return action::Default{};
    };

    hooks.syscall_exit = [](auto tracee, auto &info) -> SysCallExitAction {
        printf("[%d] Exiting syscall: %s\n", tracee->tid, info.syscall.name());
        return action::Default{};
    };

    Tracer tracer;

    if (auto r = tracer.fork([&] { printf("Hello, world!\n"); }); !r) {
        fprintf(stderr, "Failed to create process: %s\n",
                r.error().message().c_str());
        return EXIT_FAILURE;
    }

    if (auto r = tracer.execute(hooks); !r) {
        fprintf(stderr, "Error in tracer: %s\n",
                r.error().message().c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

For more complete examples, see:
* [`mbtrace`](../../examples/mbtrace.cpp) - a sample command-line interface for libmbsystrace that prints out a message for each type of hook
* [libmbsystrace tests](../../libmbsystrace/tests) - the tests include examples of how to use nearly every part of the library
