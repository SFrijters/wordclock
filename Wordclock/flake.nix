{
  description = "Flake template for Arduino projects";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    arduino-nix.url = "github:bouk/arduino-nix";
  };

  outputs =
    {
      self,
      nixpkgs,
      arduino-nix,
      ...
    }:
    let
      inherit (nixpkgs) lib;
      # Boilerplate to make the rest of the flake more readable
      # Do not inject system into these attributes
      flatAttrs = [
        "overlays"
        "nixosModules"
      ];
      # Inject a system attribute if the attribute is not one of the above
      injectSystem =
        system:
        lib.mapAttrs (name: value: if builtins.elem name flatAttrs then value else { ${system} = value; });
      # Combine the above for a list of 'systems'
      forSystems =
        systems: f:
        lib.attrsets.foldlAttrs (
          acc: system: value:
          lib.attrsets.recursiveUpdate acc (injectSystem system value)
        ) { } (lib.genAttrs systems f);
    in
    forSystems
      [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ]
      (
        system:
        let
          name = "Wordclock";

          mkspiffs-overlay = final: prev: {
            mkspiffs = prev.mkspiffs.overrideAttrs (
              finalAttrs: prevAttrs: {
                buildInputs =
                  prevAttrs.buildInputs or [ ]
                  ++ lib.optionals (final.stdenv.hostPlatform.isDarwin) [
                    final.apple-sdk_11
                    (final.darwinMinVersionHook "11.0")
                  ];
                postPatch =
                  prevAttrs.postPatch or ""
                  + lib.optionalString (final.stdenv.hostPlatform.isDarwin && final.stdenv.hostPlatform.isAarch64) ''
                    substituteInPlace Makefile \
                      --replace-fail "-arch i386 -arch x86_64" "-arch aarch64"
                  '';
                meta.platforms = lib.platforms.all;
              }
            );
          };

          overlays = [
            arduino-nix.overlay
            # https://downloads.arduino.cc/packages/package_index.json
            (arduino-nix.mkArduinoPackageOverlay ./package-index/package_index.json)
            # https://arduino.esp8266.com/stable/package_esp8266com_index.json
            (arduino-nix.mkArduinoPackageOverlay ./package-index/package_esp8266com_index.json)
            # https://downloads.arduino.cc/libraries/library_index.json
            (arduino-nix.mkArduinoLibraryOverlay ./package-index/library_index.json)
            mkspiffs-overlay
          ];

          pkgs = import nixpkgs { inherit system overlays; };

          python = pkgs.python3;

          pythonWithExtras = python.buildEnv.override {
            extraLibs = [ ];
          };

          gnumake-wrapper = pkgs.writeShellApplication {
            name = "make";
            text = ''
              ${lib.getExe pkgs.gnumake} _ARDUINO_PROJECT_DIR="''${_ARDUINO_PROJECT_DIR:-/tmp/arduino}" --file=${./Makefile} "$@"
            '';
          };

          arduino-cli-with-packages = pkgs.wrapArduinoCLI {
            libraries = with pkgs.arduinoLibraries; [
              FastLED."3.6.0"
              Time."1.6.1"
              ArduinoJson."5.13.2"
            ];

            packages = [
              pkgs.arduinoPackages.platforms.esp8266.esp8266."3.1.2"
            ];
          };

          # The variables starting with underscores are custom,
          # the ones starting with ARDUINO are used by arduino-cli.
          # See https://arduino.github.io/arduino-cli/0.33/configuration/ .

          # Store everything that arduino-cli downloads in a directory
          # reserved for this project, and following the XDG specification,
          # if the variable is available.

          # The _ARDUINO_PYTHON3 variable is passed to arduino-cli via the Makefile.
          arduinoShellHookPaths = ''
            if [ -z "''${_ARDUINO_PROJECT_DIR:-}" ]; then
              if [ -n "''${_ARDUINO_ROOT_DIR:-}" ]; then
                export _ARDUINO_PROJECT_DIR="''${_ARDUINO_ROOT_DIR}/${name}"
              elif [ -n "''${XDG_CACHE_HOME:-}" ]; then
                export _ARDUINO_PROJECT_DIR="''${XDG_CACHE_HOME}/arduino/${name}"
              else
                export _ARDUINO_PROJECT_DIR="''${HOME}/.arduino/${name}"
              fi
            fi
          '';

          devShellArduinoCLI = pkgs.mkShell {
            name = "${name}-dev";
            packages = with pkgs; [
              arduino-cli-with-packages # For compiling and uploading the sketch
              git # For embedding a version hash into the sketch
              gnumake-wrapper # To provide somewhat standardized commands to compile, upload, and monitor the sketch
              picocom # To monitor the serial output
              pythonWithExtras # So that the python3 wrapper of the esp8266 downloaded code can find a working python interpreter on the path
              esptool
              mkspiffs-presets.arduino-esp8266
            ];
            shellHook = ''
              ${arduinoShellHookPaths}
              if [ ! ''${DEVSHELL_VERBOSE:-1} = 0 ]; then
                >&2 echo "==> Using arduino-cli version $(arduino-cli version)"
                >&2 echo "    Storing arduino-cli data for this project in ''${_ARDUINO_PROJECT_DIR}"
              fi
            '';
          };

        in
        {
          devShells.default = devShellArduinoCLI;

          formatter =
            lib.warnIf (builtins.hasAttr "nixfmt-tree" pkgs) "Replace nixfmt-rfc-style with nix-tree"
              pkgs.nixfmt-rfc-style;
        }
      );
}
