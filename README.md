# R2P2-nRF52

PicoRuby runtime on nRF52840 with USB CDC console and MSC storage.

This repository is the **product layer** of R2P2 for nRF52. It owns USB device composition, CDC console, MSC storage, boot UX, and the release shape. The PicoRuby VM port, HAL, and board glue live in the [`picoruby-nRF52`](https://github.com/ryosk7/picoruby-nRF52) submodule under `components/`.

## Features

- USB CDC console — connects as two `cu.usbmodem*` device nodes; prints `[r2p2] usb console open` on open
- USB MSC storage — mounts as a FAT12 volume labelled `R2P2 NRF52`
- Composite CDC + MSC — both interfaces stay live concurrently
- PicoRuby REPL — interactive `$>` shell with LittleFS root volume (`/bin`, `/etc`, `/home`, `/lib`, `/var`)
- Timer-driven mruby/c scheduling via `app_timer` + `mrbc_tick()`

## Hardware

| Item | Value |
|------|-------|
| MCU | nRF52840 (Cortex-M4, 64 MHz, 1 MB flash, 256 KB RAM) |
| Board | [SSCI ISP1807 Dev Board](https://www.switch-science.com/products/6794) |
| Bootloader | UF2 (drag-and-drop flashing) |
| USB VID/PID | 0x16c0 / 0x27dd |

## Prerequisites

- `git` with submodule support
- `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, `arm-none-eabi-size`
- Ruby and `rake` (for the PicoRuby build)
- Nordic nRF5 SDK 17.1.0 placed at:
  `components/picoruby-nRF52/nrf52/sdk/nRF5_SDK_17.1.0_ddde560`

Download the SDK from Nordic's website and extract it to the path above before building.

## Getting Started

```bash
git clone --recurse-submodules https://github.com/ryosk7/R2P2-nRF52.git
cd R2P2-nRF52
# place nRF5 SDK 17.1.0 under components/picoruby-nRF52/nrf52/sdk/
make build
```

If you cloned without `--recurse-submodules`:

```bash
git submodule update --init --recursive
```

## Build

```bash
make build
```

Generated artifacts (default board: `ssci_isp1807_dev_board`):

| File | Format |
|------|--------|
| `build/ssci_isp1807_dev_board/firmware.out` | ELF |
| `build/ssci_isp1807_dev_board/firmware.hex` | Intel HEX |
| `build/ssci_isp1807_dev_board/firmware.uf2` | UF2 |

To select a different board:

```bash
make build BOARD=<board_name>
```

To force a clean rebuild:

```bash
make clean && make build
```

## Flash

1. Put the board into UF2 bootloader mode (double-tap the reset button).
2. Copy the UF2 image onto the mounted boot volume:

```bash
cp build/ssci_isp1807_dev_board/firmware.uf2 /Volumes/<UF2_VOLUME>/
```

3. The board reboots and re-enumerates over USB automatically.

## Verify

### USB CDC

```bash
ls /dev/cu.usbmodem*
screen /dev/cu.usbmodem0000000000001 115200
```

Expected output:

```
[r2p2] usb console open
...
Initializing FLASH disk as the root volume...
Available
PicoRuby shell logo
$>
```

### USB MSC

```bash
diskutil list
diskutil mountDisk /dev/diskX   # replace X with the disk number
ls /Volumes/"R2P2 NRF52"
cat /Volumes/"R2P2 NRF52"/README.TXT
```

### Composite (CDC + MSC simultaneously)

Keep `screen` open on the CDC port, then from another terminal:

```bash
cat /Volumes/"R2P2 NRF52"/README.TXT
echo test > /Volumes/"R2P2 NRF52"/TEST.TXT
cat /Volumes/"R2P2 NRF52"/TEST.TXT
```

Both interfaces remain live concurrently.

### Shell Storage

Inside the PicoRuby shell:

```
$> ls /
$> ls /bin
$> cat /etc/machine-id
```

Expected: `/` contains `bin`, `etc`, `home`, `lib`, `var`; `/etc/machine-id` holds a 16-character hex device ID.

## Project Structure

```
R2P2-nRF52/
├── src/                # Product/runtime C source
│   ├── main.c
│   ├── hal.c           # app_timer → mrbc_tick() scheduling
│   ├── usb_runtime.c
│   ├── usb_device.c
│   ├── usb_cdc_transport.c
│   └── usb_msc_storage.c
├── include/            # Shared headers
├── components/
│   └── picoruby-nRF52/ # Submodule: PicoRuby VM port + HAL
│       ├── picoruby/   # Nested submodule: PicoRuby VM
│       ├── mrblib/main_task.rb   # PicoRuby startup script
│       └── build_config/nrf52.rb # mruby/c cross-compile config
├── build_config/       # Board-specific make variables
├── ports/              # Board-specific runtime code
├── docs/               # Build workflow and porting notes
├── tools/              # uf2conv.rb
└── Makefile
```

## Related Repositories

| Repository | Role |
|------------|------|
| [ryosk7/picoruby-nRF52](https://github.com/ryosk7/picoruby-nRF52) | PicoRuby VM port, HAL, board glue |
| [picoruby/picoruby](https://github.com/picoruby/picoruby) | PicoRuby VM |

## Troubleshooting

**`make: Nothing to be done for 'build'`** — outputs are up to date. Run `make clean` first for a full rebuild.

**`puts` output stops after editing PicoRuby files** — rebuild PicoRuby first:

```bash
cd components/picoruby-nRF52/picoruby && rake
cd - && make build
```

**USB CDC or MSC stalls after PicoRuby starts** — confirm that the repeated `app_timer` callback in `src/hal.c` is still calling `mrbc_tick()`.

**Terminal size not probed dynamically** — the fallback in upstream PicoRuby returns `[24, 80]` when ANSI cursor probing fails over CDC. This is expected behavior.

## License

See individual submodule licenses for PicoRuby and the nRF5 SDK.
