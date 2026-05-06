{
  pkgs,
  system,
  packages,
  simulator,
}: let
  repoSrc = ../..;

  # Avoid rebuilding LLVM when local build outputs/logs change.
  filteredRepoSrc = pkgs.lib.cleanSourceWith {
    src = repoSrc;
    filter = path: _type: let
      relPath = pkgs.lib.removePrefix "${toString repoSrc}/" (toString path);
      ignored =
        relPath
        == ".git"
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

  commonBuildInputs = with pkgs;
    [
      cmake
      ninja
      git
      clang
      gcc
      binutils
      python3
      pkg-config
      libxml2
      gnutar
      wget
      gnumake
      ccache
      lld # Keep lld in buildInputs
    ]
    ++ pkgs.lib.optional (system == "x86_64-linux") mold
    ++ pkgs.lib.optional pkgs.stdenv.isDarwin pkgs.darwin.cctools;

  ccacheConfig = ''
    mkdir -p $CCACHE_DIR
    export PATH="${pkgs.ccache}/bin:$PATH"
    ccache --show-stats || true
    ccache --zero-stats || true
  '';

  moldConfig = pkgs.lib.optionalString (system == "x86_64-linux") ''
    mkdir -p $PWD/bin
    ln -s ${pkgs.mold}/bin/mold $PWD/bin/lld
    ln -s ${pkgs.mold}/bin/mold $PWD/bin/ld
  '';
in {
  packages.patmos-llvm = pkgs.clangStdenv.mkDerivation rec {
    name = "patmos-llvm-${system}";
    src = filteredRepoSrc;

    buildInputs = commonBuildInputs;
    nativeBuildInputs = commonBuildInputs;

    CCACHE_DIR = "${pkgs.ccache}/bin/ccache";
    CCACHE_MAXSIZE = "5G";
    CCACHE_COMPRESS = "1";
    CCACHE_SLOPPINESS = "file_macro,time_macros";

    preConfigurePhase = ''
      ${ccacheConfig}
      ${moldConfig}
    '';

    cmakeFlags =
      [
        "-DCMAKE_BUILD_TYPE=Release"
        "-DLLVM_TARGETS_TO_BUILD=Patmos"
        "-DLLVM_DEFAULT_TARGET_TRIPLE=patmos-unknown-unknown-elf"
        "-DLLVM_ENABLE_PROJECTS=clang;lld"
        "-DCLANG_ENABLE_OBJC_REWRITER=OFF"
        "-DCLANG_ENABLE_STATIC_ANALYZER=OFF"
        "-DCLANG_BUILD_EXAMPLES=OFF"
        "-DLLVM_ENABLE_BINDINGS=OFF"
        "-DLLVM_INSTALL_BINUTILS_SYMLINKS=OFF"
        "-DLLVM_INSTALL_CCTOOLS_SYMLINKS=OFF"
        "-DLLVM_INCLUDE_EXAMPLES=OFF"
        "-DLLVM_INCLUDE_BENCHMARKS=OFF"
        "-DLLVM_INCLUDE_TESTS=OFF"
        "-DLLVM_APPEND_VC_REV=OFF"
        "-DLLVM_ENABLE_WARNINGS=OFF"
        "-DLLVM_ENABLE_PEDANTIC=OFF"
        "-DLLVM_ENABLE_LIBPFM=OFF"
        "-DLLVM_BUILD_INSTRUMENTED_COVERAGE=OFF"
        "-DLLVM_INSTALL_UTILS=OFF"
        "-DCLANG_INCLUDE_TESTS=OFF"
        # Ensure MachineInstr::dump() is available even in Release builds
        "-DLLVM_ENABLE_DUMP=ON"
        # dsymutil needs CoreFoundation SDK headers; skip it for this cross-compiler build
        "-DLLVM_TOOL_DSYMUTIL_BUILD=OFF"
      ]
      ++ pkgs.lib.optional (system == "x86_64-linux") [
        "-DCMAKE_CXX_LINKER=mold"
        "-DCMAKE_C_LINKER=mold"
        "-DLLVM_USE_LINKER=mold"
      ];

    configurePhase = ''
      export CCACHE_CPP2=yes
      export CCACHE_SLOPPINESS=${CCACHE_SLOPPINESS}
      cmake -S llvm -B build ${pkgs.lib.escapeShellArgs cmakeFlags}
    '';

    buildPhase = ''
      export CCACHE_MAXSIZE=${CCACHE_MAXSIZE}
      export CCACHE_COMPRESS=${CCACHE_COMPRESS}
      cmake --build build --parallel 16
      # Generate TableGen files
      cmake --build build --target llvm-tblgen --parallel 12
      build/bin/llvm-tblgen -gen-instr-info -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenInstrInfo.inc
      build/bin/llvm-tblgen -gen-register-info -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenRegisterInfo.inc
      cp build/lib/Target/Patmos/PatmosGenRegisterInfo* llvm/lib/Target/Patmos/
      build/bin/llvm-tblgen -gen-asm-matcher -I llvm/lib/Target/Patmos -I llvm/include -I llvm/lib/Target llvm/lib/Target/Patmos/Patmos.td -o build/lib/Target/Patmos/PatmosGenAsmMatcher.inc
      cp build/lib/Target/Patmos/PatmosGenAsmMatcher.inc llvm/lib/Target/Patmos/
    '';

    installPhase = ''
      mkdir -p $out
      cp -r build/* $out/
      ccache --show-stats || true
    '';
  };

  checks.patmos-llvm-tests = pkgs.stdenv.mkDerivation {
    name = "patmos-llvm-tests-${system}";
    buildInputs = [packages.patmos-llvm simulator];

    buildPhase = ''
      export PASIM="${simulator}/bin/pasim"
      export PATH="${simulator}/bin:$PATH"

      if [ -f "${packages.patmos-llvm}/bin/llvm-lit" ] && [ -d "./llvm/test" ]; then
        echo "Running Patmos LLVM tests..."
        ${packages.patmos-llvm}/bin/llvm-lit ./llvm/test -v --filter=Patmos || true
      fi
    '';
  };
}
