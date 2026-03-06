/*
 * gTextureGL.cpp
 *
 *  Created on: May 10, 2020
 *      Author: noyan
 *
 *  Refactored: OpenGL implementation
 */

#include "gTextureGL.h"
#include "gVboGL.h"
#include <algorithm>
#include <utility>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include "gShader.h"
#include "gTracy.h"
#include "gFile.h"


gTextureGL::gTextureGL() {
	textureid = 0;
	masktexture = nullptr;
	ismaskloaded = false;
	quad = nullptr;
	isfbo = false;
	isloaded = false;
	issubpart = false;
	setupRenderData();
}

gTextureGL::gTextureGL(int w, int h, int format, bool isFbo) {
	textureid = 0;
	masktexture = nullptr;
	ismaskloaded = false;
	quad = nullptr;
	isloaded = false;
	issubpart = false;

	int valuetype = GL_UNSIGNED_BYTE;
	internalformat = format;
	this->format = format;
	wraps = TEXTUREWRAP_REPEAT;
	wrapt = TEXTUREWRAP_REPEAT;
	filtermin = TEXTUREMINMAGFILTER_LINEAR;
	filtermag = TEXTUREMINMAGFILTER_LINEAR;
#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR || EMSCRIPTEN
	if(format == GL_DEPTH_COMPONENT) {
		internalformat = GL_DEPTH_COMPONENT24;
		valuetype = GL_UNSIGNED_INT;
		wrapt = TEXTUREWRAP_CLAMPTOEDGE;
		wraps = TEXTUREWRAP_CLAMPTOEDGE;
		filtermin = TEXTUREMINMAGFILTER_NEAREST;
		filtermag = TEXTUREMINMAGFILTER_NEAREST;
	} else if(format == GL_DEPTH_STENCIL) {
		internalformat = GL_DEPTH24_STENCIL8;
		valuetype = GL_UNSIGNED_INT_24_8;
	}
#endif
	width = w;
	height = h;
	isfbo = isFbo;
	if (format == GL_RED) componentnum = 1;
	else if (format == GL_GREEN) componentnum = 1;
	else if (format == GL_BLUE) componentnum = 1;
	else if (format == GL_RG) componentnum = 2;
	else if (format == GL_RGB) componentnum = 3;
	else if (format == GL_RGBA) componentnum = 4;

	G_CHECK_GL(glGenTextures(1, &textureid));
	id = textureid;
	istextureallocated = true;
	bind();

	G_CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, valuetype, nullptr));

	// TODO: BEFORE SHADOWMAP GL_REPEAT
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texturefilter[filtermin]));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texturefilter[filtermag]));
	setupRenderData();
}

gTextureGL::gTextureGL(const gTextureGL& other) : gTextureGL() {
#ifdef DEBUG
	gLogw("gTexture") << "Copy constructor is called, this usually means lower performance! Consider modifying your code.";
#endif
	copyFrom(other);
}

gTextureGL::gTextureGL(gTextureGL&& other) noexcept : gTextureGL() {
	swap(other);
}

gTextureGL& gTextureGL::operator=(const gTextureGL& other) {
	if (this != &other) {
#ifdef DEBUG
		gLogw("gTexture") << "Copy operator is called, this usually means lower performance! Consider modifying your code.";
#endif
		cleanupAll();
		// Reinitialize to default state
		textureid = 0;
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

		// Now copy from other
		copyFrom(other);
	}
	return *this;
}

gTextureGL& gTextureGL::operator=(gTextureGL&& other) noexcept {
	swap(other);
	return *this;
}

gTextureGL::~gTextureGL() {
	cleanupAll();
}

void gTextureGL::copyFrom(const gTextureGL& other) noexcept {
	// Copy basic properties
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

	// Deep copy texture data
	if (other.width > 0 && other.height > 0 && other.componentnum > 0) {
		int datasize = other.width * other.height * other.componentnum;

		// Source has RAM data (regular unsigned char)
		if (other.data) {
			data = new unsigned char[datasize];
			std::copy(other.data, other.data + datasize, data);
			ismutable = true;
			setDataInternal(data, true, false);
		}
		// Source has HDR data (float)
		else if (other.datahdr) {
			datahdr = new float[datasize];
			std::copy(other.datahdr, other.datahdr + datasize, datahdr);
			ismutable = true;
			setDataInternalHDR(datahdr, true, false);
		}
		// Source only has VRAM data - read from GPU
		else if (other.istextureallocated && other.id != GL_NONE) {
			GLuint readfbo;
			G_CHECK_GL(glGenFramebuffers(1, &readfbo));
			G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, readfbo));
			G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, other.id, 0));
			if (other.ishdr) {
				datahdr = new float[datasize];
				G_CHECK_GL(glReadPixels(0, 0, other.width, other.height, GL_RGB, GL_FLOAT, datahdr));
				ismutable = true;
				setDataInternalHDR(datahdr, true, false);
			} else {
				data = new unsigned char[datasize];
				G_CHECK_GL(glReadPixels(0, 0, other.width, other.height, other.format, GL_UNSIGNED_BYTE, data));
				ismutable = true;
				setDataInternal(data, true, false);
			}
			G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
			G_CHECK_GL(glDeleteFramebuffers(1, &readfbo));
		}
	}

	// Deep copy mask texture if present
	if (other.masktexture) {
		masktexture = new gTextureGL(*other.masktexture);
		ismaskloaded = true;
	}
}

void gTextureGL::swap(gTextureGL& other) noexcept {
	std::swap(textureid, other.textureid);
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

unsigned int gTextureGL::load(const std::string& fullPath) {
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

unsigned int gTextureGL::loadTexture(const std::string& texturePath) {
	return load(gGetTexturesDir() + texturePath);
}

unsigned int gTextureGL::loadMask(const std::string& fullPath) {
	masktexture = new gTextureGL();
	ismaskloaded = true;
	return masktexture->load(fullPath);
}

unsigned int gTextureGL::loadMaskTexture(const std::string& maskTexturePath) {
	masktexture = new gTextureGL();
	ismaskloaded = true;
	return masktexture->load(gGetTexturesDir() + maskTexturePath);
}

void gTextureGL::allocateTexture() {
	if (!istextureallocated) {
		G_CHECK_GL(glGenTextures(1, &textureid));
		id = textureid;
		istextureallocated = true;
	}
}

unsigned int gTextureGL::setData(unsigned char* textureData, int width, int height, int componentNum, bool isMutable, bool isStbImage) {
	this->width = width;
	this->height = height;
	this->componentnum = componentNum;

	allocateTexture();
	setDataInternal(textureData, isMutable, isStbImage);

	return id;
}

unsigned int gTextureGL::setDataHDR(float* textureData, int width, int height, int componentNum, bool isMutable, bool isStbImage) {
	this->width = width;
	this->height = height;
	this->componentnum = componentNum;

	allocateTexture();
	setDataInternalHDR(textureData, isMutable, isStbImage);

	return id;
}

void gTextureGL::setDataInternal(unsigned char* textureData, bool isMutable, bool isStbImage, bool clean) {
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
		bind();
		G_CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
		G_CHECK_GL(glGenerateMipmap(GL_TEXTURE_2D));
		G_CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, format, getWidth(), getHeight(), 0, format, GL_UNSIGNED_BYTE, data));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texturefilter[filtermin]));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texturefilter[filtermag]));

		if (format == GL_RG) {
#if defined(GLIST_OPENGLES)
			G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED));
			G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED));
			G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED));
			G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_GREEN));
#else
			int swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
			G_CHECK_GL(glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask));
#endif
		}

		if (isstbimage && !ismutable) {
			stbi_image_free(data);
			data = nullptr;
		}
		unbind();
	} else {
		gLoge("gTexture") << "Texture failed to load at path: " << fullpath
				<< " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
		return;
	}

	setupRenderData();
}

void gTextureGL::setDataInternalHDR(float* textureData, bool isMutable, bool isStbImage, bool clean) {
	if(clean) {
		cleanupData();
	}

	ismutable = isMutable;
	isstbimage = isStbImage;
	ishdr = true;
	datahdr = textureData;

	if (datahdr) {
		bind();
		G_CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, datahdr));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texturefilter[filtermin]));
		G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texturefilter[filtermag]));
		if (isstbimage && !ismutable) {
			stbi_image_free(datahdr);
			datahdr = nullptr;
		}
		unbind();
	} else {
		gLoge("gTexture") << "Texture failed to load at path: " << fullpath
						<< " reason: " << (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
		return;
	}

	setupRenderData();
}

void gTextureGL::bind() const {
	G_PROFILE_ZONE_SCOPED_N("gTexture::bind()");
	G_CHECK_GL(glBindTexture(GL_TEXTURE_2D, textureid));
}

void gTextureGL::bind(int textureSlotNo) const {
	G_PROFILE_ZONE_SCOPED_N("gTexture::bind(int)");
	G_CHECK_GL(glActiveTexture(GL_TEXTURE0 + textureSlotNo));
	G_CHECK_GL(glBindTexture(GL_TEXTURE_2D, textureid));
}

void gTextureGL::unbind() const {
	G_PROFILE_ZONE_SCOPED_N("gTexture::unbind()");
	G_CHECK_GL(glBindTexture(GL_TEXTURE_2D, 0));
}

void gTextureGL::setWrapping(int wrapS, int wrapT) {
	wraps = wrapS;
	wrapt = wrapT;
	bind();
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texturewrap[wraps]));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texturewrap[wrapt]));
	unbind();
}

void gTextureGL::setFiltering(int minFilter, int magFilter) {
	filtermin = minFilter;
	filtermag = magFilter;
	bind();
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texturefilter[filtermin]));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texturefilter[filtermag]));
	unbind();
}

void gTextureGL::draw(int x, int y) {
	draw(x, y, width, height);
}

void gTextureGL::draw(int x, int y, int w, int h) {
	issubpart = false;
	beginDraw();
	imagematrix = glm::translate(imagematrix, glm::vec3(x, y, 0.0f));
	imagematrix = glm::scale(imagematrix, glm::vec3(w, h, 1.0f));
	endDraw();
}

void gTextureGL::draw(int x, int y, int w, int h, float rotate) {
	draw(glm::vec2(x, y), glm::vec2(w, h), rotate);
}

void gTextureGL::draw(int x, int y, int w, int h, int pivotx, int pivoty, float rotate) {
	draw(glm::vec2(x, y), glm::vec2(w, h), glm::vec2(pivotx, pivoty), rotate);
}

void gTextureGL::draw(glm::vec2 position, glm::vec2 size, float rotate) {
	draw(position, size, glm::vec2(0.5f * size.x, 0.5f * size.y), rotate);
}

void gTextureGL::draw(glm::vec2 position, glm::vec2 size, glm::vec2 pivot, float rotate) {
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

void gTextureGL::drawSub(int x, int y, int sx, int sy, int sw, int sh) {
	drawSub(x, y, sw, sh, sx, sy, sw, sh);
}

void gTextureGL::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh) {
	drawSub(x, y, w, h, sx, sy, sw, sh, 0.0f);
}

void gTextureGL::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, float rotate) {
	drawSub(glm::vec2(x, y), glm::vec2(w, h), glm::vec2(sx, sy), glm::vec2(sw, sh), rotate);
}

void gTextureGL::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, int pivotx, int pivoty, float rotate) {
	drawSub(glm::vec2(x, y), glm::vec2(w, h), glm::vec2(sx, sy), glm::vec2(sw, sh), glm::vec2(pivotx, pivoty), rotate);
}

void gTextureGL::drawSub(const gRect& src, const gRect& dst, float rotate) {
	drawSub(dst.left(), dst.top(), dst.getWidth(), dst.getHeight(), src.left(), src.top(), src.getWidth(), src.getHeight(), rotate);
}

void gTextureGL::drawSub(const gRect& src, const gRect& dst, int pivotx, int pivoty, float rotate) {
	drawSub(dst.left(), dst.top(), dst.getWidth(), dst.getHeight(), src.left(), src.top(), src.getWidth(), src.getHeight(), pivotx, pivoty, rotate);
}

void gTextureGL::drawSub(const gRect& src, const gRect& dst, glm::vec2 pivot, float rotate) {
	drawSub(dst.left(), dst.top(), dst.getWidth(), dst.getHeight(), src.left(), src.top(), src.getWidth(), src.getHeight(), pivot.x, pivot.y, rotate);
}

void gTextureGL::drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subPos, glm::vec2 subSize, float rotate) {
	drawSub(pos, size, subPos, subSize, glm::vec2(size.x / 2.0f, size.y / 2.0f), rotate);
}

void gTextureGL::drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subPos, glm::vec2 subSize, glm::vec2 pivot, float rotate) {
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

void gTextureGL::beginDraw() {
	G_PROFILE_ZONE_SCOPED_N("gTexture::beginDraw()");
	renderer->getImageShader()->use();
	imagematrix = glm::mat4(1.0f);
	renderer->setProjectionMatrix2d(glm::ortho(0.0f, (float)renderer->getWidth(), (float)renderer->getHeight(), 0.0f, -1.0f, 1.0f));
}

void gTextureGL::endDraw() {
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

	G_CHECK_GL(glActiveTexture(GL_TEXTURE0));

	bind();
	renderer->getImageShader()->setBool("isAlphaMasking", ismaskloaded);
	if(ismaskloaded) {
		renderer->getImageShader()->setInt("maskimage", 1);
		G_CHECK_GL(glActiveTexture(GL_TEXTURE1));
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

void gTextureGL::setupRenderData() {
	setupRenderData(0, 0, width, height);
}

void gTextureGL::setupRenderData(int sx, int sy, int sw, int sh) {
	G_PROFILE_ZONE_SCOPED_N("gTexture::setupRenderData()");
	if(!isloaded) {
		quad = new gVboGL();
		isloaded = true;
	}
	if (isfbo || ishdr) {
		float vertices[] = {
				// pos      // tex
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
				// pos      // tex
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

void gTextureGL::destroyTexture() {
	if (textureid != 0) {
		G_CHECK_GL(glDeleteTextures(1, &textureid));
		textureid = 0;
	}
}

void gTextureGL::cleanupAll() {
	G_PROFILE_ZONE_SCOPED_N("gTexture::cleanupAll()");
	if(isloaded) {
		delete quad;
		quad = nullptr;
		isloaded = false;
	}
	if(istextureallocated) {
		destroyTexture();
		id = 0;
		istextureallocated = false;
	}
	cleanupData();
	delete masktexture;
	masktexture = nullptr;
}

void gTextureGL::save(std::string fullpath) {
	G_PROFILE_ZONE_SCOPED_N("gTexture::save()");
	unsigned char* pixels = new unsigned char[width * height * componentnum];

	GLuint readfbo;
	G_CHECK_GL(glGenFramebuffers(1, &readfbo));
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, readfbo));
	G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0));
	G_CHECK_GL(glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, pixels));
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	G_CHECK_GL(glDeleteFramebuffers(1, &readfbo));

	// flip back vertically if fbo or hdr
	if (isfbo || ishdr) {
		unsigned char* temppix = new unsigned char[width * componentnum];
		int linenum = height / 2;
		for(int i = 0; i < linenum; i++) {
			int afirst = i * width * componentnum;
			int alast = afirst + width * componentnum;
			int bfirst = (height - i - 1) * width * componentnum;
			int blast = bfirst + width * componentnum;
			std::copy(pixels + afirst, pixels + alast, temppix);
			std::copy(pixels + bfirst, pixels + blast, pixels + afirst);
			std::copy(temppix, temppix + (width * componentnum), pixels + bfirst);
		}
		delete[] temppix;
	}

	stbi_write_png(fullpath.c_str(), width, height, componentnum, pixels, width * componentnum * sizeof(unsigned char));
	delete[] pixels;
}

void gTextureGL::saveTexture(std::string fileName) {
	save(gGetTexturesDir() + fileName);
}
