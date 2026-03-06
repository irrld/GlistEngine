/*
 * gUbo.h
 *
 *  Created on: 21.12.2023
 *      Author: irrel
 *
 *  Type alias header — resolves to gUboGL or gUboVk at compile time.
 */

#pragma once

#if defined(GLIST_VULKAN)
#include "vk/gUboVk.h"
template<typename T> using gUbo = gUboVk<T>;
#else
#include "gl/gUboGL.h"
template<typename T> using gUbo = gUboGL<T>;
#endif
