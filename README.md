# Openterface Core

Shared Core SDK for Openterface devices.

The Core is organized around profile/capability-driven hardware support so
the same codebase can serve Web/WASM, iPadOS, Linux, Windows, Local Service, and
future applications.

## What's here

| Path | Purpose |
|------|---------|
| `include/openterface/core.h` | Unified SDK public entry point |
| `include/openterface/capability.h` | Capability flags and runtime states |
| `include/openterface/profile.h` | Device family/profile registry |
| `include/openterface/input.h` | Unified input API (HID codes, packet builders, macro parsing) |
| `include/openterface/protocol_ch9329.h` | CH9329 protocol packet builders |
| `include/openterface/transport.h` | Platform-neutral transport abstraction |
| `include/openterface/device.h` | Device info helpers |
| `include/openterface/version.h` | Version query API |
| `include/openterface/core_native.h` | Native Core skeleton (device/hid/chip/video/usb_mode) |
| `include/openterface/watchdog.h` | Connection watchdog API |
| `include/openterface/hid.h` | HID session / transaction helpers |
| `include/openterface/chip.h` | Chip detection / controller helpers |
| `include/openterface/video_status.h` | Video input status polling |
| `include/openterface/usb_mode.h` | USB mode device-capability routing |
| `include/openterface/status.h` | Runtime status definitions |
| `include/openterface/native_common.h` | Common types for native layer |
| `src/core/` | Version/common Core helpers |
| `src/capability/` | Capability helpers |
| `src/device/` | Device info implementation |
| `src/profile/` | Device profile registry |
| `src/input/` | Unified input layer (HID lookup, packet building, macro parsing) |
| `src/protocol/ch9329/` | CH9329 protocol implementation |
| `src/transport/` | Generic transport dispatch |
| `src/platform/` | Internal platform backend (Linux / Windows / stub) |
| `src/hid/` | HID session and transaction guard |
| `src/chip/` | Chip detection and video chip controller |
| `src/video_status/` | Video input status polling |
| `src/usb_mode/` | USB mode controller (MS21xx + serial) |
| `src/watchdog/` | Connection watchdog |
| `cli/input_cli.c` | CLI demo / debugging tool |
| `tests/input_test.c` | Input module unit tests |
| `tests/native_core_test.c` | Native Core skeleton tests |
| `tests/watchdog_test.c` | Watchdog tests |
| `tests/core_skeleton_test.c` | Unified Core SDK skeleton tests |

## Library structure

The project builds into the following CMake targets:

| Target | Type | Description |
|--------|------|-------------|
| `openterface_core_common` | static lib | Version, capability, device info, profile, transport |
| `openterface_core_input` | static lib | Input event, HID lookup, packet building, macro parsing |
| `openterface_protocol_ch9329` | static lib | CH9329 protocol packet builders |
| `openterface_platform_internal` | static lib | Platform backend (Linux / Windows / stub) |
| `openterface_core_native` | static lib | Device session, chip detection, video status, USB mode control |
| `openterface_watchdog` | static lib | Connection watchdog |
| `input_cli` | executable | CLI demo tool (controlled by `-DBUILD_CLI`) |

## API layers

### Unified Core entry

```c
#include "openterface/core.h"

op_version_t version = op_core_version();
const op_device_profile_t *profile = op_profile_find_by_id("mini-kvm");
int can_switch = op_profile_has_capability(profile, OP_CAPABILITY_USB_ROLE_SWITCH);
```

### Input / protocol API

The canonical API lives under `op_input_*` and `op_ch9329_*`.

### Packet builders

```c
uint8_t pkt[OP_CH9329_PKT_KEYBOARD_SIZE];
uint8_t keys[] = { 0x06 };
op_ch9329_build_keyboard_packet(pkt, OP_INPUT_MOD_CTRL, keys, 1);
```

### HID code lookup

```c
int code = km_hid_code("Enter");           // 0x28
int code = km_hid_code_for_char('!', &s);  // 0x1E, s=1 (needs shift)
```

### Macro parsing

```c
km_parsed_token_t tokens[32];
int n = km_parse_macro("<CMD>V</CMD>", tokens, 32);
// tokens[0].hid_code = 0x19 (V), tokens[0].modifiers = KM_MOD_GUI
```

## Build

```bash
mkdir build && cd build
cmake ..
make
```

Build outputs go to `build/lib/` (static libraries) and `build/bin/` (executables).

### CLI demo

```bash
./bin/input_cli Enter             # single key packet
./bin/input_cli "<CTRL>C</CTRL>"  # macro packets
```

### Tests

```bash
cmake -S . -B build -DBUILD_NATIVE_CORE=ON -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

Available test targets:
- `input_test` — input module unit tests
- `core_skeleton_test` — unified SDK skeleton tests
- `openterface_native_core_test` — native core tests (when `BUILD_NATIVE_CORE=ON`)
- `watchdog_test` — watchdog tests (when `BUILD_NATIVE_CORE=ON`)

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_NATIVE_CORE` | `ON` | Build the native core library (device/hid/chip/video/usb_mode) |
| `BUILD_CLI` | `ON` | Build the CLI demo tool |
| `BUILD_TESTS` | `ON` | Build and register unit tests |

## Platform strategy

- Public ABI: C11 (`op_*`, `op_*_t`, `OP_*`).
- Pure Core: C11, suitable for WASM/iPadOS/Windows/Linux.
- Native platform backends: internal C/C++/Objective-C++ as needed, hidden behind
  the C ABI.
- Web: TypeScript providers call WASM Core and browser transports.
- Local Service: native service links full Core and exposes device graph/capabilities.

## Protocols

CH9329 is one protocol driver, not the whole device model. Device support should
be described through profiles and capabilities so mini-kvm, kvm-go, and future
hardware can select different transports/protocols/features.

CH9329 keyboard/mouse packets use:

- **Keyboard** (14 bytes): `57 AB 00 02 08 | mod | 00 | k1..k6 | checksum`
- **Mouse rel** (11 bytes): `57 AB 00 05 05 | 01 | btn | dx | dy | wheel | checksum`
- **Checksum**: sum of all bytes except last, masked to 8 bits
