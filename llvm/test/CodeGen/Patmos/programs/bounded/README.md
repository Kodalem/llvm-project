# Bounded Program Tests

Tests programs that are WCET analyzable.

Non-exauhstive list of requirements for these programs:

- All loops must have a maximum bound
- No recursion

The command `%test_execution` is provided for use in tests.
It compiles the LLVM IR program and ensures that they execute correctly.

It compiles three different versions of each program:

1. Compiled normally.
1. Compiled using single-path code.
1. Compiled using single-path code with VLIW disabled.

For the last two versions, it also ensures that valid single-path code is generated.
It does so by using `pasim` to get statistics on the execution of the program on different input. 
It then checks the statistics are identical across executions, which is the fundamental characteristic of single-path. 

The command is intended to be used in `llvm-lit` tests, and be called as part of the `; RUN:` command in the tests. 
E.g. `; EXEC_ARGS="0=0 1=1 2=2" ; %test_execution`.

The `EXEC_ARGS` variable must be use to give at least two _execution arguments_.
An execution argument is a number, followed by `=`, followed by another number.
The first number is the input to the `main` function in the tested program.
The second number must be between 0 and 256 and is the expected output of the test for that specific input.

If LLC's `--debug` output is needed, the `WITH_DEBUG` variable can be set to `true`.
This will make the tests produce debug output in a `.debug` file in the build folder (alongside where the compiled test programs are put).
See the error output of a test (which has this variable set) to see exactly where the file is.
