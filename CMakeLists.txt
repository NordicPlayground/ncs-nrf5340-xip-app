#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_XIP_SPLIT_IMAGE)
  # Add MCUboot extra configuration file to disable image verification and use 3 images
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/extensions.cmake)
  add_overlay_config(mcuboot ${CMAKE_CURRENT_SOURCE_DIR}/modules/mcuboot/xip_image.conf)
endif()
