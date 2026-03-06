/*
 * gSkyboxBase.h
 *
 *  Created on: 18 Mar 2021
 *      Author: Acer
 *
 *  Refactored: Base/GL/Vk split
 */

#ifndef GRAPHICS_GSKYBOXBASE_H_
#define GRAPHICS_GSKYBOXBASE_H_

#include <array>
#include <string>
#include <vector>
#include "gNode.h"
#include "gVbo.h"


class gSkyboxBase : public gNode {
protected:
	gSkyboxBase();
	virtual ~gSkyboxBase();

	void setupRenderData();

	unsigned int id;
	int width, height, nrChannels;
	gVbo vbo;
	int skymapint, skymapslot;
	bool ishdr, ispbr;
};

#endif /* GRAPHICS_GSKYBOXBASE_H_ */
