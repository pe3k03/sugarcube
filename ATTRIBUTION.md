# Attribution

SugarCube is a modified Nightscout build of the SmartClock firmware. The two licenses cover different people.

| Work | Who | License |
| --- | --- | --- |
| SmartClock firmware | B. van Weerd | MIT, in `LICENSE` |
| SugarCube changes and new code | Peter Štieber | PolyForm Noncommercial 1.0.0, in `LICENSE-SUGARCUBE` |
| Press Start 2P | Cody "CodeMan38" Kalama and The Press Start 2P Project Authors | SIL Open Font License 1.1, in `src/fonts/OFL.txt` |
| Visitor TT1 glyph shapes | Brian Kent | Freeware, https://www.dafont.com/visitor.font |

The MIT grant on B. van Weerd's code stays in force. SugarCube does not take that away. Someone can still use, share, and sell the original SmartClock code if they keep the MIT notice. They cannot sell Peter Štieber's SugarCube changes, or pass those changes off as their own product. Personal use, hobby builds, and other noncommercial sharing of the SugarCube changes are allowed. Peter Štieber can still license his own changes commercially.

Required Notice: Copyright (c) 2026 Peter Štieber. SugarCube changes may not be sold or used commercially. https://github.com/pseter17/sugarcube

Compared with [iodn/geekmagic-tv-esp8266](https://github.com/iodn/geekmagic-tv-esp8266), which carries the same MIT copyright as [bvweerd/geekmagic-tv-esp8266](https://github.com/bvweerd/geekmagic-tv-esp8266).

## Upstream files without SugarCube code changes

Copyright (c) 2026 B. van Weerd. MIT only. A short attribution comment was added at the top. The program code matches the upstream file.

- `platformio.ini`
- `scripts/compress-webui.py`
- `src/auth.cpp`
- `src/auth.h`
- `src/button.cpp`
- `src/button.h`
- `src/device-themes.json`
- `src/logger.cpp`
- `src/logger.h`
- `src/webserver.h`

## Upstream files with SugarCube changes

Upstream code in these files stays MIT, copyright (c) 2026 B. van Weerd. The SugarCube edits are copyright (c) 2026 Peter Štieber and are PolyForm Noncommercial 1.0.0.

- `.gitignore`
- `README.md`
- `src/config.h`
- `src/dashboard.cpp`
- `src/dashboard.h`
- `src/display.cpp`
- `src/display.h`
- `src/feeds.cpp`
- `src/feeds.h`
- `src/main.cpp`
- `src/settings.cpp`
- `src/settings.h`
- `src/webserver.cpp`
- `src/webui.h`

## SugarCube files

Copyright (c) 2026 Peter Štieber. PolyForm Noncommercial 1.0.0.

- `scripts/gen_pressstart2p.py`
- `src/pixelfont.h` for the SugarCube-specific code. The Visitor glyph shapes stay Brian Kent's freeware and are not relicensed.

## Fonts

These are not SugarCube copyright and are not under the noncommercial license.

- `src/fonts/PressStart2P-Regular.ttf`
- `src/fonts/PressStart2P8.h`
- `src/fonts/OFL.txt`
