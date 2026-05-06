{
  pkgs,
  system,
  patmos-llvm,
  patmos-newlib,
  patmos-compiler-rt,
  patmos-simulator,
}: {
  packages.patmos-toolchain = pkgs.stdenv.mkDerivation {
    name = "patmos-toolchain-${system}";
    dontUnpack = true;
    buildInputs = [patmos-llvm patmos-newlib patmos-compiler-rt patmos-simulator];

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
}
