/*
 * File:        orc_plugin_sdk_compat.h
 * Module:      orc-stage-plugin-skeleton-passthrough
 * Purpose:     Header shim to locate decode-orc plugin SDK headers in both
 *              in-tree and installed-SDK build configurations
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 <Your Name>
 */

#pragma once

#include <cstdint>
#include <memory>

#if __has_include(<orc/plugin/orc_plugin_sdk.h>)
#include <orc/plugin/orc_plugin_sdk.h>
#elif __has_include(<orc_plugin_sdk.h>)
#include <orc_plugin_sdk.h>
#else
#error "Unable to locate decode-orc plugin SDK headers"
#endif

