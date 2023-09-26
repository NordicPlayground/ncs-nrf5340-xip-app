# QSPI XIP with internal flash application support

This repository demonstrates a way of splitting an application for nRF5340 so that it partially resides on internal flash and partially on QSPI flash.
The repository includes a sample application that shows how this functionality can be used in other projects and how it can be set up.

## Getting the code

Follow the instructions on the [nRF Connect SDK documentation page](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/installing.html) to get the required tools. For the step 4 ("Get the nRF Connect SDK code"), you must instead use this repository's URL: `west init -m https://github.com/NordicPlayground/ncs-nrf5340-xip-app.git`.

## Requirements and setup

The following is needed for adapting the sample application to your project:

* One nRF5340-based board with SPI flash chip attached through dedicated QSPI peripheral, with DTS correctly configured for the connected flash chip.
  The board does not have to run in the QSPI mode; it can run in SPI or DSPI modes also.
* An application for the network core.
* An application for the application core.
* A static partition manager configuration file, with internal flash and QSPI flash correctly partitioned.
* A linker file that is set up to specify the location of the QSPI XIP code.
* Code relocation in CMake to specify what files to relocate to QSPI XIP.
* MCUboot set up as bootloader.

### QSPI flash setup

You must correctly set up the QSPI flash chip in the board devicetree file, including the operating mode.
The flash chip does not have to run in the QSPI mode for XIP to function, but using other modes will reduce the execution speed of the application.

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

**Note:** Due to a QSPI peripheral product anomaly, the QSPI peripheral must be ran with ``HFCLK192MCTRL=0``.
Setting this to any other value may cause undefined operation of the device.

### Static partition manager setup

You must create a static configuration for partition manager.
This configuration must have 3 images of 2 slots each:

* The first set of slots is for the internal flash portion of the application.
  These slots should be named ``mcuboot_primary`` and ``mcuboot_secondary``.
* The second set of slots is for the network core update.
  These slots should be named ``mcuboot_primary_1`` and ``mcuboot_secondary_1``.
* The third set of slots is for the QSPI XIP portion of the application.
  These slots should be named ``mcuboot_primary_2`` and ``mcuboot_secondary_2``.

The following snippet shows an example of the static configuration for partition manager:

```
app:
  address: 0x10200
  end_address: 0xe4000
  region: flash_primary
  size: 0xd3e00
external_flash:
  address: 0x120000
  device: MX25R64
  end_address: 0x800000
  region: external_flash
  size: 0x6e0000
mcuboot:
  address: 0x0
  end_address: 0x10000
  region: flash_primary
  size: 0x10000
mcuboot_pad:
  address: 0x10000
  end_address: 0x10200
  region: flash_primary
  size: 0x200
mcuboot_primary:
  address: 0x10000
  end_address: 0xf0000
  orig_span: &id001
  - mcuboot_pad
  - app
  region: flash_primary
  size: 0xe0000
  span: *id001
mcuboot_primary_1:
  address: 0x0
  device: flash_ctrl
  end_address: 0x40000
  region: ram_flash
  size: 0x40000
mcuboot_primary_app:
  address: 0x10200
  end_address: 0xf0000
  orig_span: &id002
  - app
  region: flash_primary
  size: 0xdfe00
  span: *id002
mcuboot_secondary:
  address: 0x0
  device: MX25R64
  end_address: 0xe0000
  region: external_flash
  size: 0xe0000
mcuboot_secondary_1:
  address: 0xe0000
  device: MX25R64
  end_address: 0x120000
  region: external_flash
  size: 0x40000
mcuboot_primary_2:
  address: 0x120000
  device: MX25R64
  end_address: 0x160000
  region: external_flash
  size: 0x40000
mcuboot_secondary_2:
  address: 0x160000
  device: MX25R64
  end_address: 0x1a0000
  region: external_flash
  size: 0x40000
otp:
  address: 0xff8100
  end_address: 0xff83fc
  region: otp
  size: 0x2fc
pcd_sram:
  address: 0x20000000
  end_address: 0x20002000
  region: sram_primary
  size: 0x2000
ram_flash:
  address: 0x40000
  end_address: 0x40000
  region: ram_flash
  size: 0x0
rpmsg_nrf53_sram:
  address: 0x20070000
  end_address: 0x20080000
  placement:
    before:
    - end
  region: sram_primary
  size: 0x10000
settings_storage:
  address: 0xf0000
  end_address: 0x100000
  region: flash_primary
  size: 0x10000
sram_primary:
  address: 0x20002000
  end_address: 0x20070000
  region: sram_primary
  size: 0x6e000
```

### Linker file setup

In the linker file, you must specify the start address and the size of the QSPI XIP region.
For nRF5340, the XIP peripheral is mapped to ``0x10000000``, which corresponds to ``0x0`` in the QSPI flash chip (the QSPI peripheral supports an offset address, but this is not supported in this code.)
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

### Firmware updates

You can apply firmware updates using the following methods:

* Using MCUmgr if the application has enabled it.
* Using MCUboot serial recovery, via UART, using the ``*_update.bin`` files.
* Using Bluetooth LE through the nRF Connect app for iOS or Android by using the ``dfu_application.zip`` file.

### Flashing to external flash in SPI/DSPI mode

When flashing applications using ``west``, this will invoke the ``nrfjprog`` runner.
This runner will use the system default configuration that will configure the application in the QSPI mode (when flashing the external flash).
This behavior can be changed using a custom ``Qspi.ini`` configuration file, but this will prevent flashing from being performed using ``west``. A sample ``Qspi.ini`` file is provided in the root of this repository. The file is set up to work on Nordic Thingy:53. If you decide to use the ``Qspi.ini`` file, the HEX files in the repository need to be manually flashed. For example, for the ``zigbee_weather_station`` application, the files to flash are the following (paths are relative to the build directory):

* 802154_rpmsg/zephyr/merged_CPUNET.hex
* mcuboot/zephyr/zephyr.hex
* zephyr/internal_flash_signed.hex
* zephyr/qspi_flash_signed.hex

For the ``smp_svr`` sample application, the files to flash are the following:

* hci_rpmsg/zephyr/merged_CPUNET.hex
* mcuboot/zephyr/zephyr.hex
* zephyr/internal_flash_signed.hex
* zephyr/qspi_flash_signed.hex

The follow commands can be used to flash and verify the application for ``zigbee_weather_station`` (adjusting the path to the ini file):

```
nrfjprog -f NRF53 --coprocessor CP_NETWORK --sectorerase --program 802154_rpmsg/zephyr/merged_CPUNET.hex --verify
nrfjprog -f NRF53 --sectorerase --program mcuboot/zephyr/zephyr.hex --verify
nrfjprog -f NRF53 --sectorerase --program zephyr/internal_flash_signed.hex --verify
nrfjprog -f NRF53 --qspisectorerase --program zephyr/qspi_flash_signed.hex --qspiini <path_to>/Qspi.ini --verify
nrfjprog -f NRF53 --reset
```

The following commands are for the ``smp_svr`` sample:

```
nrfjprog -f NRF53 --coprocessor CP_NETWORK --sectorerase --program hci_rpmsg/zephyr/merged_CPUNET.hex --verify
nrfjprog -f NRF53 --sectorerase --program mcuboot/zephyr/zephyr.hex --verify
nrfjprog -f NRF53 --sectorerase --program zephyr/internal_flash_signed.hex --verify
nrfjprog -f NRF53 --qspisectorerase --program zephyr/qspi_flash_signed.hex --qspiini <path_to>/Qspi.ini --verify
nrfjprog -f NRF53 --reset
```

**Note:** The external flash chip must be connected to the dedicated QSPI peripheral port pins of the nRF5340, it is not possible to program an external flash chip connected to different pins using ``nrfjprog``.

## Integration with own projects

The repository here serves as a starting point to add the QSPI XIP integration into other projects.
The way to do this is to use this repository as the basis for your own project manifest repository so that the CMake code and Kconfig configuration is available.

Pay attention to the following points:

* The included samples ``smp_svr`` and ``zigbee_weather_station`` can be removed, but all other files should be kept.
* The manifest can be freely updated to include other modules, update the NCS revision, or retract which modules of NCS are cloned. However, it must not use a revision older than ``18682391decaaa989c362ec8c5b65fd6203a5fdb``.
* A partition manager static configuration file must be provided for your application. You can check the example configuration in the [section above](#static-partition-manager-setup). This configuration must include 3 images of 2 slots each, whereby the third is located in the QSPI flash.
* A linker file must also be provided for your application. You can check the example configuration in the [section above](#linker-file-setup). This must have the start and end addresses of the QSPI XIP memory region where code starts and ends, including offsets for the MCUboot header, among others. This should be placed in the root of your application folder, in the same directory where the ``CMakeLists.txt`` file is located.
* The following Kconfig configuration must be set for your application:

```
CONFIG_CODE_DATA_RELOCATION=y
CONFIG_HAVE_CUSTOM_LINKER_SCRIPT=y
CONFIG_CUSTOM_LINKER_SCRIPT="linker_arm_nocopy.ld" # Change this if you have named your linker script differently
CONFIG_BUILD_NO_GAP_FILL=y
CONFIG_XIP=y
CONFIG_NORDIC_QSPI_NOR_XIP=y
CONFIG_UPDATEABLE_IMAGE_NUMBER=3
CONFIG_XIP_SPLIT_IMAGE=y
```

## Troubleshooting

Here are solutions to some common issues.

### Module does not appear to start

A common issue when using QSPI XIP in an application is that the module does not appear to start or continously ends in a fault before the application can run.

This is likely to be caused by priority mismatch: the ``init`` priority of the code residing on the QSPI flash is lower than the ``init`` priority of the QSPI flash device itself.

To debug this issue, you can use a debugger such as GDB to single-step through the application code until a QSPI address is encountered. At that point, the backtrace functionality can show what part of the code is responsible for the issue, and you can adjust the ``init`` priority of that module accordingly.

Given that the QSPI flash ``init`` priority defaults to ``41`` at the ``POST_KERNEL`` level, take into account the following points:
* There should be no QSPI flash residing code that has an ``init`` priority value that is less than or equal to the ``POST_KERNEL`` ``41`` level.
* No interrupt handlers in the QSPI flash should be enabled until the QSPI flash driver has been initialized.

### Module does not boot after update

This issue can arise if there is a mismatch between the internal flash code and QSPI XIP code. Both slots must be running the same build to successfully boot. If one of the updates is not loaded, or a different build is loaded to one of the slots, or one of the updates loaded is corrupt and deleted, then the application will fail to boot.
