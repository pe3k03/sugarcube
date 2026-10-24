# SugarCube

Nightscout firmware for the GeekMagic SmallTV Ultra (ESP8266, 240x240 ST7789).

The cube shows one glucose face. A web dashboard sets the Nightscout site, refresh interval, display, Wi-Fi, and system options. Firmware updates go through the browser update page.

This is a modified version of the SmartClock firmware by B. van Weerd: [bvweerd/geekmagic-tv-esp8266](https://github.com/bvweerd/geekmagic-tv-esp8266). His code stays MIT. SugarCube's own changes are separate and may not be sold. `ATTRIBUTION.md` lists which files are which.

## Build

PlatformIO environment `nodemcuv2`:

```bash
pio run -e nodemcuv2
```

The firmware image is `.pio/build/nodemcuv2/firmware.bin`. Upload it from the device dashboard. Wi-Fi credentials, the admin password, and the Nightscout token are stored on the device and are not part of this repository.

## Fonts

The glucose face uses Visitor TT1 by Brian Kent (freeware). Press Start 2P by Cody "CodeMan38" Kalama is included under the SIL Open Font License 1.1. See `src/fonts/OFL.txt`.

## License

Two licenses apply, split by author. See `ATTRIBUTION.md`.

- SmartClock code by B. van Weerd is MIT. See `LICENSE`.
- SugarCube changes by Peter Štieber are PolyForm Noncommercial 1.0.0. See `LICENSE-SUGARCUBE`. Those changes may be shared and modified for personal and other noncommercial use. They may not be sold or used commercially. The original author of those changes can still license them commercially.
