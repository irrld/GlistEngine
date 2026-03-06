/*
 * gTexture.h
 *
 *  Created on: May 10, 2020
 *      Author: noyan
 *
 *  Type alias header — resolves to gTextureGL or gTextureVk at compile time.
 */

#pragma once

#ifndef ENGINE_GRAPHICS_GTEXTURE_H_
#define ENGINE_GRAPHICS_GTEXTURE_H_

#if defined(GLIST_VULKAN)
#include "vk/gTextureVk.h"
using gTexture = gTextureVk;
#else
#include "gl/gTextureGL.h"
using gTexture = gTextureGL;
#endif

#endif /* ENGINE_GRAPHICS_GTEXTURE_H_ */
