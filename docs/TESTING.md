# SNAK3 1.0.6: Compact Light Control

Developer: Traumlewis. SDK 4.44, pebble-tool 5.1.2.

The ON and SYSTEM labels were removed from `draw_light_control`.
The lightbulb and Down arrow remain. The arrow now sits directly beside the
lightbulb, and the black background area is 34 pixels wide instead of 80.
Backlight behavior, saved preferences and button mappings are unchanged.

All seven platforms built successfully: `aplite`, `basalt`, `chalk`, `diorite`,
`flint`, `emery` and `gabbro`. The PBW metadata was verified as
SNAK3 / Traumlewis / 1.0.6, retaining the original appstore UUID:
`b83a2022-7b26-45ff-bceb-dfee33249857`.

All 44 existing native checks still pass: 11 checks for each of the color,
monochrome, round-display and emery configurations, with AddressSanitizer
and UndefinedBehaviorSanitizer enabled. LeakSanitizer is disabled because
of the test environment's ptrace restriction.

Targeted emulator checks were performed on Time 2 (firmware 4.44.3) and
Time Round (firmware 4.3.0):

- Both lightbulb states on the start screen.
- Both lightbulb states on the pause screen.
- Switching modes with Down, verified using the filled and outlined bulbs in screenshots.
- Visual inspection of the control within the rectangular and round safe display areas.

The backlight logic was tested across all seven platform emulators in 1.0.5
and was not changed for this visual update. Testing on physical watches
has not yet been performed for this release.
