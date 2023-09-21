# QSPI XIP with internal flash application support

This repository demonstrates a way of splitting an application for the nRF5340 to partially reside on internal flash and partially on QSPI.

## Requirements

The following requirements are needed to use and adapt this to your project:

* nRF5340-based board with SPI flash chip attached via dedicated QSPI peripheral (does not have to run in QSPI mode, can run in SPI or DSPI modes also), dts correctly configured for the connected flash chip
* Network core application
* Application core application
* Static partition manager configuration file with internal flash and QSPI flash correctly partitioned
* Linker file setup to specify location of QSPI XIP code
* Code relocation in cmake to specify what files to relocate to QSPI XIP
* MCUboot as bootloader

This repository serves as a sample application demonstrating how this functionality can be used in other projects and shows how the above can be setup.

### QSPI flash:

The QSPI flash chip must be correctly setup in the board devicetree file, this includes the operating mode, the flash chip does not have to run in QSPI mode for XIP to function, but will reduce execution speed of the application. An example of a configuration for the thingy53 which supports DSPI is provided:

```
&qspi {
	status = "okay";
	pinctrl-0 = <&qspi_default>;
	pinctrl-1 = <&qspi_sleep>;
	pinctrl-names = "default", "sleep";
	mx25r64: mx25r6435f@0 {
		compatible = "nordic,qspi-nor";
		reg = <0>;
		writeoc = "pp2o";
		readoc = "read2io";
		sck-frequency = <8000000>;
		jedec-id = [c2 28 17];
		sfdp-bfp = [
			e5 20 f1 ff  ff ff ff 03  44 eb 08 6b  08 3b 04 bb
			ee ff ff ff  ff ff 00 ff  ff ff 00 ff  0c 20 0f 52
			10 d8 00 ff  23 72 f5 00  82 ed 04 cc  44 83 68 44
			30 b0 30 b0  f7 c4 d5 5c  00 be 29 ff  f0 d0 ff ff
		];
		size = <67108864>;
		has-dpd;
		t-enter-dpd = <10000>;
		t-exit-dpd = <35000>;
	};
};
```

**Note:** Due to YOPAN-159 the QSPI peripheral must be ran with HFCLK192MCTRL=0, setting this to any other value may cause undefined operation of the device.

### Static partition manager:

A partition manager static configuration must be created which has 3 images of 2 slots each, the first slot is where the internal flash portion of the application will be, the second slot is for the network core update, and third slot is where the QSPI XIP porition of the application will be, these slots should be named "mcuboot_primary" and "mcuboot_secondary", "mcuboot_primary_1" and "mcuboot_secondary_1", "mcuboot_primary_2" and "mcuboot_secondary_2". An example static PM file is given:

```
app:
  address: 0x20200
  region: flash_primary
  size: 0xdfe00
mcuboot:
  address: 0x0
  end_address: 0x20000
  region: flash_primary
  size: 0x20000
mcuboot_pad:
  address: 0x20000
  region: flash_primary
  size: 0x200
mcuboot_primary:
  address: 0x20000
  orig_span: &id001
  - mcuboot_pad
  - app
  end_address: 0x100000
  region: flash_primary
  size: 0xe0000
  span: *id001
mcuboot_primary_app:
  address: 0x20200
  orig_span: &id002
  - app
  end_address: 0x100000
  region: flash_primary
  size: 0xdfe00
  span: *id002
mcuboot_primary_1:
  address: 0x0
  size: 0x40000
  device: flash_ctrl
  region: ram_flash
mcuboot_primary_2:
  address: 0x00000
  end_address: 0xe000
  device: MX25R64
  region: external_flash
  size: 0xe0000
mcuboot_secondary:
  address: 0xe0000
  end_address: 0x1c0000
  device: MX25R64
  region: external_flash
  size: 0xe0000
mcuboot_secondary_2:
  address: 0x1c0000
  end_address: 0x2a0000
  device: MX25R64
  region: external_flash
  size: 0xe0000
mcuboot_secondary_1:
  address: 0x2a0000
  end_address: 0x2e0000
  device: MX25R64
  region: external_flash
  size: 0x40000
external_flash_urd:
  address: 0x2e0000
  size: 0x100000
  device: MX25R64
  region: external_flash
external_flash:
  address: 0x3e0000
  size: 0x420000
  device: MX25R64
  region: external_flash
sram_primary:
  address: 0x20000000
  end_address: 0x20040000
  region: sram_primary
  size: 0x40000
```

### Linker file:

The linker file must specify the start address and size of the QSPI XIP region, for the nRF5340, the XIP peripheral is mapped to 0x10000000 which corresponds to 0x0 in the QSPI flash chip - the QSPI peripheral supports an offset address but this is not supported in this mode. The location should be offset by the value of "mcuboot_primary_2" and "mcuboot_pad" as an MCUboot header needs to be prepended to the image. An example file is provided:

```
#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
     EXTFLASH (wx) : ORIGIN = 0x10120200, LENGTH = 0xFFE00
}

#include <zephyr/arch/arm/aarch32/cortex_m/scripts/linker.ld>
```

### Code relocation:

Relocating code to QSPI XIP is part of the projects CMakeLists.txt file, relocation can be done on a file or library basis using the zephyr_code_relocate() function. For example, to relocate a file in the application:

```
zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION RAM_DATA)
```

### MCUboot:

MCUboot must be configured for 3 images which correspond to:

1. Internal flash
2. Network core
3. QSPI flash

Partial updates of either the internal flash or QSPI flash are not supported and will likely cause the module to fault, updates of both sections are required when performing a firmware update.

### Firmware updates:

TODO

### Troubleshooting:

A common issue when using QSPI XIP in an application is that the module does not appear to start, or faults continously before the application can run, this is likely to be caused by an init priority of code residing on the QSPI flash being lower than the init priority of the QSPI flash device itself. To debug this issue, a debugger such as gdb can be used to single step through the application code until either a QSPI address is encountered, at which point the backtrace functionality can show what part of the code is responsible for this, and init priority of that module adjusted accordingly. The QSPI flash init priority defaults to 41 at the POST_KERNEL level, there should be no QSPI flash residing code that has an init priority value that is less than or equal to this, and no interrupt handlers in QSPI flash should be enabled until the QSPI flash driver has been initialised.
