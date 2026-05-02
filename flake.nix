{
  description = "Patmos LLVM build and test flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        commonBuildInputs = with pkgs; [
          cmake
          ninja
          git
          gcc
          binutils
          python3
          pkg-config
          libxml2
          gnutar
          wget
          gnumake
        ] ++ pkgs.lib.optional (system == "x86_64-darwin") pkgs.darwin.cctools;

        patmos-simulator = pkgs.stdenv.mkDerivation {
          name = "patmos-simulator-${system}";
          src = pkgs.fetchurl {
            url = if system == "x86_64-darwin" then
              "https://github.com/t-crest/patmos-simulator/releases/latest/download/patmos-simulator-x86_64-apple-darwin.tar.gz"
            else if system == "aarch64-darwin" then
              "https://github.com/t-crest/patmos-simulator/releases/download/1.0.8/patmos-simulator-arm64-apple-darwin.tar.gz"
            else
              "https://github.com/t-crest/patmos-simulator/releases/latest/download/patmos-simulator-x86_64-linux-gnu.tar.gz";
            sha256 = {
              "x86_64-darwin" = "sha256-HBK/5tJIl6ve8iuGIqzFCTszB4FRQtUuQowYnCpofFg=";
              "aarch64-darwin" = "sha256-6WaKA6JVRP9BDA6aSFFeghOMRMEFZ29bSIdlCWN4y/A=";
              "x86_64-linux"   = "sha256-kxMr28pI7iHriGeghlRH+m8suGjPMavxJHxmkcPed2U=";
            }.${system} or (throw "Unsupported system: ${system}");
          };
          buildPhase = ''
                mkdir -p $out/bin
                tar -xzf $src -C $out
                chmod +x $out/bin/pasim
          '';
          installPhase = "cp -r $out/bin/* $out/";
        };

        patmos-llvm = pkgs.stdenv.mkDerivation rec {
          name = "patmos-llvm";
          src = ./.;

          buildInputs = commonBuildInputs ++ [ patmos-simulator ];
          nativeBuildInputs = commonBuildInputs ++ [ patmos-simulator ];

          configurePhase = ''
            # Configure out-of-source to a deterministic 'build' directory
            cmake -S llvm -B build \
              -DCMAKE_BUILD_TYPE=Debug \
              -DLLVM_TARGETS_TO_BUILD=Patmos \
              -DLLVM_DEFAULT_TARGET_TRIPLE=patmos-unknown-unknown-elf \
              -DLLVM_ENABLE_PROJECTS="clang;lld" \
              -DCLANG_ENABLE_OBJC_REWRITER=OFF \
              -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
              -DCLANG_BUILD_EXAMPLES=OFF \
              -DLLVM_ENABLE_BINDINGS=OFF \
              -DLLVM_INSTALL_BINUTILS_SYMLINKS=OFF \
              -DLLVM_INSTALL_CCTOOLS_SYMLINKS=OFF \
              -DLLVM_INCLUDE_EXAMPLES=OFF \
              -DLLVM_INCLUDE_BENCHMARKS=OFF \
              -DLLVM_APPEND_VC_REV=OFF \
              -DLLVM_ENABLE_WARNINGS=OFF \
              -DLLVM_ENABLE_PEDANTIC=OFF \
              -DLLVM_ENABLE_LIBPFM=OFF \
              -DLLVM_BUILD_INSTRUMENTED_COVERAGE=OFF \
              -DLLVM_INSTALL_UTILS=OFF
          '';

          buildPhase = ''
              cmake --build build --parallel
          '';

          # Generate TableGen files (from yer notes)
          postBuild = ''
            cmake --build build --target llvm-tblgen --parallel $${NIX_BUILD_CORES}
            build/bin/llvm-tblgen -gen-instr-info -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenInstrInfo.inc
            build/bin/llvm-tblgen -gen-register-info -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenRegisterInfo.inc
            cp build/lib/Target/Patmos/PatmosGenRegisterInfo* llvm/lib/Target/Patmos/
            build/bin/llvm-tblgen -gen-asm-matcher -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenAsmMatcher.inc
            cp build/lib/Target/Patmos/PatmosGenAsmMatcher.inc llvm/lib/Target/Patmos/
          '';

          installPhase = ''
            mkdir -p $out
            cp -r build/* $out/
          '';

          # Comprehensive test phase (LLVM, Clang, LLD)
          checkPhase = ''
            export PASIM="${patmos-simulator}/bin/pasim"
            export PATH="${patmos-simulator}/bin:$PATH"

            # Test LLVM
            if [ -f "build/bin/llvm-lit" ] && [ -d "llvm/test" ]; then
              echo "Running LLVM Patmos tests..."
              build/bin/llvm-lit llvm/test -v --filter=Patmos || true
            fi

            # Test Clang (requires ClangPatmosTestDeps)
            if [ -f "build/bin/clang" ] && [ -d "clang/test" ]; then
              echo "Building Clang Patmos test dependencies..."
              cmake --build build --target ClangPatmosTestDeps --parallel $${NIX_BUILD_CORES}
              echo "Running Clang Patmos tests..."
              build/bin/llvm-lit clang/test -v --filter=Patmos || true
            fi

            # Test LLD
            if [ -f "build/bin/lld" ] && [ -d "lld/test" ]; then
              echo "Running LLD Patmos tests..."
              cmake --build build --target lld --parallel $${NIX_BUILD_CORES}
              build/bin/llvm-lit lld/test -v --filter=Patmos || true
            fi
          '';
        };

        patmos-newlib = pkgs.stdenv.mkDerivation rec {
          name = "patmos-newlib";
          src = pkgs.fetchFromGitHub {
            owner = "t-crest";
            repo = "patmos-newlib";
            rev = "57965996de83d7a8d9b938b8b2b950ea48efa8cb"; # Replace with a stable commit or tag
            sha256 = "sha256-08CIszoRQOY65ivN/SJcBtZBlYxNPXXlX7EN17OcmN8=";
          };
          buildInputs = [ patmos-llvm patmos-simulator ];
          nativeBuildInputs = commonBuildInputs ++ [ patmos-simulator ];

          buildPhase = ''
            mkdir -p build-newlib
            cd build-newlib
            ../${src}/configure \
              --target=patmos-unknown-unknown-elf \
              --prefix=/usr \
              AR_FOR_TARGET="${patmos-llvm}/bin/llvm-ar" \
              CC_FOR_TARGET="${patmos-llvm}/bin/clang" \
              CFLAGS_FOR_TARGET="-target patmos-unknown-unknown-elf -O2 -emit-llvm -Wno-error -Wno-error=deprecated-non-prototype -Wno-error=invalid-noreturn -D__GLIBC_USE\(...\)=0 -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types" \
              RANLIB_FOR_TARGET="${patmos-llvm}/bin/llvm-ranlib" \
              LD_FOR_TARGET="${patmos-llvm}/bin/clang"
            make -j
            make install DESTDIR=$out
          '';
        };

        patmos-compiler-rt = pkgs.stdenv.mkDerivation {
          name = "patmos-compiler-rt";
          src = ./.; # Assuming compiler-rt is part of the LLVM monorepo
          buildInputs = [ patmos-llvm patmos-newlib patmos-simulator ];
          nativeBuildInputs = commonBuildInputs ++ [ patmos-simulator ];

          buildPhase = ''
            mkdir -p build-compiler-rt
            cd build-compiler-rt
            cmake ../compiler-rt \
              -G Ninja \
              -DCMAKE_TOOLCHAIN_FILE=../compiler-rt/cmake/patmos-clang-toolchain.cmake \
              -DCMAKE_C_COMPILER="${patmos-llvm}/bin/clang" \
              -DCMAKE_CXX_COMPILER="${patmos-llvm}/bin/clang++" \
              -DCOMPILER_RT_TEST_COMPILER="${patmos-llvm}/bin/clang" \
              -DLLVM_TOOLS_BINARY_DIR="${patmos-llvm}/bin" \
              -DLLVM_CONFIG_PATH="${patmos-llvm}/bin/llvm-config" \
              -DCOMPILER_RT_INCLUDE_TESTS=ON \
              -DCOMPILER_RT_TEST_STANDALONE_BUILD_LIBS=OFF
            ninja
          '';

          checkPhase = ''
            export PASIM="${patmos-simulator}/bin/pasim"
            export PATH="${patmos-simulator}/bin:$PATH"
            if [ -f "build-compiler-rt/bin/llvm-lit" ]; then
              build-compiler-rt/bin/llvm-lit -v test/builtins/Unit/patmos || true
            fi
          '';
        };

        patmos-benchmarks = pkgs.stdenv.mkDerivation {
          name = "patmos-benchmarks";
          src = pkgs.fetchFromGitHub {
            owner = "t-crest";
            repo = "patmos-benchmarks";
            rev = "d682431a77e37adf1de755c5917604aaeacaf800"; # Replace with a stable commit or tag
            sha256 = "sha256-3m9TEz/bHg0PNFNtYc0oPNpm0XeJ31lBPY6Hh6LqHTk=";
          };
          buildInputs = [ patmos-llvm patmos-newlib patmos-compiler-rt patmos-simulator ];
          nativeBuildInputs = commonBuildInputs ++ [ patmos-simulator ];

          buildPhase = ''
            mkdir -p build-bench
            cmake -S . -B build-bench \
              -DCMAKE_TOOLCHAIN_FILE=./cmake/patmos-clang-toolchain.cmake \
              -DENABLE_TESTING=ON
            cmake --build build-bench --parallel
          '';

          checkPhase = ''
            export PASIM="${patmos-simulator}/bin/pasim"
            export PATH="${patmos-simulator}/bin:$PATH"
            cd build-bench && ctest --output-on-failure
          '';
        };

        # --- Final Toolchain Package (Release-Ready) ---
        patmos-toolchain = pkgs.stdenv.mkDerivation {
          name = "patmos-toolchain";
          buildInputs = [ patmos-llvm patmos-newlib patmos-compiler-rt patmos-simulator ];

          installPhase = ''
            mkdir -p $out
            cp -r ${patmos-llvm}/* $out/
            mkdir -p $out/newlib-sysroot
            cp -r ${patmos-newlib}/* $out/newlib-sysroot/
            mkdir -p $out/compiler-rt-build
            cp -r ${patmos-compiler-rt}/* $out/compiler-rt-build/
            mkdir -p $out/patmos-tools
            cp -r ${patmos-simulator}/* $out/patmos-tools/
          '';
        };
      in {
          packages = {
            patmos-simulator = patmos-simulator;  # Explicitly map the name
            patmos-llvm = patmos-llvm;            # Explicitly map the name
            patmos-newlib = patmos-newlib;        # Explicitly map the name
            patmos-compiler-rt = patmos-compiler-rt;
            patmos-benchmarks = patmos-benchmarks;
            patmos-toolchain = patmos-toolchain;
            default = patmos-llvm;
          };

        devShells = {
          default = pkgs.mkShell {
            buildInputs = commonBuildInputs ++ [ patmos-simulator ];
            shellHook = ''
              echo "Patmos LLVM dev environment"
              export PASIM="${patmos-simulator}/bin/pasim"
              export PATH="${patmos-simulator}/bin:$PATH"
            '';
          };
        };
      }
    );
}