{
  description = "NVIDIA GPU power governor based on temperature";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forEachSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f nixpkgs.legacyPackages.${system});

      mkPackage = pkgs: pkgs.stdenv.mkDerivation {
        name = "nvidia-power-governor";
        src = ./.;
        buildInputs = [ pkgs.gnumake pkgs.gcc pkgs.libc.static ];
        buildPhase = "make";
        installPhase = "install -Dm755 nvidia-power-governor $out/bin/nvidia-power-governor";
      };
    in {
      packages = forEachSystem (pkgs: {
        default = mkPackage pkgs;
      });

      devShells = forEachSystem (pkgs: {
        default = pkgs.mkShell {
          packages = [ pkgs.gnumake pkgs.gcc pkgs.libc.static ];
        };
      });

      apps = forEachSystem (pkgs: {
        default = {
          type = "app";
          program = "${mkPackage pkgs}/bin/nvidia-power-governor";
        };
      });
    };
}
