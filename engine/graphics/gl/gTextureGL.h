/*
 * gTextureGL.h
 *
 *  Created on: May 10, 2020
 *      Author: noyan
 *
 *  Refactored: OpenGL implementation
 */

#ifndef ENGINE_GRAPHICS_GL_GTEXTUREGL_H_
#define ENGINE_GRAPHICS_GL_GTEXTUREGL_H_

#include "gTextureBase.h"
#include "gRect.h"

class gVboGL;

class gTextureGL : public gTextureBase {
public:
	gTextureGL();
	gTextureGL(int w, int h, int format = GL_RGBA, bool isFbo = false);
	gTextureGL(const gTextureGL& other);
	gTextureGL(gTextureGL&& other) noexcept;
	gTextureGL& operator=(const gTextureGL& other);
	gTextureGL& operator=(gTextureGL&& other) noexcept;
	virtual ~gTextureGL();

	virtual unsigned int load(const std::string& fullPath);
	unsigned int loadTexture(const std::string& texturePath);

	unsigned int loadMask(const std::string& fullPath);
	unsigned int loadMaskTexture(const std::string& maskTexturePath);

	void bind() const;
	void bind(int textureSlotNo) const;
	void unbind() const;

	void setWrapping(int wrapS, int wrapT);
	void setFiltering(int minFilter, int magFilter);

	unsigned int setData(unsigned char* textureData, int width, int height, int componentNum, bool isMutable = false, bool isStbImage = false);
	unsigned int setDataHDR(float* textureData, int width, int height, int componentNum, bool isMutable = false, bool isStbImage = false);

	void draw(int x, int y);
	void draw(int x, int y, int w, int h);
	void draw(int x, int y, int w, int h, float rotate);
	void draw(int x, int y, int w, int h, int pivotx, int pivoty, float rotate);
	void draw(glm::vec2 position, glm::vec2 size, float rotate = 0.0f);
	void draw(glm::vec2 position, glm::vec2 size, glm::vec2 pivot, float rotate = 0.0f);

	void drawSub(int x, int y, int sx, int sy, int sw, int sh);
	void drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh);
	void drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, float rotate);
	void drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, int pivotx, int pivoty, float rotate);
	void drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subpos, glm::vec2 subsize, float rotate = 0.0f);
	void drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subpos, glm::vec2 subsize, glm::vec2 pivot, float rotate = 0.0f);
	void drawSub(const gRect& src, const gRect& dst, float rotate = 0.f);
	void drawSub(const gRect& src, const gRect& dst, int pivotx, int pivoty, float rotate = 0.f);
	void drawSub(const gRect& src, const gRect& dst, glm::vec2 pivot, float rotate = 0.f);

	void setupRenderData();

	void cleanupAll();

	void save(std::string fullpath);
	void saveTexture(std::string fileName);

protected:
	void allocateTexture();
	void setDataInternal(unsigned char* textureData, bool isMutable = false, bool isStbImage = false, bool clean = true);
	void setDataInternalHDR(float* textureData, bool isMutable = false, bool isStbImage = false, bool clean = true);

	void swap(gTextureGL& other) noexcept;
	void copyFrom(const gTextureGL& other) noexcept;

	gTextureGL* masktexture;
	bool ismaskloaded;

private:
	void destroyTexture();
	void setupRenderData(int sx, int sy, int sw, int sh);
	void beginDraw();
	void endDraw();

	GLuint textureid;
	gVboGL* quad;
	glm::mat4 imagematrix;
	bool isfbo;
	bool isloaded;
	bool issubpart;
	glm::vec2 subpos;
	glm::vec2 subscale;
};

#endif /* ENGINE_GRAPHICS_GL_GTEXTUREGL_H_ */
