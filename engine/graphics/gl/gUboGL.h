/*
 * gUboGL.h
 *
 *  Created on: 21.12.2023
 *      Author: irrel
 *
 *  Refactored: OpenGL implementation
 */

#pragma once

#include "gUboBase.h"

template<typename T>
class gUboGL : public gUboBase<T> {
public:
	template<typename ...Args>
	gUboGL(Args&&... args, int bindingpoint, int usage = GL_DYNAMIC_DRAW) : bufferid(0) {
		this->size = sizeof(T);
		this->bindingpoint = bindingpoint;
		this->data = new T(std::forward<Args>(args)...);
		G_CHECK_GL(glGenBuffers(1, &bufferid));
		G_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, bufferid));
		G_CHECK_GL(glBufferData(GL_UNIFORM_BUFFER, this->size, nullptr, usage));
		G_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
		G_CHECK_GL(glBindBufferRange(GL_UNIFORM_BUFFER, bindingpoint, bufferid, 0, this->size));
		update();
	}

	~gUboGL() {
		if (bufferid != 0) {
			G_CHECK_GL(glDeleteBuffers(1, &bufferid));
			bufferid = 0;
		}
	}

	void update() {
		update(0, this->size);
	}

	void update(int offset, int length) {
		void* ptr = reinterpret_cast<char*>(this->data) + offset;
		G_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, bufferid));
		G_CHECK_GL(glBufferSubData(GL_UNIFORM_BUFFER, offset, length, ptr));
		G_CHECK_GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	}

	void setData(T* newObject) {
		this->data = newObject;
		update();
	}

private:
	GLuint bufferid;
};
