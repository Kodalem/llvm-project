{
  pkgs,
  system,
  patmos-toolchain-src,
}: let
  # Fill these when channel artifacts are published.
  channelArtifacts = {
    release = {
      url = null;
      sha256 = null;
    };
  };

  mkChannelToolchain = channel: let
    artifact = channelArtifacts.${channel};
    hasArtifact = artifact.url != null && artifact.sha256 != null;
  in
    if hasArtifact
    then
      pkgs.stdenvNoCC.mkDerivation {
        name = "patmos-bin-${channel}-${system}";
        src = pkgs.fetchurl {
          inherit (artifact) url sha256;
        };
        dontUnpack = true;
        nativeBuildInputs = [pkgs.gnutar];

        installPhase = ''
          mkdir -p "$out"
          tar -xzf "$src" -C "$out"
        '';
      }
    else patmos-toolchain-src;
in {
  packages = {
    patmos-bin-release = mkChannelToolchain "release";
  };
}
