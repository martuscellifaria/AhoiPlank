{
  description = "AhoiPlank, developer-first launcher for Wayland.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "ahoiplank";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            qt6.wrapQtAppsHook
          ];

          buildInputs = with pkgs; [
            qt6.qtbase
            qt6.qtdeclarative
            kdePackages.layer-shell-qt
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
	    "-G" "Ninja"
          ];

          postInstall = ''
            install -Dm644 ${./App/ahoiplank.desktop} \
              $out/share/applications/ahoiplank.desktop
            install -Dm644 ${./App/resources/plank.png} \
              $out/share/icons/hicolor/256x256/apps/ahoiplank.png
          '';

          meta = with pkgs.lib; {
            description = "A customizable multi-mode launcher for Wayland compositors centered on developer experience and looking good.";
            homepage = "https://github.com/martuscellifaria/ahoiplank";
            license = licenses.gpl3;
            mainProgram = "ahoiplank";
            platforms = platforms.linux;
          };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
        };
      });
}
