/*
 * gUboBase.h
 *
 *  Created on: 21.12.2023
 *      Author: irrel
 *
 *  Refactored: Base/GL/Vk split
 */

#pragma once

#include "gRenderObject.h"

template<typename T>
class gUboBase : public gRenderObject {
public:
	void bind() const {
		// Binding is managed internally
	}

	void unbind() const {
		// Binding is managed internally
	}

	T* getData() {
		return data;
	}

	int getBindingPoint() const {
		return bindingpoint;
	}

	int getSize() const {
		return size;
	}

protected:
	gUboBase() : size(0), data(nullptr), bindingpoint(0) {}
	~gUboBase() {
		delete data;
	}

	int size;
	T* data;
	int bindingpoint;
};
