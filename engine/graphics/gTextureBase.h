/*
 * gTextureBase.h
 *
 *  Created on: May 10, 2020
 *      Author: noyan
 *
 *  Refactored: Base/GL/Vk split
 */

#pragma once

#ifndef ENGINE_GRAPHICS_GTEXTUREBASE_H_
#define ENGINE_GRAPHICS_GTEXTUREBASE_H_

#include <string>
#include "gRenderObject.h"


class gTextureBase : public gRenderObject {
public:
	enum TextureType : int {
		TEXTURETYPE_DIFFUSE = 0,
		TEXTURETYPE_SPECULAR = 1,
		TEXTURETYPE_NORMAL = 2,
		TEXTURETYPE_HEIGHT = 3,
		TEXTURETYPE_PBR_ALBEDO = 4,
		TEXTURETYPE_PBR_ROUGHNESS = 5,
		TEXTURETYPE_PBR_METALNESS = 6,
		TEXTURETYPE_PBR_NORMAL = 7,
		TEXTURETYPE_PBR_AO = 8,
		TEXTURETYPE_COUNT
	};

	enum TextureWrap : int {
		TEXTUREWRAP_REPEAT = 0,
		TEXTUREWRAP_CLAMP = 1,
		TEXTUREWRAP_CLAMPTOEDGE = 2,
		TEXTUREWRAP_NEAREST = 3
	};

	enum TextureMagFilter : int {
		TEXTUREMINMAGFILTER_LINEAR = 0,
		TEXTUREMINMAGFILTER_MIPMAPLINEAR = 1,
		TEXTUREMINMAGFILTER_NEAREST = 2,
		TEXTUREMINMAGFILTER_CLAMP = 3
	};

	unsigned int getId() const;
	bool isHDR() const;
	unsigned int getInternalFormat() const;
	unsigned int getFormat() const;
	void setType(TextureType textureType);
	TextureType getType() const;
	int getWrapS() const;
	int getWrapT() const;
	int getFilterMin() const;
	int getFilterMag() const;
	const std::string& getTypeName() const;
	const std::string& getFilename() const;
	const std::string& getDir() const;
	const std::string& getFullPath() const;
	int getWidth() const;
	int getHeight() const;
	int getComponentNum() const;
	unsigned char* getData();

	void cleanupData();

	static const std::string& getTypeName(TextureType textureType);

protected:
	gTextureBase();
	virtual ~gTextureBase();

	bool isMutable();

	std::string getDirName(const std::string& fname);
	std::string getFileName(const std::string& fname);

	unsigned int id, internalformat, format;
	TextureType type;
	std::string path, fullpath, directory;
	int width, height, componentnum;
	unsigned char* data;
	float* datahdr;
	bool ismutable;
	bool isstbimage;
	bool ishdr;
	bool istextureallocated;
	int wraps, wrapt, filtermin, filtermag;

	static const int texturewrap[4];
	static const int texturefilter[4];
};

#endif /* ENGINE_GRAPHICS_GTEXTUREBASE_H_ */
