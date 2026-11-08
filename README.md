# SNAK3

A Nokia-inspired Snake game for Pebble watches, by **Traumlewis**.

Current version: **1.0.6**.

## Download

Download [SNAK3-1.0.6.pbw](https://github.com/Traumlewis/SNAK3/releases/download/v1.0.6/SNAK3-1.0.6.pbw) from the [latest release](https://github.com/Traumlewis/SNAK3/releases/latest).
The PBW contains builds for all seven supported Pebble platforms.

| Platform | Watches |
| --- | --- |
| aplite | Pebble Classic, Pebble Steel |
| basalt | Pebble Time, Pebble Time Steel |
| chalk | Pebble Time Round |
| diorite | Pebble 2, including SE |
| flint | Pebble 2 Duo / Core 2 Duo |
| emery | Pebble Time 2 / Core Time 2 |
| gabbro | Pebble Round 2 |

## Controls

- Select: start, pause/resume, or start a new round after Game Over.
- Hold Select: return to the start screen.
- Up / Down during gameplay: turn left / right.
- Down on the start or pause screen: switch the light preference.
- Back: exit.

A filled lightbulb selects continuous backlight during active gameplay. An outlined bulb selects normal system backlight control. The selection is saved. Pausing, losing focus, Game Over, or closing the app releases the backlight override. System control does not mean permanently off.

The header shows Score, the current watch time, and High with side padding. Round displays use a safe playing field and omit the additional header divider.

## Build

Use pebble-tool 5.1.2 and Pebble SDK 4.44:

```sh
pebble build
pebble install --emulator emery
```

Select a different platform name to test another watch. The generated PBW filename follows the local project directory name.

## Validation

Version 1.0.6 builds for all seven platforms. Existing native logic and layout tests pass, with targeted visual checks on Time 2 and Time Round. The light control was tested across all seven platform emulators in 1.0.5.

```sh
gcc -std=c11 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I tests tests/logic_test.c -o tests/logic_test
./tests/logic_test
```

Also compile with `-DPBL_BW`, `-DPBL_ROUND`, or `-DPBL_PLATFORM_EMERY` to cover those variants. Environments that block LeakSanitizer under ptrace can run with `ASAN_OPTIONS=detect_leaks=0`; AddressSanitizer and UBSan remain active.

See [test report](docs/TESTING.md) and [changelog](CHANGELOG.md).

## Build integrity

The release includes `SHA256SUMS`. To verify the downloaded PBW:

```sh
sha256sum -c SHA256SUMS
```

## License

[MIT License](LICENSE), copyright 2026 Traumlewis.
