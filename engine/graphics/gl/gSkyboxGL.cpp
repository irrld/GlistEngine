/*
 * gSkyboxGL.cpp
 *
 *  Created on: 18 Mar 2021
 *      Author: Acer
 *
 *  Refactored: OpenGL implementation
 */

#include "gSkyboxGL.h"
#include "gShader.h"
#include "gTexture.h"
#include "gFbo.h"
#include "gImage.h"
#include "gTracy.h"

glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[] = {
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

gSkyboxGL::gSkyboxGL() {
	captureFBO = 0;
	captureRBO = 0;
	irradianceHandle = nullptr;
	prefilterHandle = nullptr;
	brdfTexture = 0;
	cubeVAO = 0;
	cubeVBO = 0;
	quadVAO = 0;
	quadVBO = 0;
	skyboxshader = nullptr;
	pbrShader = nullptr;
	equirectangularToCubemapShader = nullptr;
	irradianceShader = nullptr;
	prefilterShader = nullptr;
	brdfShader = nullptr;
}

gSkyboxGL::~gSkyboxGL() {
	// skyhandle is stack-allocated, its destructor calls destroy()
	if (cubeVAO) {
		G_CHECK_GL(glDeleteVertexArrays(1, &cubeVAO));
		G_CHECK_GL(glDeleteBuffers(1, &cubeVBO));
	}
	if (quadVAO) {
		G_CHECK_GL(glDeleteVertexArrays(1, &quadVAO));
		G_CHECK_GL(glDeleteBuffers(1, &quadVBO));
	}
	if (captureRBO != 0) {
		G_CHECK_GL(glDeleteRenderbuffers(1, &captureRBO));
	}
	if (captureFBO != 0) {
		G_CHECK_GL(glDeleteFramebuffers(1, &captureFBO));
	}
	delete irradianceHandle;
	delete prefilterHandle;
	if (brdfTexture != 0) {
		G_CHECK_GL(glDeleteTextures(1, &brdfTexture));
	}
}

unsigned int gSkyboxGL::loadTextures(const std::vector<std::string>& paths) {
	std::vector<std::string> fullpaths;
	fullpaths.resize(6);
	for (int i = 0; i < paths.size(); i++) {
		fullpaths[i] = gGetTexturesDir() + paths[i];
	}
	return load(fullpaths);
}

unsigned int gSkyboxGL::load(const std::vector<std::string>& fullPaths) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;

	renderer->enableCubeMap();

	id = skyhandle.create();
	skyhandle.bind(skymapslot);

	for (unsigned int i = 0; i < fullPaths.size(); i++) {
        unsigned char *data = stbi_load(fullPaths[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
        	skyhandle.texImage2D(i, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
        	gLoge("gSkyBox") << "Cubemap tex failed to load at path: " << fullPaths[i];
            stbi_image_free(data);
        }
    }
	skyhandle.setWrappingAndFiltering(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

	renderer->getSkyboxShader()->use();
	renderer->getSkyboxShader()->setInt("skymap", skymapint);

	if(ispbr) generatePbrMaps();
	skyhandle.unbind(0);

    return id;
}

void gSkyboxGL::loadSkybox(gImage* images) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;

	renderer->enableCubeMap();

	id = skyhandle.create();
	skyhandle.bind(skymapslot);

	for (unsigned int i = 0; i < 6; i++) {
		skyhandle.texImage2D(i, GL_RGB,
			images[i].getWidth(), images[i].getHeight(), GL_RGB,
			GL_UNSIGNED_BYTE, images[i].getImageData());
    }

	skyhandle.setWrappingAndFiltering(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

	renderer->getSkyboxShader()->use();
	renderer->getSkyboxShader()->setInt("skymap", skymapint);

	if(ispbr) generatePbrMaps();

	skyhandle.unbind(0);
}

void gSkyboxGL::loadFromData(std::array<int, 6> widths, std::array<int, 6> heights, std::array<void*, 6> rawdata, std::array<bool, 6> ishdr) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;

	renderer->enableCubeMap();

	id = skyhandle.create();
	skyhandle.bind(skymapslot);

	for (unsigned int i = 0; i < 6; i++) {
		if (ishdr[i]) {
			skyhandle.texImage2D(i, GL_RGB, widths[i], heights[i], GL_RGB, GL_FLOAT, rawdata[i]);
		} else {
			skyhandle.texImage2D(i, GL_RGB, widths[i], heights[i], GL_RGB, GL_UNSIGNED_BYTE, rawdata[i]);
		}
	}

	skyhandle.setWrappingAndFiltering(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

	renderer->getSkyboxShader()->use();
	renderer->getSkyboxShader()->setInt("skymap", skymapint);

	if(ispbr) generatePbrMaps();

	skyhandle.unbind(0);
}

void gSkyboxGL::loadDataSkybox(std::string *data, int width, int height) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;

	renderer->enableCubeMap();

	id = skyhandle.create();
	skyhandle.bind(skymapslot);

	for (unsigned int i = 0; i < 6; i++) {
		skyhandle.texImage2D(i, GL_RGB,
				width, height, GL_RGB, GL_UNSIGNED_BYTE,
				(unsigned char*)gDecodeBase64(data[i]).c_str());
    }

	skyhandle.setWrappingAndFiltering(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

	renderer->getSkyboxShader()->use();
	renderer->getSkyboxShader()->setInt("skymap", skymapint);

	if(ispbr) generatePbrMaps();

	skyhandle.unbind(0);
}

unsigned int gSkyboxGL::loadTextureEquirectangular(const std::string& texturePath) {
	return loadEquirectangular(gGetTexturesDir() + texturePath);
}

unsigned int gSkyboxGL::loadEquirectangular(const std::string& fullPath) {
	ishdr = true;
	skymapslot = GL_TEXTURE0;
	skymapint = 0;

	renderer->enableCubeMap();

	equirectangularToCubemapShader = renderer->getEquirectangularShader();

	G_CHECK_GL(glGenFramebuffers(1, &captureFBO));
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, captureFBO));
	G_CHECK_GL(glGenRenderbuffers(1, &captureRBO));
	G_CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, captureRBO));
	G_CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512));
	G_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO));

	//***********************************************
	id = skyhandle.create();
	skyhandle.bind(skymapslot);
	for (unsigned int i = 0; i < 6; ++i) {
		skyhandle.texImage2D(i, GL_RGB32F, 512, 512, GL_RGB, GL_FLOAT, nullptr);
	}
	skyhandle.setWrappingAndFiltering(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

	equirectangularToCubemapShader->use();
	equirectangularToCubemapShader->setInt("equirectangularMap", 0);
	equirectangularToCubemapShader->setMat4("projection", captureProjection);

	// pbr: load the HDR environment map
	gTexture hdr;
	hdr.load(fullPath);
	hdr.bind(0);

	renderer->setViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, captureFBO));
	for (unsigned int i = 0; i < 6; ++i) {
		equirectangularToCubemapShader->setMat4("view", captureViews[i]);
		G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, skyhandle.getId(), 0));
		renderer->clearScreen(true, true);
		renderCube();
	}
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, gFbo::defaultfbo));

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	skyhandle.bind();
	skyhandle.generateMipMap();

	if(ispbr) generatePbrMaps();
	renderer->setViewport(0, 0, getScreenWidth(), getScreenHeight());
	return id;
}

void gSkyboxGL::generatePbrMaps() {
	skyhandle.bind(skymapslot);

	pbrShader = renderer->getPbrShader();
	irradianceShader = renderer->getIrradianceShader();
	prefilterShader = renderer->getPrefilterShader();
	brdfShader = renderer->getBrdfShader();

	// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	// --------------------------------------------------------------------------------

	irradianceHandle = new gSkyTextureGL();
	irradianceHandle->create();
	irradianceHandle->bind();
	for (unsigned int i = 0; i < 6; ++i) {
		irradianceHandle->texImage2D(i, GL_RGB32F, 32, 32, GL_RGB, GL_FLOAT, nullptr);
	}
	irradianceHandle->setWrappingAndFiltering(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, captureFBO));
	G_CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, captureRBO));
	G_CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32));

	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
	// -----------------------------------------------------------------------------
	irradianceShader->use();
	irradianceShader->setInt("environmentMap", 0);
	irradianceShader->setMat4("projection", captureProjection);
	skyhandle.bind(GL_TEXTURE0);

	renderer->setViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, captureFBO));
	for (unsigned int i = 0; i < 6; ++i) {
		irradianceShader->setMat4("view", captureViews[i]);
		G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceHandle->getId(), 0));
		renderer->clearScreen(true, true);
		renderCube();
	}
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, gFbo::defaultfbo));

	// pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
	// --------------------------------------------------------------------------------

	prefilterHandle = new gSkyTextureGL();
	prefilterHandle->create();
	prefilterHandle->bind();
	for (unsigned int i = 0; i < 6; ++i) {
		prefilterHandle->texImage2D(i, GL_RGB32F, 128, 128, GL_RGB, GL_FLOAT, nullptr);
	}
	prefilterHandle->setWrappingAndFiltering(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
	prefilterHandle->generateMipMap();
	// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	// ----------------------------------------------------------------------------------------------------
	prefilterShader->use();
	prefilterShader->setInt("environmentMap", 0);
	prefilterShader->setMat4("projection", captureProjection);
	skyhandle.bind(GL_TEXTURE0);

	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, captureFBO));
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = 128 * std::pow(0.5, mip);
		unsigned int mipHeight = 128 * std::pow(0.5, mip);
		G_CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, captureRBO));
		G_CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight));
		renderer->setViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		prefilterShader->setFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i) {
			prefilterShader->setMat4("view", captureViews[i]);
			G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterHandle->getId(), mip));
			renderer->clearScreen(true, true);
			renderCube();
		}
	}
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, gFbo::defaultfbo));

	// pbr: generate a 2D LUT from the BRDF equations used.
	// ----------------------------------------------------

	G_CHECK_GL(glGenTextures(1, &brdfTexture));

	// pre-allocate enough memory for the LUT texture.
	G_CHECK_GL(glBindTexture(GL_TEXTURE_2D, brdfTexture));
	G_CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 512, 512, 0, GL_RG, GL_FLOAT, 0));
	// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	G_CHECK_GL(glDeleteRenderbuffers(1, &captureRBO));
	G_CHECK_GL(glDeleteFramebuffers(1, &captureFBO));
	captureFBO = 0;
	captureRBO = 0;
	G_CHECK_GL(glGenFramebuffers(1, &captureFBO));
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, captureFBO));
	G_CHECK_GL(glGenRenderbuffers(1, &captureRBO));
	G_CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, captureRBO));
	G_CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512));
	G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfTexture, 0));

	renderer->setViewport(0, 0, renderer->getScreenWidth(), renderer->getScreenHeight());
	brdfShader->use();
	renderer->clearScreen(true, true);
	renderQuad();

	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, gFbo::defaultfbo));

	pbrShader->use();
	pbrShader->setInt("irradianceMap", 0);
	pbrShader->setInt("prefilterMap", 1);
	pbrShader->setInt("brdfLUT", 2);


	// initialize static shader uniforms before rendering
	// --------------------------------------------------
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)renderer->getScreenWidth() / (float)renderer->getScreenHeight(), 0.1f, 100.0f);
	pbrShader->use();
	pbrShader->setMat4("projection", projection);


	renderer->getSkyboxShader()->use();
	renderer->getSkyboxShader()->setInt("skymap", skymapint);

	skyhandle.unbind(0);
	ispbr = true;
}

void gSkyboxGL::bindPbrMaps() {
	pbrShader->use();
	pbrShader->setMat4("view", renderer->getViewMatrix());
	pbrShader->setVec3("camPos", renderer->getCameraPosition());
	irradianceHandle->bind(GL_TEXTURE0);
	prefilterHandle->bind(GL_TEXTURE1);
	G_CHECK_GL(glActiveTexture(GL_TEXTURE2));
	G_CHECK_GL(glBindTexture(GL_TEXTURE_2D, brdfTexture));
}

void gSkyboxGL::draw() {
	G_PROFILE_ZONE_SCOPED_N("gSkybox::draw()");
	skyboxshader = renderer->getSkyboxShader();
	renderer->enableDepthTestEqual(); // change depth function so depth test passes when values are equal to depth buffer's content
	skyboxshader->use();
	skyboxshader->setInt("aIsHDR", ishdr);
	skyboxshader->setMat4("projection", renderer->getProjectionMatrix());
	skyboxshader->setMat4("view", renderer->getViewMatrix());
	skyboxshader->setMat4("model", localtransformationmatrix);

	skyhandle.bind(skymapslot);
    vbo.bind();
	renderer->drawElements(GL_TRIANGLES, vbo.getIndicesNum());
    vbo.unbind();
	skyhandle.unbind();
}


void gSkyboxGL::renderCube() {
	G_PROFILE_ZONE_SCOPED_N("gSkybox::renderCube()");
    // initialize (if necessary)
    if (cubeVAO == 0) {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
    	G_CHECK_GL(glGenVertexArrays(1, &cubeVAO));
    	G_CHECK_GL(glGenBuffers(1, &cubeVBO));
        // fill buffer
    	G_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, cubeVBO));
    	G_CHECK_GL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));
        // link vertex attributes
    	G_CHECK_GL(glBindVertexArray(cubeVAO));
    	G_CHECK_GL(glEnableVertexAttribArray(0));
    	G_CHECK_GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0));
    	G_CHECK_GL(glEnableVertexAttribArray(1));
    	G_CHECK_GL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))));
    	G_CHECK_GL(glEnableVertexAttribArray(2));
    	G_CHECK_GL(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))));
    	G_CHECK_GL(glBindVertexArray(0));
    }
    // render Cube
	G_CHECK_GL(glBindVertexArray(cubeVAO));
	G_CHECK_GL(glDrawArrays(GL_TRIANGLES, 0, 36));
	G_CHECK_GL(glBindVertexArray(0));
}

void gSkyboxGL::renderQuad() {
	G_PROFILE_ZONE_SCOPED_N("gSkybox::renderQuad()");
    if (quadVAO == 0) {
    	float quadVertices[] = {
    		// positions        // texture Coords
    		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    	};
    	G_CHECK_GL(glGenVertexArrays(1, &quadVAO));
    	G_CHECK_GL(glGenBuffers(1, &quadVBO));
    	G_CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
    	G_CHECK_GL(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));
    	G_CHECK_GL(glBindVertexArray(quadVAO));
    	G_CHECK_GL(glEnableVertexAttribArray(0));
    	G_CHECK_GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0));
    	G_CHECK_GL(glEnableVertexAttribArray(1));
    	G_CHECK_GL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))));
    	G_CHECK_GL(glBindVertexArray(0));
    }
	G_CHECK_GL(glBindVertexArray(quadVAO));
	G_CHECK_GL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
	G_CHECK_GL(glBindVertexArray(0));
}
