{ pkgs ? import <nixpkgs> {} }:

with pkgs;
let gpu-burn-pkg = (
      import <nixos> {
        config.cudaSupport = true;
        config.allowUnfree = true;
      }).gpu-burn;
in
mkShell {
  packages = [ gnumake libc.static gpu-burn-pkg ];

  shellHook = ''
    export NIX_SHELL_NAME="NVPG"
  '';
}
