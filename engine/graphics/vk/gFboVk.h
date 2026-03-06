/*
 * gFboVk.h
 *
 *  Created on: 23 Feb 2021
 *      Author: Noyan
 *
 *  Refactored: Vulkan implementation
 */

#ifndef GRAPHICS_VK_GFBOVK_H_
#define GRAPHICS_VK_GFBOVK_H_

#if defined(GLIST_VULKAN)

#include "gFboBase.h"


class gFboVk : public gFboBase {
public:
	gFboVk();
	~gFboVk();

	void allocate(int width, int height, bool isDepthMap = false, bool useDepthTexture = false);
	unsigned int getId() const;

	void bind();
	void unbind();

private:
	uint32_t id;
	static uint32_t nextid;
};

#endif // GLIST_VULKAN
#endif /* GRAPHICS_VK_GFBOVK_H_ */
