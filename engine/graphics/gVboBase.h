/*
 * gVboBase.h
 *
 *  Created on: 25 Kas 2020
 *      Author: Acer
 *
 *  Refactored: Base/GL/Vk split
 */

#ifndef GRAPHICS_GVBOBASE_H_
#define GRAPHICS_GVBOBASE_H_

#include "gRenderObject.h"


struct gVertex {
    // position
    glm::vec3 position{0};
    // normal
    glm::vec3 normal{0};
    // texCoords
    glm::vec2 texcoords{0};
    // tangent
    glm::vec3 tangent{0};
    // bitangent
    glm::vec3 bitangent{0};
    // color
    glm::vec3 color{0};
};


class gVboBase : public gRenderObject {
public:
	gVboBase();
	virtual ~gVboBase();

	gVboBase(const gVboBase&) = delete;
	gVboBase& operator=(const gVboBase& other) = delete;

	void enable();
	void disable();
	bool isEnabled() const;
	bool isVertexDataAllocated() const;
	bool isIndexDataAllocated() const;
	int getVerticesNum() const;
	int getIndicesNum() const;

	void setDynamic(bool dynamic);
	bool isDynamic() const;

	// Stub methods (not implemented)
	void setColorData(const gColor* colors, int total, int usage);
	void setColorData(const float* color0r, int total, int usage, int stride = 0);
	void setTexCoordData(const glm::vec2* texCoords, int total, int usage);
	void setTexCoordData(const float* texCoord0x, int total, int usage, int stride = 0);
	void setNormalData(const glm::vec3* normals, int total, int usage = GL_STATIC_DRAW);
	void setNormalData(const float* normal0x, int total, int usage, int stride = 0);
	void setIndexData(const int* indices, int total, int usage);

protected:
    bool isdynamic;
    bool isenabled;

    bool isvertexdataallocated;
    int vertexdatacoordnum, totalvertexnum;
    int vertexusage, vertexstride;

    bool isindexdataallocated;
    int totalindexnum;
};

#endif /* GRAPHICS_GVBOBASE_H_ */
