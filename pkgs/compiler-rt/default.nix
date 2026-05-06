{
  pkgs,
  system,
  patmos-llvm,
  patmos-newlib,
  patmos-simulator,
}: let
  repoSrc = ../..;

  # Keep source hashing stable by excluding local build outputs.
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
    ]
    ++ pkgs.lib.optional pkgs.stdenv.isDarwin pkgs.darwin.cctools;
in {
  packages.patmos-compiler-rt = pkgs.stdenv.mkDerivation {
    name = "patmos-compiler-rt-${system}";
    src = filteredRepoSrc;
    buildInputs = [patmos-llvm patmos-newlib patmos-simulator];
    nativeBuildInputs = commonBuildInputs ++ [patmos-simulator];

    # patmos-newlib is Autotools-based; prevent the cmake hook from taking over.
    dontUseCmakeConfigure = true;
    # Keep the vendored config.sub: it accepts patmos-unknown-unknown-elf,
    # while the newer generic replacement from nixpkgs rejects that target.
    dontUpdateAutotoolsGnuConfigScripts = true;

    buildPhase = ''
      runHook preBuild
      mkdir -p build-compiler-rt
      cd build-compiler-rt
      cmake ../compiler-rt \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DCMAKE_TOOLCHAIN_FILE=../compiler-rt/cmake/patmos-clang-toolchain.cmake \
        -DCMAKE_C_COMPILER="${patmos-llvm}/bin/clang" \
        -DCMAKE_CXX_COMPILER="${patmos-llvm}/bin/clang++" \
        -DCOMPILER_RT_TEST_COMPILER="${patmos-llvm}/bin/clang" \
        -DLLVM_TOOLS_BINARY_DIR="${patmos-llvm}/bin" \
        -DLLVM_CONFIG_PATH="${patmos-llvm}/bin/llvm-config" \
        -DCOMPILER_RT_INCLUDE_TESTS=ON \
        -DCOMPILER_RT_TEST_STANDALONE_BUILD_LIBS=OFF
      make -j''${NIX_BUILD_CORES:-1}
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      cd "$NIX_BUILD_TOP/$sourceRoot/build-compiler-rt"
      make install
      runHook postInstall
    '';

    checkPhase = ''
      export PASIM="${patmos-simulator}/bin/pasim"
      export PATH="${patmos-simulator}/bin:$PATH"
      if [ -f "build-compiler-rt/bin/llvm-lit" ]; then
        build-compiler-rt/bin/llvm-lit -v compiler-rt/test/builtins/Unit/patmos || true
      fi
    '';
  };
}
