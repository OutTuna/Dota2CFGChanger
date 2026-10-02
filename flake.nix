{
  description = "Dota 2 Config Manager for Linux";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/c59305bab2065cfecc4944690d9eedbb56f3a9fa";

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      package = pkgs.callPackage ./packaging/nix/package.nix { };
    in
    {
      packages.${system} = {
        default = package;
        dota2cfgchanger = package;
      };
      checks.${system} = {
        package = package;
        nixos = package.tests.nixos;
      };
      formatter.${system} = pkgs.nixfmt;
    };
}
