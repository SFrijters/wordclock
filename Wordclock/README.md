# Preparing the Wordclock with nix

## Flashing

* Enter a development shell via `nix develop`.

* Run `make` with a subcommand or plain `make` to do all of the following in order:
    * `make spiffs`: Prepare and flash the SPIFFS that contains the website files.
    * `make compile`: Compile the sketch.
    * `make upload`: Upload the sketch.

* (optional) Run `make monitor` to attach a serial monitor. Detach using `[C-a] [C-x]`.

## Choosing clock settings

From a WiFi-cabable device:

* Connect to the Wordclock-xxxxxxx network.
* Go to 192.168.4.1 in the browser of your choice.
* Choose your color settings / advanced settings.
* Choose a WiFi network that the Wordclock can connect to get the current time and supply the SSID/password.
* Click 'Connect'.
* Connect to your usual WiFi network again.

After this, the Wordclock will disable its own access point features, and instead connect to the chosen network and - after getting the current time via NTP - act like a clock.

You can now connect to the Wordclock using its IP address on your usual WiFi network, or the hostname 'woordklok'.

The Wordclock stores the SSID/password in non-volatile memory, so it should reconnect after power cycling. If it can't, it will revert to acting as an access point.
