/*
 * gVboVk.h
 *
 *  Created on: 25 Kas 2020
 *      Author: Acer
 *
 *  Refactored: Vulkan implementation
 */

#ifndef GRAPHICS_VK_GVBOVK_H_
#define GRAPHICS_VK_GVBOVK_H_

#if defined(GLIST_VULKAN)

#include "gVboBase.h"
#include <vector>
#include <cstdint>

class gVulkanRenderEngine;

class gVboVk : public gVboBase {
public:
	gVboVk();
	~gVboVk();

	void setVertexData(const gVertex* vertices, int coordNum, int total);
	void setVertexData(const glm::vec3* vertices, int total, int usage = 0);
	void setVertexData(const glm::vec2* vertices, int total, int usage = 0);
	void setVertexData(const float* verticesptr, int coordNum, int total, int usage, int stride = 0);
	void setIndexData(const gIndex* indices, int total);

	void clear();

	void bind() const;
	void unbind() const;

	int getVAOid() const;

private:
	gVulkanRenderEngine* engine;
	uint32_t vaoslot;
	uint32_t vboslot;
	uint32_t eboslot;

	// CPU-side caches for dynamic mode
	std::vector<uint8_t> cachedvertexdata;
	int cachedvertexstride = 0;
	std::vector<uint8_t> cachedindexdata;
	bool cachedhasindices = false;
};

#endif // GLIST_VULKAN
#endif /* GRAPHICS_VK_GVBOVK_H_ */
