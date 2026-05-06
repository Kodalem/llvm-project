{
  description = "Patmos LLVM Project - Modular Flakes";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  nixConfig = {
    extra-substituters = "https://patmos-llvm.cachix.org";
    extra-trusted-public-keys = "patmos-llvm.cachix.org-1:7xiUm7SvEZ66fS1aS+ZtVRyWzEB2XRfaSR7KtmuUlPA=";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = import nixpkgs {inherit system;};
        packages = import ./pkgs {inherit pkgs system;};
      in {
        packages = packages.packages;
        checks = packages.checks;
        devShells = packages.devShells;
        apps = {
          patmos-clang = {
            type = "app";
            program = "${packages.packages.patmos-prefixed}/bin/patmos-clang";
          };
          patmos-clang-release = {
            type = "app";
            program = "${packages.packages.patmos-prefixed-release}/bin/patmos-clang";
          };
          patmos-clang-src = {
            type = "app";
            program = "${packages.packages.patmos-prefixed-src}/bin/patmos-clang";
          };
          patmos-llc = {
            type = "app";
            program = "${packages.packages.patmos-prefixed}/bin/patmos-llc";
          };
          default = {
            type = "app";
            program = "${packages.packages.patmos-prefixed}/bin/patmos-clang";
          };
        };
      }
    );
}
