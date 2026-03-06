/*
 * gUboVk.h
 *
 *  Created on: 21.12.2023
 *      Author: irrel
 *
 *  Refactored: Vulkan implementation
 */

#pragma once

#if defined(GLIST_VULKAN)

#include "gUboBase.h"
#include "gVulkanRenderEngine.h"

#include <cstring>

template<typename T>
class gUboVk : public gUboBase<T> {
public:
	template<typename ...Args>
	gUboVk(Args&&... args, int bindingpoint, int usage = 0) : bufferslot(0) {
		engine = static_cast<gVulkanRenderEngine*>(renderer);
		this->size = sizeof(T);
		this->bindingpoint = bindingpoint;
		this->data = new T(std::forward<Args>(args)...);

		bufferslot = engine->allocBufferSlot();

		VkBufferCreateInfo bufinfo{};
		bufinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufinfo.size = this->size;
		bufinfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocinfo{};
		allocinfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		auto& slot = engine->getBufferSlot(bufferslot);
		VmaAllocationInfo allocationInfo;
		vmaCreateBuffer(engine->getVmaAllocator(), &bufinfo, &allocinfo,
			&slot.buffer, &slot.allocation, &allocationInfo);
		slot.size = this->size;
		slot.mapped = allocationInfo.pMappedData;

		engine->setUBOBindingHandle(bindingpoint, bufferslot);
		update();
	}

	~gUboVk() {
		if (bufferslot != 0) {
			engine->freeBufferSlot(bufferslot);
			bufferslot = 0;
		}
	}

	void update() {
		update(0, this->size);
	}

	void update(int offset, int length) {
		if (bufferslot == 0) return;
		auto& slot = engine->getBufferSlot(bufferslot);
		if (slot.mapped && this->data) {
			void* ptr = reinterpret_cast<char*>(this->data) + offset;
			memcpy(static_cast<uint8_t*>(slot.mapped) + offset, ptr, length);
		}
	}

	void setData(T* newObject) {
		this->data = newObject;
		update();
	}

private:
	gVulkanRenderEngine* engine;
	uint32_t bufferslot;
};

#endif // GLIST_VULKAN
