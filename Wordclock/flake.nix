{
  description = "Flake template for Arduino projects";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
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

          overlays = [
            arduino-nix.overlay
            # https://downloads.arduino.cc/packages/package_index.json
            (arduino-nix.mkArduinoPackageOverlay ./package-index/package_index.json)
            # https://arduino.esp8266.com/stable/package_esp8266com_index.json
            (arduino-nix.mkArduinoPackageOverlay ./package-index/package_esp8266com_index.json)
            # https://downloads.arduino.cc/libraries/library_index.json
            (arduino-nix.mkArduinoLibraryOverlay ./package-index/library_index.json)
          ];

          pkgs = import nixpkgs { inherit system overlays; };

          gnumake-wrapper = pkgs.writeShellApplication {
            name = "make";
            text = ''
              ${lib.getExe pkgs.gnumake} --file=${./Makefile} "$@"
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

          # The variables starting with underscores are custom and not used by arduino-cli directly
          # The _ARDUINO_PROJECT_DIR variable is passed to arduino-cli via the Makefile.
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
              python3
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

          devShellArduinoCLI-CI = pkgs.mkShell {
            name = "${name}-ci";
            packages = with pkgs; [
              arduino-cli-with-packages
              git
              gnumake-wrapper
              python3
              mkspiffs-presets.arduino-esp8266
            ];
          };
        in
        {
          devShells = {
            default = devShellArduinoCLI;
            ci = devShellArduinoCLI-CI;
          };

          formatter = pkgs.nixfmt-tree;
        }
      );
}
