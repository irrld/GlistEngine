/*
 * gSkyTexture.h
 *
 *  Created on: 2026-03-01
 *      Author: Metehan Gezer
 *
 *  Type alias header — resolves to gSkyTextureGL or gSkyTextureVk at compile time.
 */

#ifndef GRAPHICS_GSKYTEXTURE_H_
#define GRAPHICS_GSKYTEXTURE_H_

#if defined(GLIST_VULKAN)
#include "vk/gSkyTextureVk.h"
using gSkyTexture = gSkyTextureVk;
#else
#include "gl/gSkyTextureGL.h"
using gSkyTexture = gSkyTextureGL;
#endif

#endif /* GRAPHICS_GSKYTEXTURE_H_ */
