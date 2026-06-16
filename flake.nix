{
  description = "NVIDIA GPU power governor based on temperature";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forEachSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f nixpkgs.legacyPackages.${system});

      mkPackage = pkgs: static: pkgs.stdenv.mkDerivation {
        name = "nvidia-power-governor" + (if static then "-static" else "");
        src = ./.;
        buildInputs = [ pkgs.gnumake pkgs.gcc ] ++ (if static then [ pkgs.libc.static ] else []);
        buildPhase = (if static then "make static" else "make");
        installPhase = "install -Dm755 nvidia-power-governor $out/bin/nvidia-power-governor";
      };
    in {
      packages = forEachSystem (pkgs: {
        default = mkPackage pkgs false;
        dynamic = mkPackage pkgs false;
        static  = mkPackage pkgs true;
      });

      devShells = forEachSystem (pkgs: {
        default = pkgs.mkShell {
          packages = [ pkgs.gnumake pkgs.gcc ];
        };
      });

      apps = forEachSystem (pkgs: {
        default = {
          type = "app";
          program = "${mkPackage pkgs false}/bin/nvidia-power-governor";
        };
      });
    };
}
