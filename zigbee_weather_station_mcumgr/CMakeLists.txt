#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

################################################################################

# The application uses the configuration/<board> scheme for configuration files.
set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/configuration/\${BOARD}")

# Specify partition manager file
set(PM_STATIC_YML_FILE "${CMAKE_CURRENT_SOURCE_DIR}/pm_static.yml")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ZigbeeWeatherStationMcumgr)

################################################################################

# NORDIC SDK APP START
target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sensor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/weather_station.c
)

# NORDIC SDK APP END

# SMP_SRV APP START
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE src/bluetooth.c)

# SMP_SRV APP END

# ZIGBEE WEATHER STATION APP CODE RELOCATION START
zephyr_code_relocate(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sensor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/weather_station.c
                     LOCATION EXTFLASH_TEXT NOCOPY)

zephyr_code_relocate(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sensor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/weather_station.c
                     LOCATION RAM_DATA)

# zephyr_code_relocate(LIBRARY drivers__ieee802154 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY drivers__sensor__adxl362 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY drivers__sensor__bme680 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY drivers__sensor__bmm150 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY ..__nrf__lib__dk_buttons_and_leds LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY subsys__net LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY subsys__net__ip LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY ..__nrfxlib__zboss__production LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY zboss-sources LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY ..__nrf__subsys__zigbee__lib__zigbee_app_utils LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY ..__nrf__subsys__zigbee__lib__zigbee_fota LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY ..__nrf__subsys__zigbee LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY nrf-802154-platform LOCATION EXTFLASH_TEXT NOCOPY)

# zephyr_code_relocate(LIBRARY drivers__ieee802154 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY drivers__sensor__adxl362 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY drivers__sensor__bme680 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY drivers__sensor__bmm150 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY ..__nrf__lib__dk_buttons_and_leds LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY subsys__net LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY subsys__net__ip LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY ..__nrfxlib__zboss__production LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY zboss-sources LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY ..__nrf__subsys__zigbee__lib__zigbee_app_utils LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY ..__nrf__subsys__zigbee__lib__zigbee_fota LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY ..__nrf__subsys__zigbee LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY nrf-802154-platform LOCATION RAM_DATA)

# ZIGBEE WEATHER STATION APP CODE RELOCATION END

# SMP_SRV APP CODE RELOCATION START
# This places the application bluetooth.c file and some MCUmgr libraries into QSPI XIP
# zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__transport LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__util LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__smp LOCATION EXTFLASH_TEXT NOCOPY)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__img_mgmt LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__os_mgmt LOCATION EXTFLASH_TEXT NOCOPY)

# zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__transport LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__util LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__smp LOCATION RAM_DATA)
# zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__img_mgmt LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__os_mgmt LOCATION RAM_DATA)

# SMP_SRV APP CODE RELOCATION END


# EXTRA CODE RELOCATION START

zephyr_code_relocate(LIBRARY mcuboot_util LOCATION EXTFLASH_TEXT NOCOPY)

zephyr_code_relocate(LIBRARY mcuboot_util LOCATION RAM_DATA)

# EXTRA CODE RELOCATION END
