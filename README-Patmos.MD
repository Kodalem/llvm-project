Patmos LLVM
================

This is a fork of the LLVM Project that adds a target for the Patmos Architecture.

# Installation (Fast way)

Install nix or lix, and then run the following command in your terminal:

```zsh
sudo nix develop github:Kodalem/llvm-project/patmos-llvm-22x-rusty --accept-flake-config
```

Nope, you cannot _not_ `sudo`, you cannot _not_ `--accept-flake-config`, otherwise you will suffer the pain of being installing the compiler for 2 hours.

# Installation

Prebuilt releases are [published on Github](https://github.com/t-crest/patmos-llvm-project/releases) as `.tar.gz` files.
To install, download the tarball that fits your platform into you desired installation folder (usually `t-crest/local`.
Then open a terminal in that folder and run the following command (where `<tarfile>` is the release tar file you downloaded):

```
tar -xvf <tarfile>
```

This will correctly install all artifacts into the right paths. You are now ready to use the compiler.
Remember to set your `PATH` to point to `t-crest/local/bin`.

## Nix Usage (Prefixed Tools)

The flake exports Patmos-specific wrappers so your host LLVM/Clang is not overridden.
Use `patmos-*` commands (`patmos-clang`, `patmos-llc`, `patmos-llvm-objdump`, ...).

### Channels

- `patmos-prefixed` (default): release channel wrapper.

### Quick shell usage

```zsh
# Default channel (currently release wrapper; falls back to source if artifact metadata is not set)
nix shell -p github:Kodalem/llvm-project/patmos-llvm-22x-rusty#patmos-prefixed

# If you are already from the repo
nix develop

# Check that it works
patmos-clang --version
```

### Dev shells

```zsh
# Default dev shell (release wrapper)
nix develop github:Kodalem/llvm-project/patmos-llvm-22x-rusty

# Example compile command
patmos-clang -O2 -c fibonacci-test.cpp -o fibonacci-test.o
```

### Persistent install (AT YER OWN RISK)

```zsh
nix profile install github:Kodalem/llvm-project/patmos-llvm-22x-rusty#patmos-prefixed
patmos-clang --version
```

# Development

### Requirements

Platform:

- Ubuntu 18/20
- MacOs 10.15
- Windows Subsystem for Linux running Ubuntu 18/20

Tools:

- `gcc` v9.1 or `clang` v11.0.0
- `git` v2.17.1
- `cmake` v3.21.0
- `make` v4.1
- GNU tar v1.29 (must be available on the path as `tar`)
- Binutils as default (E.g., `ar` must point to binutils `ar`)
- Patmos Simulator Tools v1.0.2

### Branches

- `master`: Primary development branch.
- `upstream`: Only for tracking the upstream LLVM repository. Should only be used to pull in new versions of LLVM and not to make any changes to them. [See more here](#anch-updating-llvm).

### Cloning

The LLVM project is very large so we can save some time by avoiding cloning the part we don't use for Patmos.
The following commands with clone only the parts of the Patmos LLVM repository that we actually use:

```
git clone --filter=blob:none --sparse git@github.com:t-crest/patmos-llvm-project
cd patmos-llvm-project
git sparse-checkout add /.github /llvm /clang /compiler-rt /lld /libunwind
```

### Build

Ensure all the above requirements are met and that the tools are available on the PATH.

Then you must create a build folder.
This folder doesn't have to be in the LLVM Project root folder, but that is where we will put it in this example.
In this folder, you must setup the build with `cmake` (referencing the LLVM subfolder), after which you can build:

```zsh
mkdir -p build
cd build
cmake ../llvm -DCMAKE_BUILD_TYPE=Debug -DLLVM_TARGETS_TO_BUILD="Patmos" -DLLVM_DEFAULT_TARGET_TRIPLE=patmos-unknown-unknown-elf -DLLVM_ENABLE_PROJECTS="clang;lld" -DCLANG_ENABLE_ARCMT=false -DCLANG_ENABLE_STATIC_ANALYZER=false -DCLANG_BUILD_EXAMPLES=false -DLLVM_ENABLE_BINDINGS=false -DLLVM_INSTALL_BINUTILS_SYMLINKS=false -DLLVM_INSTALL_CCTOOLS_SYMLINKS=false -DLLVM_INCLUDE_EXAMPLES=false -DLLVM_INCLUDE_BENCHMARKS=false -DLLVM_APPEND_VC_REV=false -DLLVM_ENABLE_WARNINGS=false -DLLVM_ENABLE_PEDANTIC=false -DLLVM_ENABLE_LIBPFM=false -DLLVM_BUILD_INSTRUMENTED_COVERAGE=false -DLLVM_INSTALL_UTILS=false
make -j
```

If you need to work on Compiler-RT, you can do the following (from the root folder):

```
mkdir -p build-compiler-rt
cd build-compiler-rt
cmake ../compiler-rt/ -DCMAKE_TOOLCHAIN_FILE=../compiler-rt/cmake/patmos-clang-toolchain.cmake -DCMAKE_PROGRAM_PATH="$(pwd)/../build/bin" -DCOMPILER_RT_INCLUDE_TESTS=ON
make -j
```

YOU CANNOT SKIP BUILDING COMPILER-RT BECAUSE OTHERWISE THE TWO .O files WILL BE MISSING!!!

If you need to build the standard library (newlib):

```zsh
git clone https://github.com/t-crest/patmos-newlib
mkdir build-newlib
cd build-newlib
../patmos-newlib/configure --target=patmos-unknown-unknown-elf AR_FOR_TARGET="$(pwd)/../build/bin/llvm-ar" CC_FOR_TARGET="$(pwd)/../build/bin/clang" CFLAGS_FOR_TARGET="-target patmos-unknown-unknown-elf -O2 -emit-llvm -Wno-error -Wno-error=deprecated-non-prototype -Wno-error=invalid-noreturn -D__GLIBC_USE\(...\)=0 -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types" RANLIB_FOR_TARGET="$(pwd)/../build/bin/llvm-ranlib" LD_FOR_TARGET="$(pwd)/../build/bin/clang"
make
```

This compiles std into bitcode files, such that they are ready to publish with the compiler.

### Test

To run the tests you must first build following the previous section.
Then it is simple enough. From the build folder we have 2 commands.

To test LLVM:

```
make -j llc UnitTests && ./bin/llvm-lit ../llvm/test -v --filter=Patmos
```

To test Clang:

```
make -j ClangPatmosTestDeps && ./bin/llvm-lit ../clang/test -v --filter=Patmos
```

To test LLD (from the `build` folder):

```
make -j lld && ./bin/llvm-lit ../lld/test -v --filter=Patmos
```

To test Compiler-RT, go to the `build-compiler-rt` folder:

```
./bin/llvm-lit -v test/builtins/Unit/patmos
```

### Packaging

To create a tarball containing the built compiler and standard library, use the following command from the `build` folder:

```
make PatmosPackage
```

To do this, you must have built LLVM, Compiler-RT, and `newlib` using the previous steps.
The tarball is then available under `build/patmos-unknown-unknown-elf/package-temp/patmos-llvm-*.tar.gz`, where `*` depends on the platform you are building on.
Using this tarball, you can install the compiler as described in the installation section.

_Note: The steps for building LLVM are meant for development and as such produce a debug-mode compiler (which is extremely slow).
To package a release compiler, use the flag `-DCMAKE_BUILD_TYPE=Release`_

### Releasing New Versions

To fully automatically publish a new version, simply tag the required commit using `x.y.z` version numbering (with an optional `-` followed by anything).
Github Actions will then automatically test the commit, make packages for each platform, and publish them all.

### <a name="anch-updating-llvm"></a>Updating LLVM

Even though this repository is a fork of [the official LLVM repository](https://github.com/llvm/llvm-project),
we only pull the upstream changes at official release tags.
The `upstream` branch is specifically intended for this and nothing else.
Its `HEAD` should always be a commit for an official release.
E.g. it could be tracking the commit for the 11.1.0 release.

Updating to a new release of upstream LLVM is not a trivial task.
The main difficulty comes from the fact that LLVM uses release branches, whic means their history splits after a major release.
This means trying to simply pull in the latest release tag will likely cause many merge conflicts in LLVM code that we (the Patmos project) haven't touched.
Trying to fix the merge conflict can be tedious and error prone.
Instead, the following guide will mimick mergin by simply applying Patmos' code directly to the new release.
This will cause much fewer and simply conflicts and is much less likely to be done incorrectly:

_`llvm-upstream` refers to the LLVM github repo: github.com/llvm/llvm-project._

_`t-crest` refers to the Patmos LLVM github repo: github.com/t-crest/patmos-llvm-project._

_As an example, `llvmorg-11.1.0` is the current LLVM version tag used by Patmos, and `llvmorg-12.0.1` is the one we are trying to update to. Change them as appropriate_

- Get the most recent LLVM release's tag:

```sh
git checkout master
git pull llvm-upstream --tags
```

- Update the `upstream` branch to point to it:

```sh
git branch -f upstream llvmorg-12.0.1
```

- Push the new tag and "upstream" to the t-crest repo:

```sh
git checkout upstream
git push t-crest +upstream
git push llvmorg-12.0.1
```

- Create a diff of the difference between Patmos HEAD and `llvmorg-11.1.0`, which will then contain all Patmos-specific code/changes:

```sh
git diff llvmorg-11.1.0 master > patmos-diff.patch
```

- Move to a temporary branch where you apply the changes to the latest LLVM release:

```sh
git checkout llvmorg-12.0.1 -b tempbranch
git apply --reject --verbose --ignore-whitespace patmos-diff.patch
```

- Git has now applied all changes it can. Where unable, it left a `*.rej` file containing the unapplied patmos changes (where `*` is th name of the file containing the unapplied code).
  Go through the project and apply rejected code manually and deleting the `*.rej` files as you go.
  To check whether you applied the diff correctly, you can print out the changes (`git diff > post-apply-patmos-diff.patch`) and compare it with the original diff (`diff post-apply-patmos-diff.patch patmos-diff.patch`).
  The differences should be minor and only in non-patmos code.

- Commit: `git commit -m "Merged Patmos HEAD with llvmorg-12.0.1"`

- Rewrite git history, to show that the last Patmos commit is also a parent of the new one (to make it look like we merged it with `llvmorg-12.0.1`):

```sh
git replace --graft tempbranch llvmorg-12.0.1 master
git filter-branch llvmorg-12.0.1..tempbranch
```

- Change `master` to point to the new commit, delete the temporary branch, and push to T-CREST reposity:

```sh
git branch -f master tempbranch
git checkout master
git branch -D tempbranch
git push origin +master
```

You have now effectively merged the old Patmos head with the new LLVM release tag.
The git history will showing as being a merge of the two commits.
After this, the Patmos code will probably need tweaking to conform to any changes LLVM has made since the last release.

---

Here are Christine's notes, these notes will be deleted/merged in proper release branch

### INVESTIGATIONS:

/llvm/test/Bindings/Go/lit.local.cfg was yeeeted at 2022 https://discourse.llvm.org/t/rfc-remove-the-go-bindings/65725
Fix: https://github.com/tinygo-org/go-llvm

handleLoopboundAttr at Sema/SemaAttr.cpp - Whose is it?

Currenrtl using the RISCV target as example to look into how exactly the system will work and what needs to be migrated.
Check on the LLVM

Todo, get a fpga board to try and test the code on to determine if shit actually works.

Create the following dummy target files to learn how targetting works in llvm.
Read the docs about the tablegen code generator.

Cannot use Ferrocene, because is both not open source and they don't actually edit the LLVM, they just use it as a base.
And update accordingly in upstream LLVM.

[001A] - Means, that you need to ask Emad about this. Use this number in Control-F to find all instances of this.

TableGen stuff first, if you have missing instruction header thingies

```zsh

# Ensure tblgen is built
make -C build llvm-tblgen

# Ensure that Patmos table instruction thingies are built
build/bin/llvm-tblgen -gen-instr-info -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenInstrInfo.inc
make -C build llvm-tblgen && build/bin/llvm-tblgen -gen-register-info -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenRegisterInfo.inc
cp build/lib/Target/Patmos/PatmosGenRegisterInfo* llvm/lib/Target/Patmos/
build/bin/llvm-tblgen -gen-asm-matcher -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenAsmMatcher.inc
cp build/lib/Target/Patmos/PatmosGenAsmMatcher.inc llvm/lib/Target/Patmos/


build/bin/llvm-tblgen -gen-callingconv -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenCallingConv.inc
cp build/lib/Target/Patmos/PatmosGenCallingConv.inc llvm/lib/Target/Patmos/
  
```

What the hecc to do next...

```zsh

# 0) Find first real blocker
./build/bin/llvm-lit llvm/test/CodeGen/Patmos/ -j1 -sv --max-failures=1

# 1) Reproduce just that test
./build/bin/llvm-lit llvm/test/CodeGen/Patmos/<failing-test>.ll -sv

# 2) Rebuild only needed tool fast
cmake --build build --target llc -j16

# 3) Re-run first-failure scan
./build/bin/llvm-lit llvm/test/CodeGen/Patmos/ -j1 -sv --max-failures=1

# 4) How to manually debug command (check .ll file for the exact flags)
./build/bin/llc -march=patmos < ./llvm/test/CodeGen/Patmos/flags/mpatmos-subfunction-align/align_multiple_subfunctions.ll

```

How to compile the stuff later:

```zsh

# 0) This is how to compile the code
./build/bin/clang++ --target=patmos-unknown-unknown-elf -O2 -c ./fibonacci-test.cpp -o ./fibonacci-test.o

# 1) This is how to object dump the code
./build/bin/llvm-objdump -d ./fibonacci-test.o

# 2) This is how you generate the .pml file for Platin
### NOTE: THIS IS IMPORTANT TO USE -mserialize-pml, AND NOT -mserialize
./build/bin/clang++ --target=patmos-unknown-unknown-elf -O2 -mserialize-pml=fibonacci.pml -c ./fibonacci-test.cpp -o fibonacci.out

# 3) This is how use you Platin (assuming you Git clone'd it and compiled it in Ruby)
./.platin-inst/bin/platin wcet -i fibonacci.pml -b fibonacci.out -e main --report --disable-ait --objdump-command ./build/bin/llvm-objdump

```

## Platin annoyances aka. setup

Annoying about lp_solve?

```zsh
brew tap brewsci/science
brew install lp_solve
```

How to even install the bloody thing?

```zsh
./patmos-platin/install.sh -i ./platin-inst
```

## Extra flags

`-mpatmos-skip-postra-scheduler` is possible to use to skip Patmos post-RA scheduling to avoid reg-unit assertions. Default is `false`
but for whatever reason you might need it, there's an option for that. This was initially used to analyze crash issues during compiling to
eliminate red-herrings.

## Note, apparently pasim needs to be installed to run the LLVM tests...

(WHY WASNT THIS DOCUMENTED, TODO, ADD A ERROR MESSAGE WHERE IT ASKS PEOPLE THAT THEY MUST INSTALL IT)

LLVM Requiring stricter physical register live-li-ness and stronger typed calling
is going to be the eeeeeend of meeeeeeee.

/Users/christineek/CLionProjects/patmos-llvm-project/llvm/test/CodeGen/Patmos/programs/bounded/single_source/instruction_combinations/trunc_to_br_condition.ll

Sanity reminder

```zsh
# Build it
nix build .#patmos-newlib -L
# Copy it
nix copy --to file:///Volumes/T7/nix-cache .#patmos-newlib
# Collect it
nix-collect-garbage -d
# Restore it
nix copy --from file:///Volumes/T7/nix-cache .#patmos-newlib
# Break it *enter Daft Punk riff here*
```

Bless the Omnissiah for the following method to eliminate derivation being over sensitive

```nix
let
  repoSrc = ../..;

  # Keep source hashing stable by excluding local build outputs.
  filteredRepoSrc = pkgs.lib.cleanSourceWith {
    src = repoSrc;
    filter = path: _type:
      let
        relPath = pkgs.lib.removePrefix "${toString repoSrc}/" (toString path);
        ignored =
          relPath == ".git"
          || pkgs.lib.hasPrefix ".git/" relPath
          || relPath == "build"
          || pkgs.lib.hasPrefix "build/" relPath
          || relPath == "build-compiler-rt"
          || pkgs.lib.hasPrefix "build-compiler-rt/" relPath
          || relPath == "build-newlib"
          || pkgs.lib.hasPrefix "build-newlib/" relPath
          || relPath == "result"
          || pkgs.lib.hasPrefix "result/" relPath
          || pkgs.lib.hasSuffix ".log" relPath;
      in
        !ignored;
  };
```

Uploading it to cachix manually

```zsh
nix build .#patmos-prefixed -L --show-trace --no-link --print-out-paths | cachix push kodalem
# Do not forget to copy the signing key, because they do not support multiple keys in multiple caches.
```

Maintenance - be sure to update the secrets in the github repository if a new cache signing key from cachix, and check the CI file as well in the workflow
