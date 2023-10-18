/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/*
 * nRF5340dk is shipping a QSPI external flash mapped at 0x1000_0000 that can
 * be used for XIP
 */
MEMORY
{
     EXTFLASH (wx) : ORIGIN = 0x10120200, LENGTH = 0x3FE00
}

#include <zephyr/arch/arm/aarch32/cortex_m/scripts/linker.ld>
