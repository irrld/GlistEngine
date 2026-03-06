/*
 * gShader.h
 *
 *  Created on: May 7, 2020
 *      Author: noyan
 *
 *  Type alias header — resolves to gShaderGL or gShaderVk at compile time.
 */

#ifndef ENGINE_GRAPHICS_GSHADER_H_
#define ENGINE_GRAPHICS_GSHADER_H_

#if defined(GLIST_VULKAN)
#include "vk/gShaderVk.h"
using gShader = gShaderVk;
#else
#include "gl/gShaderGL.h"
using gShader = gShaderGL;
#endif

#endif /* ENGINE_GRAPHICS_GSHADER_H_ */
