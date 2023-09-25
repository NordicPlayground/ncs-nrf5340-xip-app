# QSPI XIP with internal flash application support

This repository demonstrates a way of splitting an application for nRF5340 so that it partially resides on internal flash and partially on QSPI.
The repository includes a sample application that shows how this functionality can be used in other projects and how it can be set up.

## Requirements

The following is needed for adapting the sample application to your project:

* One nRF5340-based board with SPI flash chip attached through a dedicated QSPI peripheral, with DTS correctly configured for the connected flash chip.
  The board does not have to run in the QSPI mode; can run in the SPI or DSPI modes also.
* An application for the network core.
* An application for the application core.
* A static partition manager configuration file, with the internal flash and the QSPI flash correctly partitioned.
* A linker file that is set up to specify the location of the QSPI XIP code.
* Code relocation in CMake to specify what files to relocate to QSPI XIP.
* MCUboot set up as bootloader.

### QSPI flash setup

You must correctly set up the QSPI flash chip in the board devicetree file, including the operating mode.
The flash chip does not have to run in the QSPI mode for XIP to function, but using the mode will reduce the execution speed of the application.

The following snippet shows an example of the configuration for Nordic Thingy:53 that supports DSPI:

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

**Note:** Due to YOPAN-159, the QSPI peripheral must be ran with ``HFCLK192MCTRL=0``.
Setting this to any other value may cause undefined operation of the device.

### Static partition manager setup

You must create a static configuration for the partition manager.
This configuration must have 3 images of 2 slots each:

* The first set of slots is for the internal flash portion of the application.
  These slots should be named ``mcuboot_primary`` and ``mcuboot_secondary``.
* The second set of slots is for the network core update.
  These slots should be named ``mcuboot_primary_1`` and ``mcuboot_secondary_1``.
* The third set of slots is for the QSPI XIP portion of the application.
  These slots should be named ``mcuboot_primary_2`` and ``mcuboot_secondary_2``.

The following snippet shows an example of the static configuration for the partition manager:

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

### Linker file setup

In the linker file, you must specify the start address and the size of the QSPI XIP region.
For nRF5340, the XIP peripheral is mapped to ``0x10000000``, which corresponds to ``0x0`` in the QSPI flash chip (the QSPI peripheral supports an offset address, but this is not supported in this mode.)
Make sure to offset the location by the value of ``mcuboot_primary_2`` and ``mcuboot_pad``, as an MCUboot header needs to be prepended to the image.

The following snippet shows an example of the file configuration:

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

### Code relocation setup

Relocating code to QSPI XIP is part of the project's ``CMakeLists.txt`` file.
You can set up the relocation on a file or library basis using the ``zephyr_code_relocate()`` function.

For example, to relocate a file in the application, you can use the following configuration:

```
zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION RAM_DATA)
```

### MCUboot setup

You must configure MCUboot for 3 images, corresponding to the following areas:

1. Internal flash
2. Network core
3. QSPI flash

Partial updates of either the internal flash or the QSPI flash are not supported and will likely cause the module to fault.
You must update both sections when performing a firmware update.

### Firmware update setup

TODO

### Troubleshooting

Here are solutions to some common issues.

#### Module does not appear to start

A common issue when using QSPI XIP in an application is that the module does not appear to start or continously ends in a fault before the application can run.

This is likely to be caused by priority mismatch: the ``init`` priority of the code residing on the QSPI flash is lower than the ``init`` priority of the QSPI flash device itself.

To debug this issue, you can use a debugger such as GDB to single-step through the application code until a QSPI address is encountered. At that point, the backtrace functionality can show what part of the code is responsible for the issue, and you can adjust the ``init`` priority of that module accordingly.

Given that the QSPI flash ``init`` priority defaults to ``41`` at the ``POST_KERNEL`` level, take into account the following points:

* There should be no QSPI flash residing code that has an ``init`` priority value that is less than or equal to the ``POST_KERNEL`` level.
* No interrupt handlers in the QSPI flash should be enabled until the QSPI flash driver has been initialized.
