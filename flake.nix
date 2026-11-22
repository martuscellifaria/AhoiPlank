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
            wayland-scanner
            qt6.wrapQtAppsHook
            makeShellWrapper
          ];

          buildInputs = with pkgs; [
            qt6.qtbase
            qt6.qtdeclarative
            qt6.qtwayland
            qt6.qtsvg
            kdePackages.layer-shell-qt
            wayland
            wayland-protocols
            libxkbcommon
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
          ];

          postInstall = ''
            install -Dm644 $src/App/ahoiplank.desktop \
              $out/share/applications/ahoiplank.desktop
            install -Dm644 $src/App/resources/plank.png \
              $out/share/icons/hicolor/256x256/apps/ahoiplank.png
          '';

          postFixup = ''
            wrapProgram $out/bin/ahoiplank \
              --run 'if [ ! -e /etc/NIXOS ] && [ -z "$LIBGL_DRIVERS_PATH" ] && [ -z "$QT_QUICK_BACKEND" ]; then export QT_QUICK_BACKEND=software; fi'
          '';

          meta = with pkgs.lib; {
            description = "A customizable multi-mode launcher for Wayland compositors centered on developer experience and looking good.";
            homepage = "https://github.com";
            license = licenses.gpl3Plus;
            mainProgram = "ahoiplank";
            platforms = platforms.linux;
          };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
        };
      });
}

