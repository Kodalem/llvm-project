{
  description = "Patmos LLVM build tester flake thingy, it is 2337 and I am going insaneeeeeee";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        buildInputs = with pkgs; [
          cmake
          ninja
          git
          gcc
          binutils
          python3
          pkg-config
          libxml2
        ] ++ pkgs.lib.optional (system == "x86_64-darwin") pkgs.darwin.cctools;
      in {
        packages = let
          patmos-llvm = pkgs.stdenv.mkDerivation {
            name = "patmos-llvm";
            src = ./.;
            buildInputs = buildInputs;
            nativeBuildInputs = buildInputs;
            configurePhase = ''
              # Configure out-of-source to a deterministic 'build' directory
              cmake -S llvm -B build \
                -DCMAKE_BUILD_TYPE=Debug \
                -DLLVM_TARGETS_TO_BUILD=Patmos \
                -DLLVM_DEFAULT_TARGET_TRIPLE=patmos-unknown-unknown-elf \
                -DLLVM_ENABLE_PROJECTS="clang;lld" \
                -DCLANG_ENABLE_OBJC_REWRITER=false \
                -DCLANG_ENABLE_STATIC_ANALYZER=false \
                -DCLANG_BUILD_EXAMPLES=false \
                -DLLVM_ENABLE_BINDINGS=false \
                -DLLVM_INSTALL_BINUTILS_SYMLINKS=false \
                -DLLVM_INSTALL_CCTOOLS_SYMLINKS=false \
                -DLLVM_INCLUDE_EXAMPLES=false \
                -DLLVM_INCLUDE_BENCHMARKS=false \
                -DLLVM_APPEND_VC_REV=false \
                -DLLVM_ENABLE_WARNINGS=false \
                -DLLVM_ENABLE_PEDANTIC=false \
                -DLLVM_ENABLE_LIBPFM=false \
                -DLLVM_BUILD_INSTRUMENTED_COVERAGE=false \
                -DLLVM_INSTALL_UTILS=false
            '';
            buildPhase = ''
              # Use cmake --build for portability between generators (ninja/make)
              cmake --build build -- -j$NIX_BUILD_CORES
            '';
            installPhase = ''
              mkdir -p $out
              cp -r build/* $out/
            '';
          };
        in {
          patmos-llvm = patmos-llvm;
          default = patmos-llvm;
        };

        devShells = {
          default = pkgs.mkShell {
            buildInputs = buildInputs;
            shellHook = ''
              echo "Patmos LLVM dev environment"
            '';
          };
        };
      }
    );
}
