/*
 * gSkyTextureVk.h
 *
 *  Created on: 2026-03-01
 *      Author: Metehan Gezer
 *
 *  Refactored: Vulkan implementation
 */

#ifndef GRAPHICS_VK_GSKYTEXTUREVK_H_
#define GRAPHICS_VK_GSKYTEXTUREVK_H_

#if defined(GLIST_VULKAN)

#include "gSkyTextureBase.h"


class gSkyTextureVk : public gSkyTextureBase {
public:
	gSkyTextureVk();
	~gSkyTextureVk();

	uint32_t create();

	void bind();
	void bind(int textureSlot);
	void unbind();
	void unbind(int textureSlotNo);

	void texImage2D(int faceIndex, int internalFormat, int width, int height,
			int format, int type, void* data);

	void setWrappingAndFiltering(int wrapS, int wrapT, int wrapR,
			int minFilter, int magFilter);

	void generateMipMap();

	void destroy();

private:
	static uint32_t nextid;
};

#endif // GLIST_VULKAN
#endif /* GRAPHICS_VK_GSKYTEXTUREVK_H_ */
