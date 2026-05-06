{
  pkgs,
  system,
  patmos-llvm,
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
        || relPath == ".github"
        || pkgs.lib.hasPrefix ".github/" relPath
        || relPath == ".ci"
        || pkgs.lib.hasPrefix ".ci/" relPath
        || relPath == ".forgejo"
        || pkgs.lib.hasPrefix ".forgejo/" relPath
        || relPath == "build"
        || pkgs.lib.hasPrefix "build/" relPath
        || relPath == "build-compiler-rt"
        || pkgs.lib.hasPrefix "build-compiler-rt/" relPath
        || relPath == "build-newlib"
        || pkgs.lib.hasPrefix "build-newlib/" relPath
        || relPath == "result"
        || pkgs.lib.hasPrefix "result/" relPath
        || (builtins.match "[^/]+\\.md" relPath) != null
        || pkgs.lib.hasSuffix ".log" relPath;
    in
      !ignored;
  };
  commonBuildInputs = with pkgs;
    [
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
      texinfo
    ]
    ++ pkgs.lib.optional pkgs.stdenv.isDarwin pkgs.darwin.cctools;
in {
  packages.patmos-newlib = pkgs.stdenv.mkDerivation rec {
    name = "patmos-newlib-${system}";
    newlibBuildDir = "build-newlib";
    src = pkgs.fetchFromGitHub {
      owner = "t-crest";
      repo = "patmos-newlib";
      rev = "57965996de83d7a8d9b938b8b2b950ea48efa8cb";
      sha256 = "sha256-08CIszoRQOY65ivN/SJcBtZBlYxNPXXlX7EN17OcmN8=";
    };
    buildInputs = [patmos-llvm patmos-simulator];
    nativeBuildInputs = commonBuildInputs ++ [patmos-simulator];

    # patmos-newlib is Autotools-based; prevent the cmake hook from taking over.
    dontUseCmakeConfigure = true;
    # Keep the vendored config.sub: it accepts patmos-unknown-unknown-elf,
    # while the newer generic replacement from nixpkgs rejects that target.
    dontUpdateAutotoolsGnuConfigScripts = true;

    configurePhase = ''
      runHook preConfigure

      # Some Patmos LLVM outputs may have non-executable tool files in the store.
      # Mirror required tools locally and force execute bits as a fallback.
      localToolchain="$NIX_BUILD_TOP/patmos-llvm-local"
      mkdir -p "$localToolchain/bin" "$localToolchain/lib"
      ln -s "${patmos-llvm}/lib/clang" "$localToolchain/lib/clang"

      resolveTool() {
        toolName="$1"
        storeTool="${patmos-llvm}/bin/$toolName"
        localTool="$localToolchain/bin/$toolName"

        if [ -x "$storeTool" ]; then
          printf '%s\n' "$storeTool"
          return 0
        fi

        cp "$storeTool" "$localTool"
        chmod u+x "$localTool"
        printf '%s\n' "$localTool"
      }

      targetAr="$(resolveTool llvm-ar)"
      targetRanlib="$(resolveTool llvm-ranlib)"
      targetClang="$(resolveTool clang)"

      echo "Using AR_FOR_TARGET=$targetAr"
      echo "Using CC_FOR_TARGET=$targetClang"
      echo "Using RANLIB_FOR_TARGET=$targetRanlib"

      mkdir -p "$NIX_BUILD_TOP/$sourceRoot/$newlibBuildDir"
      cd "$NIX_BUILD_TOP/$sourceRoot/$newlibBuildDir"
      ../configure \
        --target=patmos-unknown-unknown-elf \
        --prefix=/usr \
        AR_FOR_TARGET="$targetAr" \
        CC_FOR_TARGET="$targetClang" \
        CFLAGS_FOR_TARGET="-target patmos-unknown-unknown-elf -O2 -emit-llvm -Wno-error -Wno-error=deprecated-non-prototype -Wno-error=invalid-noreturn -D__GLIBC_USE\(...\)=0 -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types" \
        RANLIB_FOR_TARGET="$targetRanlib" \
        LD_FOR_TARGET="$targetClang"
      cd "$NIX_BUILD_TOP/$sourceRoot"
      runHook postConfigure
    '';

    buildPhase = ''
      runHook preBuild
      make -C "$NIX_BUILD_TOP/$sourceRoot/$newlibBuildDir" -j''${NIX_BUILD_CORES:-1}
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      make -C "$NIX_BUILD_TOP/$sourceRoot/$newlibBuildDir" install DESTDIR="$out"
      runHook postInstall
    '';
  };
}
