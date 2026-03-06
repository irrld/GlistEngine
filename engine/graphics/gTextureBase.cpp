/*
 * gTextureBase.cpp
 *
 *  Created on: May 10, 2020
 *      Author: noyan
 *
 *  Refactored: Base/GL/Vk split
 */

#include "gTextureBase.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#ifdef _WIN32
#define STBI_WINDOWS_UTF8
#endif

#include "stb/stb_image.h"
#include "gTracy.h"


#if defined(GLIST_OPENGLES)
const int gTextureBase::texturewrap[4] = {GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};
const int gTextureBase::texturefilter[4] = {GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST, GL_NEAREST};
#else
const int gTextureBase::texturewrap[4] = {GL_REPEAT, GL_CLAMP, GL_CLAMP_TO_EDGE, GL_NEAREST};
const int gTextureBase::texturefilter[4] = {GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST, GL_CLAMP};
#endif


gTextureBase::gTextureBase() {
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
	componentnum = 0;
	data = nullptr;
	datahdr = nullptr;
	ismutable = false;
	isstbimage = false;
	ishdr = false;
	istextureallocated = false;
}

gTextureBase::~gTextureBase() {
}

void gTextureBase::cleanupData() {
	G_PROFILE_ZONE_SCOPED_N("gTextureBase::cleanupData()");
	if(isstbimage) {
		if (datahdr) {
			stbi_image_free(datahdr);
			datahdr = nullptr;
		} else if (data) {
			stbi_image_free(data);
			data = nullptr;
		}
	} else if(ismutable) {
		delete[] data;
		data = nullptr;
		delete[] datahdr;
		datahdr = nullptr;
	}
}

bool gTextureBase::isMutable() {
	return ismutable;
}

unsigned int gTextureBase::getId() const {
	return id;
}

bool gTextureBase::isHDR() const {
	return ishdr;
}

unsigned int gTextureBase::getInternalFormat() const {
	return internalformat;
}

unsigned int gTextureBase::getFormat() const {
	return format;
}

void gTextureBase::setType(TextureType textureType) {
	type = textureType;
}

gTextureBase::TextureType gTextureBase::getType() const {
	return type;
}

int gTextureBase::getWrapS() const {
	return wraps;
}

int gTextureBase::getWrapT() const {
	return wrapt;
}

int gTextureBase::getFilterMin() const {
	return filtermin;
}

int gTextureBase::getFilterMag() const {
	return filtermag;
}

const std::string& gTextureBase::getTypeName() const {
	return getTypeName(type);
}

const std::string& gTextureBase::getFilename() const {
	return path;
}

const std::string& gTextureBase::getDir() const {
	return directory;
}

const std::string& gTextureBase::getFullPath() const {
	return fullpath;
}

int gTextureBase::getWidth() const {
	return width;
}

int gTextureBase::getHeight() const {
	return height;
}

int gTextureBase::getComponentNum() const {
	return componentnum;
}

unsigned char* gTextureBase::getData() {
	return data;
}

std::string gTextureBase::getDirName(const std::string& fname) {
	size_t pos = fname.find_last_of("\\/");
	return (std::string::npos == pos)
				   ? ""
				   : fname.substr(0, pos);
}

std::string gTextureBase::getFileName(const std::string& fname) {
	size_t pos = fname.find_last_of("\\/");
	return (std::string::npos == pos)
				   ? ""
				   : fname.substr(pos + 1, fname.size());
}

const std::string& gTextureBase::getTypeName(TextureType textureType) {
	static const std::string diffuse = "texture_diffuse";
	static const std::string specular = "texture_specular";
	static const std::string normal = "texture_normal";
	static const std::string height = "texture_height";
	static const std::string albedo = "texture_albedo";
	static const std::string roughness = "texture_roughness";
	static const std::string metalness = "texture_metalness";
	static const std::string ao = "texture_ao";
	static const std::string unknown = "texture_unknown";
	switch (textureType) {
	case TEXTURETYPE_DIFFUSE:
		return diffuse;
	case TEXTURETYPE_SPECULAR:
		return specular;
	case TEXTURETYPE_NORMAL:
	case TEXTURETYPE_PBR_NORMAL:
		return normal;
	case TEXTURETYPE_HEIGHT:
		return height;
	case TEXTURETYPE_PBR_ALBEDO:
		return albedo;
	case TEXTURETYPE_PBR_ROUGHNESS:
		return roughness;
	case TEXTURETYPE_PBR_METALNESS:
		return metalness;
	case TEXTURETYPE_PBR_AO:
		return ao;
	default:
		return unknown;
	}
}
