# SugarCube

Nightscout firmware for the GeekMagic SmallTV Ultra (ESP8266, 240x240 ST7789).

The cube shows one glucose face. A web dashboard sets the Nightscout site, refresh interval, display, Wi-Fi, and system options. Firmware updates go through the browser update page.

This is a modified version of the SmartClock firmware by B. van Weerd, published under the MIT license: [bvweerd/geekmagic-tv-esp8266](https://github.com/bvweerd/geekmagic-tv-esp8266). The copyright and permission notice in `LICENSE` stay with this project.

## Build

PlatformIO environment `nodemcuv2`:

```bash
pio run -e nodemcuv2
```

The firmware image is `.pio/build/nodemcuv2/firmware.bin`. Upload it from the device dashboard. Wi-Fi credentials, the admin password, and the Nightscout token are stored on the device and are not part of this repository.

## Fonts

The glucose face uses Visitor TT1 by Brian Kent (freeware). Press Start 2P by Cody "CodeMan38" Kalama is included under the SIL Open Font License 1.1.

## License

MIT. See `LICENSE`.
