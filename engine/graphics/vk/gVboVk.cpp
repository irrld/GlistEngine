/*
 * gVboVk.cpp
 *
 *  Created on: 25 Kas 2020
 *      Author: Acer
 *
 *  Refactored: Vulkan implementation
 */

#if defined(GLIST_VULKAN)

#include "gVboVk.h"
#include "gVulkanRenderEngine.h"
#include "gTracy.h"

#include <cstring>

gVboVk::gVboVk() : vaoslot(0), vboslot(0), eboslot(0) {
	engine = static_cast<gVulkanRenderEngine*>(renderer);
	vaoslot = engine->allocVAOSlot();
}

gVboVk::~gVboVk() {
	clear();
}

void gVboVk::setVertexData(const gVertex* vertices, int coordNum, int total) {
	G_PROFILE_ZONE_SCOPED_N("gVbo::setVertexData()");
	isvertexdataallocated = true;
	vertexdatacoordnum = coordNum;
	totalvertexnum = total;

	if (isdynamic) {
		size_t size = totalvertexnum * sizeof(gVertex);
		cachedvertexdata.resize(size);
		if (vertices) {
			memcpy(cachedvertexdata.data(), vertices, size);
		}
		cachedvertexstride = sizeof(gVertex);
		return;
	}

	bind();
	if (vboslot != 0) {
		engine->freeBufferSlot(vboslot);
	}
	vboslot = engine->allocBufferSlot();

	size_t size = totalvertexnum * sizeof(gVertex);

	VkBufferCreateInfo bufinfo{};
	bufinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufinfo.size = size;
	bufinfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocinfo{};
	allocinfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	auto& slot = engine->getBufferSlot(vboslot);
	VmaAllocationInfo allocationInfo;
	vmaCreateBuffer(engine->getVmaAllocator(), &bufinfo, &allocinfo,
		&slot.buffer, &slot.allocation, &allocationInfo);
	slot.size = size;
	slot.mapped = allocationInfo.pMappedData;

	if (vertices && slot.mapped) {
		memcpy(slot.mapped, vertices, size);
	}

	auto& vao = engine->getVAOSlot(vaoslot);
	vao.vbo = vboslot;
	vao.vertexstride = sizeof(gVertex);
	unbind();
}

void gVboVk::setVertexData(const float* verticesptr, int coordNum, int total, int usage, int stride) {
	isvertexdataallocated = true;
	vertexdatacoordnum = coordNum;
	totalvertexnum = total;
	vertexusage = usage;
	vertexstride = stride;

	int pervertex = stride == 0 ? vertexdatacoordnum * sizeof(float) : stride;

	if (isdynamic) {
		size_t size = totalvertexnum * pervertex;
		cachedvertexdata.resize(size);
		if (verticesptr) {
			memcpy(cachedvertexdata.data(), verticesptr, size);
		}
		cachedvertexstride = pervertex;
		return;
	}

	bind();
	if (vboslot != 0) {
		engine->freeBufferSlot(vboslot);
	}
	vboslot = engine->allocBufferSlot();

	size_t size = totalvertexnum * pervertex;

	VkBufferCreateInfo bufinfo{};
	bufinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufinfo.size = size;
	bufinfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocinfo{};
	allocinfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	auto& slot = engine->getBufferSlot(vboslot);
	VmaAllocationInfo allocationInfo;
	vmaCreateBuffer(engine->getVmaAllocator(), &bufinfo, &allocinfo,
		&slot.buffer, &slot.allocation, &allocationInfo);
	slot.size = size;
	slot.mapped = allocationInfo.pMappedData;

	if (verticesptr && slot.mapped) {
		memcpy(slot.mapped, verticesptr, size);
	}

	auto& vao = engine->getVAOSlot(vaoslot);
	vao.vbo = vboslot;
	vao.vertexstride = pervertex;
	unbind();
}

void gVboVk::setVertexData(const glm::vec3* vertices, int total, int usage) {
	setVertexData(&vertices[0].x, 3, total, usage);
}

void gVboVk::setVertexData(const glm::vec2* vertices, int total, int usage) {
	setVertexData(&vertices[0].x, 2, total, usage);
}

void gVboVk::setIndexData(const gIndex* indices, int total) {
	if (indices == nullptr) {
		if (isdynamic) {
			cachedindexdata.clear();
			cachedhasindices = false;
		} else {
			if (eboslot != 0) {
				engine->freeBufferSlot(eboslot);
				auto& vao = engine->getVAOSlot(vaoslot);
				vao.ebo = 0;
				vao.hasindices = false;
				eboslot = 0;
			}
		}
		isindexdataallocated = false;
		return;
	}

	isindexdataallocated = true;
	totalindexnum = total;

	if (isdynamic) {
		size_t size = totalindexnum * sizeof(gIndex);
		cachedindexdata.resize(size);
		memcpy(cachedindexdata.data(), indices, size);
		cachedhasindices = true;
		return;
	}

	bind();
	if (eboslot != 0) {
		engine->freeBufferSlot(eboslot);
	}
	eboslot = engine->allocBufferSlot();

	size_t size = totalindexnum * sizeof(gIndex);

	VkBufferCreateInfo bufinfo{};
	bufinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufinfo.size = size;
	bufinfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocinfo{};
	allocinfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	auto& slot = engine->getBufferSlot(eboslot);
	VmaAllocationInfo allocationInfo;
	vmaCreateBuffer(engine->getVmaAllocator(), &bufinfo, &allocinfo,
		&slot.buffer, &slot.allocation, &allocationInfo);
	slot.size = size;
	slot.mapped = allocationInfo.pMappedData;

	if (indices && slot.mapped) {
		memcpy(slot.mapped, indices, size);
	}

	auto& vao = engine->getVAOSlot(vaoslot);
	vao.ebo = eboslot;
	vao.hasindices = true;
	unbind();
}

void gVboVk::clear() {
	if (isdynamic) {
		cachedvertexdata.clear();
		cachedindexdata.clear();
		cachedhasindices = false;
		cachedvertexstride = 0;
	} else {
		if (vboslot != 0) {
			engine->freeBufferSlot(vboslot);
			vboslot = 0;
		}
		if (eboslot != 0) {
			engine->freeBufferSlot(eboslot);
			eboslot = 0;
		}
	}
	if (vaoslot != 0) {
		engine->freeVAOSlot(vaoslot);
		vaoslot = 0;
	}
	isvertexdataallocated = false;
	isindexdataallocated = false;
}

void gVboVk::bind() const {
	if (isdynamic) {
		// Set cached data pointers in VAO for deferred flush to read
		auto& vao = engine->getVAOSlot(vaoslot);
		vao.isdynamic = true;
		vao.cachedvertexptr = cachedvertexdata.data();
		vao.cachedvertexsize = cachedvertexdata.size();
		vao.vertexstride = cachedvertexstride;
		if (cachedhasindices && !cachedindexdata.empty()) {
			vao.cachedindexptr = cachedindexdata.data();
			vao.cachedindexsize = cachedindexdata.size();
			vao.hasindices = true;
		} else {
			vao.cachedindexptr = nullptr;
			vao.cachedindexsize = 0;
			vao.hasindices = false;
		}
	}
	engine->setCurrentVAO(vaoslot);
}

void gVboVk::unbind() const {
	engine->setCurrentVAO(0);
}

int gVboVk::getVAOid() const {
	return vaoslot;
}

#endif // GLIST_VULKAN
