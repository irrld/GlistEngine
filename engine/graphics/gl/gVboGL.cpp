/*
 * gVboGL.cpp
 *
 *  Created on: 25 Kas 2020
 *      Author: Acer
 *
 *  Refactored: OpenGL implementation
 */

#include "gVboGL.h"
#include "gTracy.h"

gVboGL::gVboGL() : vao(0), vbo(0), ebo(0) {
	G_CHECK_GL(glGenVertexArrays(1, &vao));
}

gVboGL::~gVboGL() {
	clear();
}

void gVboGL::setVertexData(const gVertex* vertices, int coordNum, int total) {
	G_PROFILE_ZONE_SCOPED_N("gVbo::setVertexData()");
	bind();
	isvertexdataallocated = true;
	vertexdatacoordnum = coordNum;
	totalvertexnum = total;

	if (vbo == 0) {
		G_CHECK_GL(glGenBuffers(1, &vbo));
	}
	G_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	G_CHECK_GL(glBufferData(GL_ARRAY_BUFFER, totalvertexnum * sizeof(gVertex), vertices, GL_STATIC_DRAW));

	G_CHECK_GL(glEnableVertexAttribArray(0));
	G_CHECK_GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(gVertex), (void*)0));
	// vertex normals
	G_CHECK_GL(glEnableVertexAttribArray(1));
	G_CHECK_GL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(gVertex), (void*)offsetof(gVertex, normal)));
	// vertex texture coords
	G_CHECK_GL(glEnableVertexAttribArray(2));
	G_CHECK_GL(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(gVertex), (void*)offsetof(gVertex, texcoords)));
	// vertex tangent
	G_CHECK_GL(glEnableVertexAttribArray(3));
	G_CHECK_GL(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(gVertex), (void*)offsetof(gVertex, tangent)));
	// vertex bitangent
	G_CHECK_GL(glEnableVertexAttribArray(4));
	G_CHECK_GL(glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(gVertex), (void*)offsetof(gVertex, bitangent)));
	// vertex color
	G_CHECK_GL(glEnableVertexAttribArray(5));
	G_CHECK_GL(glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(gVertex), (void*)offsetof(gVertex, color)));
	unbind();
}

void gVboGL::setVertexData(const float* verticesptr, int coordNum, int total, int usage, int stride) {
	bind();
	isvertexdataallocated = true;
	vertexdatacoordnum = coordNum;
	totalvertexnum = total;
	vertexusage = usage;
	vertexstride = stride;

	GLsizeiptr size = stride == 0 ? vertexdatacoordnum * sizeof(float) : stride;
	if (vbo == 0) {
		G_CHECK_GL(glGenBuffers(1, &vbo));
	}
	G_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	G_CHECK_GL(glBufferData(GL_ARRAY_BUFFER, totalvertexnum * size, verticesptr, usage));
	G_CHECK_GL(glEnableVertexAttribArray(0));
	G_CHECK_GL(glVertexAttribPointer(0, vertexdatacoordnum, GL_FLOAT, GL_FALSE, vertexdatacoordnum * sizeof(float), nullptr));
	unbind();
}

void gVboGL::setVertexData(const glm::vec3* vertices, int total, int usage) {
	setVertexData(&vertices[0].x, 3, total, usage);
}

void gVboGL::setVertexData(const glm::vec2* vertices, int total, int usage) {
	setVertexData(&vertices[0].x, 2, total, usage);
}

void gVboGL::setIndexData(const gIndex* indices, int total) {
	if (indices == nullptr) {
		if (ebo != 0) {
			G_CHECK_GL(glDeleteBuffers(1, &ebo));
			ebo = 0;
		}
		isindexdataallocated = false;
		return;
	}
	bind();
	isindexdataallocated = true;
	totalindexnum = total;

	if (ebo == 0) {
		G_CHECK_GL(glGenBuffers(1, &ebo));
	}
	G_CHECK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
	G_CHECK_GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalindexnum * sizeof(gIndex), indices, GL_STATIC_DRAW));
	unbind();
}

void gVboGL::clear() {
	if (vbo != 0) {
		G_CHECK_GL(glDeleteBuffers(1, &vbo));
		vbo = 0;
	}
	if (ebo != 0) {
		G_CHECK_GL(glDeleteBuffers(1, &ebo));
		ebo = 0;
	}
	if (vao != 0) {
		G_CHECK_GL(glDeleteVertexArrays(1, &vao));
		vao = 0;
	}
	isvertexdataallocated = false;
	isindexdataallocated = false;
}

void gVboGL::bind() const {
	G_CHECK_GL(glBindVertexArray(vao));
}

void gVboGL::unbind() const {
	G_CHECK_GL(glBindVertexArray(0));
}

int gVboGL::getVAOid() const {
	return vao;
}
