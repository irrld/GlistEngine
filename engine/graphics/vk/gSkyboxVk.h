/*
 * gSkyboxVk.h
 *
 *  Created on: 18 Mar 2021
 *      Author: Acer
 *
 *  Refactored: Vulkan implementation
 */

#ifndef GRAPHICS_VK_GSKYBOXVK_H_
#define GRAPHICS_VK_GSKYBOXVK_H_

#if defined(GLIST_VULKAN)

#include "gSkyboxBase.h"
#include "gSkyTextureVk.h"

class gImage;


class gSkyboxVk : public gSkyboxBase {
public:
	gSkyboxVk();
	~gSkyboxVk();

	unsigned int loadTextures(const std::vector<std::string>& texturePaths);
	unsigned int load(const std::vector<std::string>& fullPaths);
	unsigned int loadEquirectangular(const std::string& fullPath);
	unsigned int loadTextureEquirectangular(const std::string& texturePath);
	void loadFromData(std::array<int, 6> widths, std::array<int, 6> heights, std::array<void*, 6> rawdata, std::array<bool, 6> ishdr);
	void draw();
	void generatePbrMaps();
	void bindPbrMaps();
	void loadSkybox(gImage* images);
	void loadDataSkybox(std::string* data, int width, int height);

private:
	gSkyTextureVk skyhandle;
};

#endif // GLIST_VULKAN
#endif /* GRAPHICS_VK_GSKYBOXVK_H_ */
