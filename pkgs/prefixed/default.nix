{
  pkgs,
  system,
  patmos-toolchain-src,
  patmos-toolchain-release,
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

  # Keep default target centralized for all prefixed compiler frontends.
  patmosTriple = "patmos-unknown-unknown-elf";

  mkPrefixed = name: toolchainRoot:
    pkgs.stdenv.mkDerivation {
      inherit name;
      dontUnpack = true;
      nativeBuildInputs = [pkgs.makeWrapper];

      installPhase = ''
              mkdir -p "$out/bin"

              makeWrapper "${toolchainRoot}/bin/clang" "$out/bin/patmos-clang" \
                --set-default PATMOS_TRIPLE "${patmosTriple}" \
                --set-default PATMOS_SYSROOT "${toolchainRoot}/newlib-sysroot" \
                --set-default PASIM "${patmos-simulator}/bin/pasim" \
                --add-flags "--target=${patmosTriple}"

              makeWrapper "${toolchainRoot}/bin/clang++" "$out/bin/patmos-clang++" \
                --set-default PATMOS_TRIPLE "${patmosTriple}" \
                --set-default PATMOS_SYSROOT "${toolchainRoot}/newlib-sysroot" \
                --set-default PASIM "${patmos-simulator}/bin/pasim" \
                --add-flags "--target=${patmosTriple}"

              makeWrapper "${toolchainRoot}/bin/llc" "$out/bin/patmos-llc"
              makeWrapper "${toolchainRoot}/bin/opt" "$out/bin/patmos-opt"
              makeWrapper "${toolchainRoot}/bin/llvm-ar" "$out/bin/patmos-llvm-ar"
              makeWrapper "${toolchainRoot}/bin/llvm-ranlib" "$out/bin/patmos-llvm-ranlib"
              makeWrapper "${toolchainRoot}/bin/llvm-link" "$out/bin/patmos-llvm-link"
              makeWrapper "${toolchainRoot}/bin/llvm-config" "$out/bin/patmos-llvm-config"
              makeWrapper "${toolchainRoot}/bin/llvm-objdump" "$out/bin/patmos-llvm-objdump"
              makeWrapper "${toolchainRoot}/bin/llvm-lit" "$out/bin/patmos-llvm-lit"
              makeWrapper "${toolchainRoot}/bin/lld" "$out/bin/patmos-lld"
              makeWrapper "${patmos-simulator}/bin/pasim" "$out/bin/patmos-pasim"

              cat > "$out/bin/patmos-env" <<'EOF'
        #!/usr/bin/env sh
        # Print portable environment exports for shells that support eval.
        echo "export PATMOS_TRIPLE=${patmosTriple}"
        echo "export PATMOS_SYSROOT=${toolchainRoot}/newlib-sysroot"
        echo "export PASIM=${patmos-simulator}/bin/pasim"
        EOF
              chmod +x "$out/bin/patmos-env"
      '';
    };

  prefixedSrc = mkPrefixed "patmos-prefixed-src-${system}" patmos-toolchain-src;
  prefixedRelease = mkPrefixed "patmos-prefixed-release-${system}" patmos-toolchain-release;
in {
  packages = {
    patmos-prefixed-src = prefixedSrc;
    patmos-prefixed-release = prefixedRelease;
    patmos-prefixed = prefixedRelease;
  };
}
