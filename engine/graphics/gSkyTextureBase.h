/*
 * gSkyTextureBase.h
 *
 *  Created on: 2026-03-01
 *      Author: Metehan Gezer
 *
 *  Refactored: Base/GL/Vk split
 */

#ifndef GRAPHICS_GSKYTEXTUREBASE_H_
#define GRAPHICS_GSKYTEXTUREBASE_H_

#include "gRenderObject.h"


class gSkyTextureBase : public gRenderObject {
public:
	unsigned int getId() const;

protected:
	gSkyTextureBase();
	virtual ~gSkyTextureBase();

	unsigned int id;
};

#endif /* GRAPHICS_GSKYTEXTUREBASE_H_ */
