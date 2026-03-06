/*
 * gVbo.h
 *
 *  Created on: 25 Kas 2020
 *      Author: Acer
 *
 *  Type alias header — resolves to gVboGL or gVboVk at compile time.
 */

#ifndef GRAPHICS_GVBO_H_
#define GRAPHICS_GVBO_H_

#if defined(GLIST_VULKAN)
#include "vk/gVboVk.h"
using gVbo = gVboVk;
#else
#include "gl/gVboGL.h"
using gVbo = gVboGL;
#endif

#endif /* GRAPHICS_GVBO_H_ */
