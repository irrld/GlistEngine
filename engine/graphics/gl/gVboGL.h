/*
 * gVboGL.h
 *
 *  Created on: 25 Kas 2020
 *      Author: Acer
 *
 *  Refactored: OpenGL implementation
 */

#ifndef GRAPHICS_GL_GVBOGL_H_
#define GRAPHICS_GL_GVBOGL_H_

#include "gVboBase.h"


class gVboGL : public gVboBase {
public:
	gVboGL();
	~gVboGL();

	void setVertexData(const gVertex* vertices, int coordNum, int total);
	void setVertexData(const glm::vec3* vertices, int total, int usage = GL_STATIC_DRAW);
	void setVertexData(const glm::vec2* vertices, int total, int usage = GL_STATIC_DRAW);
	void setVertexData(const float* verticesptr, int coordNum, int total, int usage, int stride = 0);
	void setIndexData(const gIndex* indices, int total);

	void clear();

	void bind() const;
	void unbind() const;

	int getVAOid() const;

private:
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
};

#endif /* GRAPHICS_GL_GVBOGL_H_ */
