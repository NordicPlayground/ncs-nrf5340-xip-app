cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(smp_svr)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE src/bluetooth.c)

# This places the application bluetooth.c file and some MCUmgr libraries into QSPI XIP
zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__transport LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__util LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__smp LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__img_mgmt LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__os_mgmt LOCATION EXTFLASH_TEXT NOCOPY)

zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__transport LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__util LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__smp LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__img_mgmt LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__os_mgmt LOCATION RAM_DATA)
