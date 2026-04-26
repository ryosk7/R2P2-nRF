# R2P2-nRF52

This project runs [R2P2](https://github.com/picoruby/R2P2) (Ruby Rapid Portable Platform) on nRF52840.
It uses [picoruby-nRF52](https://github.com/ryosk7/picoruby-nRF52), a component to run [PicoRuby](https://github.com/picoruby/picoruby) on nRF52.

## Getting Started

### Preparation

- `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, `arm-none-eabi-size`
- Ruby and `rake`
- Nordic nRF5 SDK 17.1.0 placed at:
  `components/picoruby-nRF52/nrf52/sdk/nRF5_SDK_17.1.0_ddde560`

### Setup

```sh
$ git clone --recursive https://github.com/ryosk7/R2P2-nRF52.git
```

### Build

```sh
$ cd R2P2-nRF52
$ make build
```

### Flash

1. Put the board into UF2 bootloader mode.
2. Copy `build/ssci_isp1807_dev_board/firmware.uf2` onto the mounted UF2 boot volume.
3. The board reboots and re-enumerates over USB.

### Verify

```sh
# USB CDC console
$ screen /dev/cu.usbmodem0000000000001 115200

# USB MSC volume
$ ls /Volumes/"R2P2 NRF52"
```

## Supported Environment

- **Build OS**: macOS
- **Device**: [SSCI ISP1807 Dev Board](https://www.switch-science.com/products/6794) (nRF52840)

## License

[R2P2-nRF52](https://github.com/ryosk7/R2P2-nRF52) is released under the [MIT License](https://github.com/ryosk7/R2P2-nRF52/blob/master/LICENSE).
