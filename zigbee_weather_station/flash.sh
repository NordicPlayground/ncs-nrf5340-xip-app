nrfjprog -f NRF53 --coprocessor CP_NETWORK --sectorerase --program ./multiprotocol_rpmsg/zephyr/merged_CPUNET.hex
nrfjprog -f NRF53 --sectorerase --program ./mcuboot/zephyr/zephyr.hex
nrfjprog -f NRF53 --sectorerase --program ./intflash_signed.hex
nrfjprog -f NRF53 --qspisectorerase --program ./qspi.hex --qspiini ../Qspi.ini --verify
nrfjprog -f NRF53 --reset
