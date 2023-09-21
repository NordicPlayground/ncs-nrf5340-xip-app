nrfjprog -f NRF53 --recover
nrfjprog -f NRF53 --coprocessor CP_NETWORK --recover
nrfjprog -f NRF53 --coprocessor CP_NETWORK --sectorerase --program ./802154_rpmsg/zephyr/merged_CPUNET.hex
nrfjprog -f NRF53 --sectorerase --program ./mcuboot/zephyr/zephyr.hex
nrfjprog -f NRF53 --sectorerase --program ./zephyr/internal_flash_signed.hex
nrfjprog -f NRF53 --qspisectorerase --program ./zephyr/qspi_flash_signed.hex --qspiini ../Qspi.ini --verify
nrfjprog -f NRF53 --reset
