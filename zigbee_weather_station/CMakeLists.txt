#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

################################################################################

# The application uses the configuration/<board> scheme for configuration files.
set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/configuration/\${BOARD}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ZigbeeWeatherStation)

################################################################################

# NORDIC SDK APP START
target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sensor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/weather_station.c
)

# NORDIC SDK APP END

zephyr_code_relocate(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sensor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/weather_station.c
                     LOCATION EXTFLASH_TEXT NOCOPY)

zephyr_code_relocate(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sensor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/weather_station.c
                     LOCATION RAM_DATA)

zephyr_code_relocate(LIBRARY drivers__ieee802154 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY drivers__sensor__adxl362 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY drivers__sensor__bme680 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY drivers__sensor__bmm150 LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY ..__nrf__lib__dk_buttons_and_leds LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__net LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__net__ip LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY ..__nrfxlib__zboss__production LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY zboss-sources LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY nrf-802154-platform LOCATION EXTFLASH_TEXT NOCOPY)

zephyr_code_relocate(LIBRARY drivers__ieee802154 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY drivers__sensor__adxl362 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY drivers__sensor__bme680 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY drivers__sensor__bmm150 LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY ..__nrf__lib__dk_buttons_and_leds LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__net LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__net__ip LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY ..__nrfxlib__zboss__production LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY zboss-sources LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY nrf-802154-platform LOCATION RAM_DATA)
