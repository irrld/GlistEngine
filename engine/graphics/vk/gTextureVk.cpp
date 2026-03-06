/*
 * gTextureVk.cpp
 *
 *  Created on: May 10, 2020
 *      Author: noyan
 *
 *  Refactored: Vulkan implementation
 */

#if defined(GLIST_VULKAN)

#include "gTextureVk.h"
#include "gVboVk.h"
#include "gVulkanRenderEngine.h"
#include <algorithm>
#include <utility>
#include <cstring>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include "gShader.h"
#include "gTracy.h"
#include "gFile.h"


uint32_t gTextureVk::nextid = 1;


gTextureVk::gTextureVk() {
	engine = static_cast<gVulkanRenderEngine*>(renderer);
	image = VK_NULL_HANDLE;
	imageallocation = VK_NULL_HANDLE;
	imageview = VK_NULL_HANDLE;
	sampler = VK_NULL_HANDLE;
	activeslot = 0;
	masktexture = nullptr;
	ismaskloaded = false;
	quad = nullptr;
	isfbo = false;
	isloaded = false;
	issubpart = false;
	setupRenderData();
}

gTextureVk::gTextureVk(int w, int h, int format, bool isFbo) {
	engine = static_cast<gVulkanRenderEngine*>(renderer);
	image = VK_NULL_HANDLE;
	imageallocation = VK_NULL_HANDLE;
	imageview = VK_NULL_HANDLE;
	sampler = VK_NULL_HANDLE;
	activeslot = 0;
	masktexture = nullptr;
	ismaskloaded = false;
	quad = nullptr;
	isloaded = false;
	issubpart = false;

	internalformat = format;
	this->format = format;
	wraps = TEXTUREWRAP_REPEAT;
	wrapt = TEXTUREWRAP_REPEAT;
	filtermin = TEXTUREMINMAGFILTER_LINEAR;
	filtermag = TEXTUREMINMAGFILTER_LINEAR;
	width = w;
	height = h;
	isfbo = isFbo;
	if (format == GL_RED) componentnum = 1;
	else if (format == GL_GREEN) componentnum = 1;
	else if (format == GL_BLUE) componentnum = 1;
	else if (format == GL_RG) componentnum = 2;
	else if (format == GL_RGB) componentnum = 3;
	else if (format == GL_RGBA) componentnum = 4;

	id = nextid++;
	istextureallocated = true;
	uploadToGPU(format, GL_UNSIGNED_BYTE, nullptr);
	setupRenderData();
}

gTextureVk::gTextureVk(const gTextureVk& other) : gTextureVk() {
#ifdef DEBUG
	gLogw("gTexture") << "Copy constructor is called, this usually means lower performance! Consider modifying your code.";
#endif
	copyFrom(other);
}

gTextureVk::gTextureVk(gTextureVk&& other) noexcept : gTextureVk() {
	swap(other);
}

gTextureVk& gTextureVk::operator=(const gTextureVk& other) {
	if (this != &other) {
#ifdef DEBUG
		gLogw("gTexture") << "Copy operator is called, this usually means lower performance! Consider modifying your code.";
#endif
		cleanupAll();
		// Reinitialize
		image = VK_NULL_HANDLE;
		imageallocation = VK_NULL_HANDLE;
		imageview = VK_NULL_HANDLE;
		sampler = VK_NULL_HANDLE;
		activeslot = 0;
		id = GL_NONE;
		internalformat = GL_RGBA;
		format = GL_RGBA;
		wraps = TEXTUREWRAP_REPEAT;
		wrapt = TEXTUREWRAP_REPEAT;
		filtermin = TEXTUREMINMAGFILTER_MIPMAPLINEAR;
		filtermag = TEXTUREMINMAGFILTER_LINEAR;
		type = TEXTURETYPE_DIFFUSE;
		path = "";
		width = 0;
		height = 0;
		ismutable = false;
		isstbimage = false;
		isfbo = false;
		ishdr = false;
		ismaskloaded = false;
		isloaded = false;
		masktexture = nullptr;
		componentnum = 0;
		istextureallocated = false;
		data = nullptr;
		datahdr = nullptr;
		quad = nullptr;
		setupRenderData();
		copyFrom(other);
	}
	return *this;
}

gTextureVk& gTextureVk::operator=(gTextureVk&& other) noexcept {
	swap(other);
	return *this;
}

gTextureVk::~gTextureVk() {
	cleanupAll();
}

void gTextureVk::copyFrom(const gTextureVk& other) noexcept {
	internalformat = other.internalformat;
	format = other.format;
	wraps = other.wraps;
	wrapt = other.wrapt;
	filtermin = other.filtermin;
	filtermag = other.filtermag;
	type = other.type;
	path = other.path;
	fullpath = other.fullpath;
	directory = other.directory;
	width = other.width;
	height = other.height;
	componentnum = other.componentnum;
	isfbo = other.isfbo;
	ishdr = other.ishdr;
	issubpart = other.issubpart;
	subpos = other.subpos;
	subscale = other.subscale;

	if (other.width > 0 && other.height > 0 && other.componentnum > 0) {
		int datasize = other.width * other.height * other.componentnum;
		if (other.data) {
			data = new unsigned char[datasize];
			std::copy(other.data, other.data + datasize, data);
			ismutable = true;
			setDataInternal(data, true, false);
		} else if (other.datahdr) {
			datahdr = new float[datasize];
			std::copy(other.datahdr, other.datahdr + datasize, datahdr);
			ismutable = true;
			setDataInternalHDR(datahdr, true, false);
		}
		// Vulkan: no GPU readback for VRAM-only textures
	}

	if (other.masktexture) {
		masktexture = new gTextureVk(*other.masktexture);
		ismaskloaded = true;
	}
}

void gTextureVk::swap(gTextureVk& other) noexcept {
	std::swap(image, other.image);
	std::swap(imageallocation, other.imageallocation);
	std::swap(imageview, other.imageview);
	std::swap(sampler, other.sampler);
	std::swap(activeslot, other.activeslot);
	std::swap(id, other.id);
	std::swap(internalformat, other.internalformat);
	std::swap(format, other.format);
	std::swap(wraps, other.wraps);
	std::swap(wrapt, other.wrapt);
	std::swap(filtermin, other.filtermin);
	std::swap(filtermag, other.filtermag);
	std::swap(type, other.type);
	std::swap(path, other.path);
	std::swap(fullpath, other.fullpath);
	std::swap(directory, other.directory);
	std::swap(width, other.width);
	std::swap(height, other.height);
	std::swap(componentnum, other.componentnum);
	std::swap(data, other.data);
	std::swap(datahdr, other.datahdr);
	std::swap(ismutable, other.ismutable);
	std::swap(isstbimage, other.isstbimage);
	std::swap(isfbo, other.isfbo);
	std::swap(ishdr, other.ishdr);
	std::swap(ismaskloaded, other.ismaskloaded);
	std::swap(masktexture, other.masktexture);
	std::swap(istextureallocated, other.istextureallocated);
	std::swap(quad, other.quad);
	std::swap(imagematrix, other.imagematrix);
	std::swap(isloaded, other.isloaded);
	std::swap(issubpart, other.issubpart);
	std::swap(subpos, other.subpos);
	std::swap(subscale, other.subscale);
}

unsigned int gTextureVk::load(const std::string& fullPath) {
	cleanupData();

	this->fullpath = fullPath;
	directory = getDirName(this->fullpath);
	path = getFileName(this->fullpath);

	gFile f;
	if(!f.load(this->fullpath, gFile::FILEMODE_READONLY, true)) {
		gLoge("gTexture") << "Texture failed to open (gFile) at path: " << this->fullpath;
		return id;
	}

	auto bytes = f.getBytes();
	if(bytes.empty()) {
		gLoge("gTexture") << "Texture file is empty at path: " << this->fullpath;
		return id;
	}

	const stbi_uc* mem = reinterpret_cast<const stbi_uc*>(bytes.data());
	int len = (int)bytes.size();

	ishdr = stbi_is_hdr_from_memory(mem, len);

	if (ishdr) {
		stbi_set_flip_vertically_on_load(true);
		float* img = stbi_loadf_from_memory(mem, len, &width, &height, &componentnum, 0);
		if(!img) {
			gLoge("gTexture") << "stbi_loadf_from_memory failed for: " << this->fullpath
							  << " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
			return id;
		}
		setDataHDR(img, width, height, componentnum, false, true);
	} else {
		unsigned char* img = stbi_load_from_memory(mem, len, &width, &height, &componentnum, 0);
		if(!img) {
			gLoge("gTexture") << "stbi_load_from_memory failed for: " << this->fullpath
							  << " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
			return id;
		}
		setData(img, width, height, componentnum, false, true);
	}

	return id;
}

unsigned int gTextureVk::loadTexture(const std::string& texturePath) {
	return load(gGetTexturesDir() + texturePath);
}

unsigned int gTextureVk::loadMask(const std::string& fullPath) {
	masktexture = new gTextureVk();
	ismaskloaded = true;
	return masktexture->load(fullPath);
}

unsigned int gTextureVk::loadMaskTexture(const std::string& maskTexturePath) {
	masktexture = new gTextureVk();
	ismaskloaded = true;
	return masktexture->load(gGetTexturesDir() + maskTexturePath);
}

void gTextureVk::allocateTexture() {
	if (!istextureallocated) {
		id = nextid++;
		istextureallocated = true;
	}
}

unsigned int gTextureVk::setData(unsigned char* textureData, int width, int height, int componentNum, bool isMutable, bool isStbImage) {
	this->width = width;
	this->height = height;
	this->componentnum = componentNum;

	allocateTexture();
	setDataInternal(textureData, isMutable, isStbImage);

	return id;
}

unsigned int gTextureVk::setDataHDR(float* textureData, int width, int height, int componentNum, bool isMutable, bool isStbImage) {
	this->width = width;
	this->height = height;
	this->componentnum = componentNum;

	allocateTexture();
	setDataInternalHDR(textureData, isMutable, isStbImage);

	return id;
}

void gTextureVk::setDataInternal(unsigned char* textureData, bool isMutable, bool isStbImage, bool clean) {
	if(clean) {
		cleanupData();
	}

	ismutable = isMutable;
	isstbimage = isStbImage;
	ishdr = false;
	data = textureData;
	if (componentnum == 1) format = GL_RED;
	else if (componentnum == 2) format = GL_RG;
	else if (componentnum == 3) format = GL_RGB;
	else if (componentnum == 4) format = GL_RGBA;

	if (data) {
		uploadToGPU(format, GL_UNSIGNED_BYTE, data);

		if (isstbimage && !ismutable) {
			stbi_image_free(data);
			data = nullptr;
		}
	} else {
		gLoge("gTexture") << "Texture failed to load at path: " << fullpath
				<< " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
		return;
	}

	setupRenderData();
}

void gTextureVk::setDataInternalHDR(float* textureData, bool isMutable, bool isStbImage, bool clean) {
	if(clean) {
		cleanupData();
	}

	ismutable = isMutable;
	isstbimage = isStbImage;
	ishdr = true;
	datahdr = textureData;

	if (datahdr) {
		uploadToGPU(GL_RGB, GL_FLOAT, datahdr);

		if (isstbimage && !ismutable) {
			stbi_image_free(datahdr);
			datahdr = nullptr;
		}
	} else {
		gLoge("gTexture") << "Texture failed to load at path: " << fullpath
						<< " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
		return;
	}

	setupRenderData();
}

void gTextureVk::uploadToGPU(int format, int type, void* data) {
	// Destroy any previous image
	destroyImage();

	if (width == 0 || height == 0) {
		return;
	}

	// Determine Vulkan format and bytes per pixel
	VkFormat vkformat = VK_FORMAT_R8G8B8A8_UNORM;
	int bpp = 4;
	bool needsRGBexpand = false;

	if (type == GL_FLOAT) {
		if (format == GL_RGB) {
			vkformat = VK_FORMAT_R32G32B32A32_SFLOAT;
			bpp = 16;
			needsRGBexpand = true;
		} else {
			vkformat = VK_FORMAT_R32G32B32A32_SFLOAT;
			bpp = 16;
		}
	} else {
		if (format == GL_RED) {
			vkformat = VK_FORMAT_R8_UNORM;
			bpp = 1;
		} else if (format == GL_RG) {
			vkformat = VK_FORMAT_R8G8_UNORM;
			bpp = 2;
		} else if (format == GL_RGB) {
			vkformat = VK_FORMAT_R8G8B8A8_UNORM;
			bpp = 4;
			needsRGBexpand = true;
		} else {
			vkformat = VK_FORMAT_R8G8B8A8_UNORM;
			bpp = 4;
		}
	}

	VkDeviceSize imagesize = static_cast<VkDeviceSize>(width) * height * bpp;

	// Create staging buffer
	VkBufferCreateInfo stagingbufinfo{};
	stagingbufinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingbufinfo.size = imagesize;
	stagingbufinfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingbufinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo stagingallocinfo{};
	stagingallocinfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	stagingallocinfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer stagingbuffer;
	VmaAllocation stagingallocation;
	VmaAllocationInfo stagingallocresult;
	vmaCreateBuffer(engine->getVmaAllocator(), &stagingbufinfo, &stagingallocinfo,
		&stagingbuffer, &stagingallocation, &stagingallocresult);

	// Copy data to staging buffer
	if (data && stagingallocresult.pMappedData) {
		if (needsRGBexpand && type != GL_FLOAT) {
			unsigned char* src = static_cast<unsigned char*>(data);
			unsigned char* dst = static_cast<unsigned char*>(stagingallocresult.pMappedData);
			for (int i = 0; i < width * height; i++) {
				dst[i * 4 + 0] = src[i * 3 + 0];
				dst[i * 4 + 1] = src[i * 3 + 1];
				dst[i * 4 + 2] = src[i * 3 + 2];
				dst[i * 4 + 3] = 255;
			}
		} else if (needsRGBexpand && type == GL_FLOAT) {
			float* src = static_cast<float*>(data);
			float* dst = static_cast<float*>(stagingallocresult.pMappedData);
			for (int i = 0; i < width * height; i++) {
				dst[i * 4 + 0] = src[i * 3 + 0];
				dst[i * 4 + 1] = src[i * 3 + 1];
				dst[i * 4 + 2] = src[i * 3 + 2];
				dst[i * 4 + 3] = 1.0f;
			}
		} else {
			memcpy(stagingallocresult.pMappedData, data, imagesize);
		}
	}

	// Create the VkImage
	VkImageCreateInfo imginfo{};
	imginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imginfo.imageType = VK_IMAGE_TYPE_2D;
	imginfo.format = vkformat;
	imginfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
	imginfo.mipLevels = 1;
	imginfo.arrayLayers = 1;
	imginfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imginfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imginfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imginfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo imgallocinfo{};
	imgallocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(engine->getVmaAllocator(), &imginfo, &imgallocinfo,
		&image, &imageallocation, nullptr);

	// Record a one-time command buffer to transition layout and copy data
	VkCommandBufferAllocateInfo cmdallocinfo{};
	cmdallocinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdallocinfo.commandPool = engine->getCommandPool();
	cmdallocinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdallocinfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(engine->getVkDevice(), &cmdallocinfo, &cmd);

	VkCommandBufferBeginInfo begininfo{};
	begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &begininfo);

	// Transition UNDEFINED -> TRANSFER_DST_OPTIMAL
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	// Copy staging buffer to image (only if we have data)
	if (data) {
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

		vkCmdCopyBufferToImage(cmd, stagingbuffer, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	// Transition TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(cmd);

	// Submit and wait
	VkSubmitInfo submitinfo{};
	submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitinfo.commandBufferCount = 1;
	submitinfo.pCommandBuffers = &cmd;

	vkQueueSubmit(engine->getGraphicsQueue(), 1, &submitinfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(engine->getGraphicsQueue());

	vkFreeCommandBuffers(engine->getVkDevice(), engine->getCommandPool(), 1, &cmd);

	// Free staging buffer
	vmaDestroyBuffer(engine->getVmaAllocator(), stagingbuffer, stagingallocation);

	// Create image view
	VkImageViewCreateInfo viewinfo{};
	viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewinfo.image = image;
	viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewinfo.format = vkformat;
	viewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewinfo.subresourceRange.baseMipLevel = 0;
	viewinfo.subresourceRange.levelCount = 1;
	viewinfo.subresourceRange.baseArrayLayer = 0;
	viewinfo.subresourceRange.layerCount = 1;

	vkCreateImageView(engine->getVkDevice(), &viewinfo, nullptr, &imageview);

	// Create sampler (if not already created)
	if (sampler == VK_NULL_HANDLE) {
		VkSamplerCreateInfo samplerinfo{};
		samplerinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerinfo.magFilter = VK_FILTER_LINEAR;
		samplerinfo.minFilter = VK_FILTER_LINEAR;
		samplerinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerinfo.anisotropyEnable = VK_FALSE;
		samplerinfo.maxAnisotropy = 1.0f;
		samplerinfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerinfo.unnormalizedCoordinates = VK_FALSE;
		samplerinfo.compareEnable = VK_FALSE;
		samplerinfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		vkCreateSampler(engine->getVkDevice(), &samplerinfo, nullptr, &sampler);
	}
}

void gTextureVk::bind() const {
	G_PROFILE_ZONE_SCOPED_N("gTexture::bind()");
	engine->bindTexture(activeslot, const_cast<gTextureVk*>(this));
}

void gTextureVk::bind(int textureSlotNo) const {
	G_PROFILE_ZONE_SCOPED_N("gTexture::bind(int)");
	const_cast<gTextureVk*>(this)->activeslot = textureSlotNo;
	engine->bindTexture(textureSlotNo, const_cast<gTextureVk*>(this));
}

void gTextureVk::unbind() const {
	G_PROFILE_ZONE_SCOPED_N("gTexture::unbind()");
	engine->unbindTexture(activeslot);
}

void gTextureVk::setWrapping(int wrapS, int wrapT) {
	wraps = wrapS;
	wrapt = wrapT;
	// Sampler parameters are set at creation time in Vulkan
}

void gTextureVk::setFiltering(int minFilter, int magFilter) {
	filtermin = minFilter;
	filtermag = magFilter;
	// Sampler parameters are set at creation time in Vulkan
}

void gTextureVk::draw(int x, int y) {
	draw(x, y, width, height);
}

void gTextureVk::draw(int x, int y, int w, int h) {
	issubpart = false;
	beginDraw();
	imagematrix = glm::translate(imagematrix, glm::vec3(x, y, 0.0f));
	imagematrix = glm::scale(imagematrix, glm::vec3(w, h, 1.0f));
	endDraw();
}

void gTextureVk::draw(int x, int y, int w, int h, float rotate) {
	draw(glm::vec2(x, y), glm::vec2(w, h), rotate);
}

void gTextureVk::draw(int x, int y, int w, int h, int pivotx, int pivoty, float rotate) {
	draw(glm::vec2(x, y), glm::vec2(w, h), glm::vec2(pivotx, pivoty), rotate);
}

void gTextureVk::draw(glm::vec2 position, glm::vec2 size, float rotate) {
	draw(position, size, glm::vec2(0.5f * size.x, 0.5f * size.y), rotate);
}

void gTextureVk::draw(glm::vec2 position, glm::vec2 size, glm::vec2 pivot, float rotate) {
	G_PROFILE_ZONE_SCOPED_N("gTexture::draw()");
	issubpart = false;
	beginDraw();
	imagematrix = glm::translate(imagematrix, glm::vec3(position, 0.0f));

	imagematrix = glm::translate(imagematrix, glm::vec3(pivot.x, pivot.y, 0.0f));
	imagematrix = glm::rotate(imagematrix, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f));
	imagematrix = glm::translate(imagematrix, glm::vec3(-pivot.x, -pivot.y, 0.0f));

	imagematrix = glm::scale(imagematrix, glm::vec3(size.x, size.y, 1.0f));
	endDraw();
}

void gTextureVk::drawSub(int x, int y, int sx, int sy, int sw, int sh) {
	drawSub(x, y, sw, sh, sx, sy, sw, sh);
}

void gTextureVk::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh) {
	drawSub(x, y, w, h, sx, sy, sw, sh, 0.0f);
}

void gTextureVk::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, float rotate) {
	drawSub(glm::vec2(x, y), glm::vec2(w, h), glm::vec2(sx, sy), glm::vec2(sw, sh), rotate);
}

void gTextureVk::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, int pivotx, int pivoty, float rotate) {
	drawSub(glm::vec2(x, y), glm::vec2(w, h), glm::vec2(sx, sy), glm::vec2(sw, sh), glm::vec2(pivotx, pivoty), rotate);
}

void gTextureVk::drawSub(const gRect& src, const gRect& dst, float rotate) {
	drawSub(dst.left(), dst.top(), dst.getWidth(), dst.getHeight(), src.left(), src.top(), src.getWidth(), src.getHeight(), rotate);
}

void gTextureVk::drawSub(const gRect& src, const gRect& dst, int pivotx, int pivoty, float rotate) {
	drawSub(dst.left(), dst.top(), dst.getWidth(), dst.getHeight(), src.left(), src.top(), src.getWidth(), src.getHeight(), pivotx, pivoty, rotate);
}

void gTextureVk::drawSub(const gRect& src, const gRect& dst, glm::vec2 pivot, float rotate) {
	drawSub(dst.left(), dst.top(), dst.getWidth(), dst.getHeight(), src.left(), src.top(), src.getWidth(), src.getHeight(), pivot.x, pivot.y, rotate);
}

void gTextureVk::drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subPos, glm::vec2 subSize, float rotate) {
	drawSub(pos, size, subPos, subSize, glm::vec2(size.x / 2.0f, size.y / 2.0f), rotate);
}

void gTextureVk::drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subPos, glm::vec2 subSize, glm::vec2 pivot, float rotate) {
	issubpart = true;
	glm::vec2 texturesize = glm::vec2(getWidth(), getHeight());
	subscale = texturesize / subSize;
	subpos = subscale * (subPos / texturesize);

	beginDraw();
	imagematrix = glm::translate(imagematrix, glm::vec3(pos, 0.0f));

	imagematrix = glm::translate(imagematrix, glm::vec3(pivot.x, pivot.y, 0.0f));
	imagematrix = glm::rotate(imagematrix, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f));
	imagematrix = glm::translate(imagematrix, glm::vec3(-pivot.x, -pivot.y, 0.0f));

	imagematrix = glm::scale(imagematrix, glm::vec3(size.x, size.y, 1.0f));
	endDraw();
}

void gTextureVk::beginDraw() {
	G_PROFILE_ZONE_SCOPED_N("gTexture::beginDraw()");
	renderer->getImageShader()->use();
	imagematrix = glm::mat4(1.0f);
	renderer->setProjectionMatrix2d(glm::ortho(0.0f, (float)renderer->getWidth(), (float)renderer->getHeight(), 0.0f, -1.0f, 1.0f));
}

void gTextureVk::endDraw() {
	G_PROFILE_ZONE_SCOPED_N("gTexture::endDraw()");
	renderer->getImageShader()->setMat4("projection", renderer->getProjectionMatrix2d());
	renderer->getImageShader()->setMat4("model", imagematrix);
	renderer->getImageShader()->setVec4("spriteColor", glm::vec4(renderer->getColor()->r, renderer->getColor()->g, renderer->getColor()->b, renderer->getColor()->a));
	renderer->getImageShader()->setInt("image", 0);
	renderer->getImageShader()->setInt("maskimage", 1);
	renderer->getImageShader()->setBool("isAlphaMasking", ismaskloaded);
	renderer->getImageShader()->setBool("isSubPart", issubpart);
	if (issubpart) {
		renderer->getImageShader()->setVec2("subPos", subpos);
		renderer->getImageShader()->setVec2("subScale", subscale);
	}

	bind();
	renderer->getImageShader()->setBool("isAlphaMasking", ismaskloaded);
	if(ismaskloaded) {
		renderer->getImageShader()->setInt("maskimage", 1);
		masktexture->bind(1);
	}
	bool alphablending = renderer->isAlphaBlendingEnabled();
	bool needsalphablending = format == GL_RGBA || format == GL_RG || ismaskloaded;
	if (needsalphablending && !alphablending) {
		renderer->enableAlphaBlending();
	}

	quad->bind();
	renderer->drawFullscreenQuad();
	quad->unbind();

	if (needsalphablending && !alphablending) {
		renderer->disableAlphaBlending();
	}
	unbind();
}

void gTextureVk::setupRenderData() {
	setupRenderData(0, 0, width, height);
}

void gTextureVk::setupRenderData(int sx, int sy, int sw, int sh) {
	G_PROFILE_ZONE_SCOPED_N("gTexture::setupRenderData()");
	if(!isloaded) {
		quad = new gVboVk();
		isloaded = true;
	}
	if (isfbo || ishdr) {
		float vertices[] = {
				0.0f, 1.0f, (float)sx / width, (float)(sy) / height,
				1.0f, 0.0f, (float)(sx + sw) / width, (float)(sy + sh) / height,
				0.0f, 0.0f, (float)sx / width, (float)(sy + sh) / height,

				0.0f, 1.0f, (float)sx / width, (float)(sy) / height,
				1.0f, 1.0f, (float)(sx + sw) / width, (float)(sy) / height,
				1.0f, 0.0f, (float)(sx + sw) / width, (float)(sy + sh) / height
		};
		quad->setVertexData(vertices, 4, 6, GL_STATIC_DRAW);
	} else {
		float vertices[] = {
				0.0f, 1.0f, (float)sx / width, (float)(sy + sh) / height,
				1.0f, 0.0f, (float)(sx + sw) / width, (float)sy / height,
				0.0f, 0.0f, (float)sx / width, (float)sy / height,

				0.0f, 1.0f, (float)sx / width, (float)(sy + sh) / height,
				1.0f, 1.0f, (float)(sx + sw) / width, (float)(sy + sh) / height,
				1.0f, 0.0f, (float)(sx + sw) / width, (float)sy / height
		};
		quad->setVertexData(vertices, 4, 6, GL_STATIC_DRAW);
	}
}

void gTextureVk::destroyImage() {
	if (imageview != VK_NULL_HANDLE || image != VK_NULL_HANDLE) {
		gVulkanRenderEngine::DeferredDeletion del;
		del.imageview = imageview;
		del.image = image;
		del.allocation = imageallocation;
		engine->deferDeletion(del);
		imageview = VK_NULL_HANDLE;
		image = VK_NULL_HANDLE;
		imageallocation = VK_NULL_HANDLE;
	}
}

void gTextureVk::destroyAll() {
	gVulkanRenderEngine::DeferredDeletion del;
	del.imageview = imageview;
	del.image = image;
	del.allocation = imageallocation;
	del.sampler = sampler;
	if (del.imageview != VK_NULL_HANDLE || del.image != VK_NULL_HANDLE || del.sampler != VK_NULL_HANDLE) {
		engine->deferDeletion(del);
	}
	imageview = VK_NULL_HANDLE;
	image = VK_NULL_HANDLE;
	imageallocation = VK_NULL_HANDLE;
	sampler = VK_NULL_HANDLE;
}

void gTextureVk::cleanupAll() {
	G_PROFILE_ZONE_SCOPED_N("gTexture::cleanupAll()");
	if(isloaded) {
		delete quad;
		quad = nullptr;
		isloaded = false;
	}
	if(istextureallocated) {
		destroyAll();
		id = 0;
		istextureallocated = false;
	}
	cleanupData();
	delete masktexture;
	masktexture = nullptr;
}

void gTextureVk::save(std::string fullpath) {
	// Vulkan GPU readback not yet implemented
	gLogw("gTexture") << "save() is not yet supported in the Vulkan backend";
}

void gTextureVk::saveTexture(std::string fileName) {
	save(gGetTexturesDir() + fileName);
}

#endif // GLIST_VULKAN
