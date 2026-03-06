/*
 * gFbo.h
 *
 *  Created on: 23 Feb 2021
 *      Author: Noyan
 *
 *  Type alias header — resolves to gFboGL or gFboVk at compile time.
 */

#ifndef GRAPHICS_GFBO_H_
#define GRAPHICS_GFBO_H_

#if defined(GLIST_VULKAN)
#include "vk/gFboVk.h"
using gFbo = gFboVk;
#else
#include "gl/gFboGL.h"
using gFbo = gFboGL;
#endif

#endif /* GRAPHICS_GFBO_H_ */
