/*
 * gSkyboxGL.h
 *
 *  Created on: 18 Mar 2021
 *      Author: Acer
 *
 *  Refactored: OpenGL implementation
 */

#ifndef GRAPHICS_GL_GSKYBOXGL_H_
#define GRAPHICS_GL_GSKYBOXGL_H_

#include "gSkyboxBase.h"
#include "gSkyTextureGL.h"

class gImage;


class gSkyboxGL : public gSkyboxBase {
public:
	gSkyboxGL();
	~gSkyboxGL();

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
	gSkyTextureGL skyhandle;

	GLuint captureFBO;
	GLuint captureRBO;
	gSkyTextureGL* irradianceHandle;
	gSkyTextureGL* prefilterHandle;
	GLuint brdfTexture;

	GLuint cubeVAO, cubeVBO;
	GLuint quadVAO, quadVBO;

	gShader* skyboxshader;
	gShader* pbrShader;
	gShader* equirectangularToCubemapShader;
	gShader* irradianceShader;
	gShader* prefilterShader;
	gShader* brdfShader;

	void renderCube();
	void renderQuad();
};

#endif /* GRAPHICS_GL_GSKYBOXGL_H_ */
