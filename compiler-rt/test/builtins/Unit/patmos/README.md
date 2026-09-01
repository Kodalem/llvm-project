# Patmos Builitins Tests

Tests of the Patmos builtins.

Each `.c` file is a test of a builtin function.
The tests can use the `llvm-lit` `%test-patmos-librt` command to execute and must have a `main` function that takes no arguments and returns 0 upon success.
`%test-patmos-librt` compiles the file and links it with the Patmos `librt` (and a start function) and executes it on `pasim`.
Upon failure, the tests should use the return code as a guide to where the error occurred.

## Tests that take an input (how to not f- the lit [as hecc] local config fr fr)

Some builtins need to run against a range of input values.
The input does not arrive on the command line.
It is the symbol `input`, defined at link time, and
`llvm/test/CodeGen/Patmos/programs/bounded/_start.ll` passes that symbol's *address* to
`main`.
Defining `input` as `100` makes `main` receive `100`.

The linker needs that value, but a `RUN:` line can only append arguments to the end of a
substitution, so building and running are separate:

```c
// RUN: %build-patmos-input-librt --defsym input=100
// RUN: %exec-patmos --print-stats my_builtin 2>&1 | \
// RUN: FileCheck %s --check-prefixes "CHECK,IN1XX,IN100"
```
`%build-patmos-librt` - compile and link `%s`. Trailing arguments go to the linker.

`%build-patmos-input-librt` - the same, but linked against the start function that  forwards `input` to `main`. Requires `--defsym input=<value>`.

`%exec-patmos` - run the binary just built. Trailing arguments go to `pasim`.

`%test-patmos-librt` - build and run in one step, for tests that take no input.

A build line leaves its binary in place for the `RUN:` lines that follow, so each build
line and the `%exec-patmos` line under it belong together as a pair.

## Why the tests are written this way

These tests used to set their input through a shell variable:

```c
// RUN: INPUT=100; \
// RUN: %test-patmos-input-librt --print-stats my_builtin 2>&1 | \
```

That only works when lit runs the `RUN:` lines through an external shell.
`execute_external=True` is going to be deprecated starting from LLVM23, so it was best
thought long term we should stop using it, henceforth lit's internal
shell reads `INPUT=100;` as a command to run rather than an assignment, so every one of
these tests died before it reached `pasim`.

Passing the value as `--defsym input=100` drops the shell variable, and that is what forced
the single `%test-patmos-input-librt` substitution to split into a build step and an
execution step.

This directory selects no test format of its own; it inherits lit's internal shell from
`compiler-rt/test/lit.common.cfg.py`.  Please... really please, if you try to LLM this stuff or
check the previous commits, do not reintroduce `config.test_format = lit.formats.ShTest(execute_external=True)`,
and do not paper over the deprecation with `force_execute_external=True`.

LLVM's `lit` also applies substitutions in the order they are registered, so an inherited
substitution that is a prefix of a Patmos one will corrupt it. Compiler-rt already defines `%run`, 
which is why the execution substitution is
`%exec-patmos` and not `%run-patmos`.
`lit.local.cfg` checks for this and fails with an explanatory message when a new
substitution would collide. This is quite hacky, but future maintainers (if there will be any)
will not have to deal with cryptic errors - these off-hand comments are vestige of my sanity working on this...
mess...
