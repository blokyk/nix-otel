{
  description = "Example rust project";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem
      (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [ self.overlays.default ];
          };
        in
        {
          packages = rec {
            nix-otel = pkgs.nix-otel;
            default = nix-otel;
          };
          checks = self.packages.${system};

          # for debugging
          inherit pkgs;

          devShells.default = pkgs.nix-otel.overrideAttrs (
            old: {
              # make rust-analyzer work
              RUST_SRC_PATH = pkgs.rustPlatform.rustLibSrc;

              # any dev tools you use in excess of the rust ones
              nativeBuildInputs = old.nativeBuildInputs ++ (
                with pkgs; [
                  nix
                  bear
                  rust-analyzer
                  rust-cbindgen
                  clang-tools_14
                ]
              );
            }
          );
        }
      )
    // {
      overlays.default = (
        final: prev: {
          nix-otel = final.callPackage ./default.nix {};
        }
      );
    };
}
