/*
 * gRenderManager.h
 *
 *  Created on: 4 Ara 2020
 *      Author: Acer
 */

#pragma once

#ifndef CORE_GRENDERER_H_
#define CORE_GRENDERER_H_

#include "gObject.h"

#if defined(GLIST_VULKAN)
// ---- Vulkan backend: provide GL-compatible typedefs so existing engine code
//      (gVbo, gFbo, gShader, gTexture, gUbo, gMesh, etc.) compiles unchanged.
//      The Vulkan renderer interprets these handle values through internal mapping.
#include <cstdint>
typedef uint32_t GLuint;
typedef uint32_t GLenum;
typedef int32_t  GLint;
typedef int32_t  GLsizei;
typedef intptr_t GLsizeiptr;
typedef uint8_t  GLboolean;
typedef float    GLfloat;

// GL constants referenced in the public engine interface and graphics classes.
// Values match the real OpenGL constants so serialized data remains compatible.
// Use #ifndef guards because GL/gl.h may still be included (e.g. via GLFW).
#ifndef GL_NONE
#define GL_NONE                          0
#endif
#ifndef GL_FALSE
#define GL_FALSE                         0
#endif
#ifndef GL_TRUE
#define GL_TRUE                          1
#endif
#ifndef GL_POINTS
#define GL_POINTS                        0x0000
#endif
#ifndef GL_LINES
#define GL_LINES                         0x0001
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP                     0x0002
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP                    0x0003
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES                     0x0004
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP                0x0005
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN                  0x0006
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE                 0x1401
#endif
#ifndef GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT                0x1403
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT                  0x1405
#endif
#ifndef GL_FLOAT
#define GL_FLOAT                         0x1406
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8             0x84FA
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT               0x1902
#endif
#ifndef GL_RED
#define GL_RED                           0x1903
#endif
#ifndef GL_GREEN
#define GL_GREEN                         0x1904
#endif
#ifndef GL_BLUE
#define GL_BLUE                          0x1905
#endif
#ifndef GL_RGBA
#define GL_RGBA                          0x1908
#endif
#ifndef GL_RGB
#define GL_RGB                           0x1907
#endif
#ifndef GL_RG
#define GL_RG                            0x8227
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL                 0x84F9
#endif
#ifndef GL_LESS
#define GL_LESS                          0x0201
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS                        0x0207
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D                    0x0DE1
#endif
#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP              0x8513
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_X
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X   0x8515
#endif
#ifndef GL_NEAREST
#define GL_NEAREST                       0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR                        0x2601
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR          0x2703
#endif
#ifndef GL_REPEAT
#define GL_REPEAT                        0x2901
#endif
#ifndef GL_CLAMP
#define GL_CLAMP                         0x2900
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE                 0x812F
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW                   0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW                  0x88E8
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER                  0x8892
#endif
#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER                0x8A11
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0                      0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1                      0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2                      0x84C2
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0             0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT              0x8D00
#endif
#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT      0x821A
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24             0x81A6
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8              0x88F0
#endif
#ifndef GL_RGB16F
#define GL_RGB16F                        0x881B
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F                       0x881A
#endif
#ifndef GL_RGB32F
#define GL_RGB32F                        0x8815
#endif
#ifndef GL_RG32F
#define GL_RG32F                         0x8230
#endif
#ifndef GL_SRGB
#define GL_SRGB                          0x8C40
#endif
#ifndef GL_SRGB_ALPHA
#define GL_SRGB_ALPHA                    0x8C42
#endif

#else // GLIST_OGL -- OpenGL backend
#if defined(WIN32) || defined(LINUX)
//#include <GL/glext.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#if defined(EMSCRIPTEN)
#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h> // well, okay?
#include <GLES3/gl3platform.h>
#endif
#if defined(ANDROID)
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES3/gl3platform.h>
#endif
#if TARGET_OS_OSX
#include <GL/glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif
#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
#	include <OpenGLES/ES3/gl.h>
#	include <OpenGLES/ES3/glext.h>
#	include <OpenGLES/gltypes.h>
#endif
#endif // GLIST_VULKAN
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtx/quaternion.hpp>

#include "gColor.h"
#include "gConstants.h"
#include <deque>
#include <memory>
#include <vector>

#ifndef GLIST_MAX_LIGHTS
// amount of maximum lights, this is used to allocate memory for the light uniform buffer
#define GLIST_MAX_LIGHTS 8
#endif

// You can define ENGINE_OPENGL_CHECKS to enable OpenGL checks
// without debugging.
#if defined(GLIST_VULKAN)
// Under Vulkan, GL error checking macros are pass-throughs
#define G_CHECK_GL(fn) fn
#define G_CHECK_GL2(value, fn) value = fn
#else
#if defined(DEBUG) || defined(ENGINE_OPENGL_CHECKS)
#define G_CHECK_GL(a) gCheckGLErrorAndPrint("Previously Unhandled ", __PRETTY_FUNCTION__, __LINE__); a; gCheckGLErrorAndPrint("", __PRETTY_FUNCTION__, __LINE__)
#define G_CHECK_GL2(value, fn) gCheckGLErrorAndPrint("Previously Unhandled ", __PRETTY_FUNCTION__, __LINE__); value = fn; gCheckGLErrorAndPrint("", __PRETTY_FUNCTION__, __LINE__)
#else
#define G_CHECK_GL(fn) fn
#define G_CHECK_GL2(value, fn) value = fn
#endif
#endif

#if !defined(GLIST_VULKAN)
void gCheckGLErrorAndPrint(const std::string& prefix, const std::string& func, int line);
#endif

void gEnableCulling();
void gDisableCulling();
bool gIsCullingEnabled();
void gCullFace(int cullingFace);
int gGetCullFace();
void gSetCullingDirection(int cullingDirection);
int gGetCullingDirection();

void gDrawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f);
void gDrawLine(float x1, float y1, float z1, float x2, float y2, float z2, float thickness = 1.0f);
void gDrawTriangle(float px, float py, float qx, float qy, float rx, float ry, bool is_filled = true);
void gDrawCircle(float xCenter, float yCenter, float radius, bool isFilled = false, float numberOfSides = 64.0f);
void gDrawCross(float x, float y, float width, float height, float thickness, bool isFilled);
void gDrawArc(float xCenter, float yCenter, float radius, bool isFilled = true, int numberOfSides = 60, float degree = 360.0f, float rotate = 360.0f);
void gDrawArrow(float x1, float y1, float length, float angle, float tipLength, float tipAngle);
void gDrawRectangle(float x, float y, float w, float h, bool isFilled = false);
void gDrawRoundedRectangle(float x, float y, float w, float h, int radius, bool isFilled);
void gDrawBox(float x, float y, float z, float w = 1.0f, float h = 1.0f, float d = 1.0f, bool isFilled = true);
void gDrawBox(glm::mat4 transformationMatrix, bool isFilled = true);
void gDrawSphere(float xPos, float yPos, float zPos, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int xSegmentNum = 64, int ySegmentNum = 32, bool isFilled = true);
void gDrawCylinder(float x, float y, float z, int r, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawCylinderOblique(float x, float y, float z, int r, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawCylinderTrapezodial(float x, float y, float z, int r1, int r2, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawCylinderObliqueTrapezodial(float x, float y, float z, int r1, int r2, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0), int segmentnum = 32, bool isFilled = true);
void gDrawCone(float x, float y, float z, int r, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawConeOblique(float x, float y, float z, int r, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawPyramid(float x, float y, float z, int r, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int numberofsides = 4, bool isFilled = true);
void gDrawPyramidOblique(float x, float y, float z, int r, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int numberofsides = 4, bool isFilled = true);
void gDrawTube(float x, float y, float z, int outerradius,int innerradious, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawTubeOblique(float x, float y, float z, int outerradius,int innerradious, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawTubeTrapezodial(float x, float y, float z, int topouterradius,int topinnerradious, int buttomouterradious, int buttominnerradious, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
void gDrawTubeObliqueTrapezodial(float x, float y, float z, int topouterradius,int topinnerradious, int buttomouterradious, int buttominnerradious, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);

// Forward declarations — concrete class + type alias to avoid conflict
// with the type alias headers (gVbo.h, gFbo.h, gShader.h, gUbo.h).
#if defined(GLIST_VULKAN)
class gVboVk;
using gVbo = gVboVk;
class gFboVk;
using gFbo = gFboVk;
class gShaderVk;
using gShader = gShaderVk;
template<typename T> class gUboVk;
template<typename T> using gUbo = gUboVk<T>;
#else
class gVboGL;
using gVbo = gVboGL;
class gFboGL;
using gFbo = gFboGL;
class gShaderGL;
using gShader = gShaderGL;
template<typename T> class gUboGL;
template<typename T> using gUbo = gUboGL<T>;
#endif
class gLight;
class gImage;
class gCamera;
class gGrid;
class gLine;
class gTriangle;
class gCircle;
class gCross;
class gArc;
class gRectangle;
class gRoundedRectangle;
class gBox;


class gRenderer : public gObject {
public:
	static const int SCREENSCALING_NONE, SCREENSCALING_MIPMAP, SCREENSCALING_AUTO;
	static const int DEPTHTESTTYPE_LESS, DEPTHTESTTYPE_ALWAYS;
	static const int FOGMODE_LINEAR, FOGMODE_EXP;
	struct alignas(16) gSceneLightData {
		alignas(4) int type;
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 direction;
		alignas(16) glm::vec4 ambient;
		alignas(16) glm::vec4 diffuse;
		alignas(16) glm::vec4 specular;

		alignas(4) float constant;
		alignas(4) float linear;
		alignas(4) float quadratic;

		alignas(4) float spotcutoffangle;
		alignas(4) float spotoutercutoffangle;
	};

	struct alignas(16) gSceneLights {
		alignas(4) int lightnum = 0;
		// bitwise enabled lights, 1 means enabled, 0 means disabled, 32-bit integer
		// supports only 32 max lights, make sure to change this if max lights is changed to be something above 32
		alignas(4) int enabledlights = 0;
		alignas(16) glm::vec4 globalambientcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gSceneLightData lights[GLIST_MAX_LIGHTS];
	};

	enum SceneDataFlags {
		ENABLE_SSAO = 0b0001,
		ENABLE_FOG = 0b0010,
		ENABLE_GAMMA = 0b0100,
		ENABLE_HDR = 0b1000,
		ENABLE_SOFT_SHADOWS = 0b10000
	};

	struct alignas(16) gSceneFogData {
		alignas(16) glm::vec3 color;
		alignas(4) float linearStart;
		alignas(4) float linearEnd;
		alignas(4) float density;
		alignas(4) float gradient;
		alignas(4) int mode;
	};
	struct alignas(16) gSceneData {
		alignas(16) gColor rendercolor;
		alignas(16) glm::vec3 viewpos;
		alignas(16) glm::mat4 viewmatrix;
		alignas(4) unsigned int flags;
		gSceneFogData fog;
	};

	gRenderer() = default;
	virtual ~gRenderer();

	static void setScreenSize(int screenWidth, int screenHeight);
	static void setUnitScreenSize(int unitWidth, int unitHeight);
	static void setScreenScaling(int screenScaling);

	int getWidth();
	int getHeight();
	int getScreenWidth();
	int getScreenHeight();
	int getUnitWidth();
	int getUnitHeight();
	static int getScreenScaling();
	gVbo* getFullscreenQuad() const;

	static void setCurrentResolution(int resolution);
	static void setCurrentResolution(int screenWidth, int screenHeight);
	static void setUnitResolution(int resolution);
	static void setUnitResolution(int screenWidth, int screenHeight);
	static int getResolution(int screenWidth, int screenHeight);
	int getCurrentResolution();
	int getUnitResolution();
	static float getScaleMultiplier();
	static int scaleX(int x);
	static int scaleY(int y);
	static int unscaleX(int x);
	static int unscaleY(int y);

	//grid
	void drawGrid();
	void drawGridYZ();
	void drawGridXY();
	void drawGridXZ();
	void enableGrid();
	void disableGrid();
	void setGridEnableAxis(bool xy, bool yz, bool xz);
	void setGridEnableXY(bool xy);
	void setGridEnableYZ(bool yz);
	void setGridEnableXZ(bool xz);
	void setGridColorofAxisXZ(int r, int g, int b, int a);
	void setGridColorofAxisYZ(int r, int g, int b, int a);
	void setGridColorofAxisXY(int r, int g, int b, int a);
	void setGridColorofAxisXZ(gColor* color);
	void setGridColorofAxisYZ(gColor* color);
	void setGridColorofAxisXY(gColor* color);
	void setGridColorofAxisWireFrameXZ(int r, int g, int b, int a);
	void setGridColorofAxisWireFrameYZ(int r, int g, int b, int a);
	void setGridColorofAxisWireFrameXY(int r, int g, int b, int a);
	void setGridColorofAxisWireFrameXZ(gColor* color);
	void setGridColorofAxisWireFrameYZ(gColor* color);
	void setGridColorofAxisWireFrameXY(gColor* color);
	bool isGridEnabled();
	bool isGridXYEnabled();
	bool isGridYZEnabled();
	bool isGridXZEnabled();
	void setGridMaxLength(float length);
	float getGridMaxLength();
	void setGridLineInterval(float intervalvalue);
	float getGridLineInterval();

	gGrid* getGrid() const;
	void setGrid(gGrid* newgrid);

	void setColor(int r, int g, int b, int a = 255);
	void setColor(float r, float g, float b, float a = 1.0f);
	void setColor(const gColor& color);
	void setColor(gColor* color);
	gColor* getColor();

	virtual void beginFrame() {}
	virtual void endFrame() {}

	virtual void clear() = 0;
	virtual void clearColor(int r, int g, int b, int a = 255) = 0;
	virtual void clearColor(gColor color) = 0;

	void enableLighting();
	void disableLighting();
	bool isLightingEnabled();
	void setLightingColor(int r, int g, int b, int a = 255);
	void setLightingColor(gColor* color) { setLightingColor(color->r, color->g, color->b, color->a); }
	gColor* getLightingColor();
	void setLightingPosition(glm::vec3 lightingPosition);
	glm::vec3 getLightingPosition();

	void setGlobalAmbientColor(int r, int g, int b, int a = 255);
	void setGlobalAmbientColor(gColor color) { setGlobalAmbientColor(color.r, color.g, color.b, color.a); }
	gColor* getGlobalAmbientColor();

	void enableFog();
	void disableFog();
	void setFogNo(int no);
	void setFogColor(float r, float g, float b);
	void setFogColor(const gColor& color);
	void setFogMode(int fogMode);
	void setFogDensity(float value);
	void setFogGradient(float value);
	void setFogLinearStart(float value);
	void setFogLinearEnd(float value);

	bool isFogEnabled();
	int getFogNo() const;
	const gColor& getFogColor() const;
	int getFogMode() const;
	float getFogDensity() const;
	float getFogGradient() const;
	float getFogLinearStart() const;
	float getFogLinearEnd() const;

	// add and remove functions are called by gLight class automatically,
	// so you don't need to
	void addSceneLight(gLight* light);
	void removeSceneLight(gLight* light);
	gLight* getSceneLight(int lightNo);
	int getSceneLightNum();
	void removeAllSceneLights();
	void updateLights();

	void updateScene();

	virtual void enableDepthTest() = 0;
	virtual void enableDepthTest(int depthTestType) = 0;
	virtual void setDepthTestFunc(int depthTestType) = 0;
	virtual void disableDepthTest() = 0;
	virtual bool isDepthTestEnabled() = 0;
	virtual int getDepthTestType() = 0;

	virtual void enableAlphaBlending() = 0;
	virtual void disableAlphaBlending() = 0;
	virtual bool isAlphaBlendingEnabled() = 0;
	virtual void enableAlphaTest() = 0;
	virtual void disableAlphaTest() = 0;
	virtual bool isAlphaTestEnabled() = 0;

	bool isSSAOEnabled();
	void enableSSAO();
	void disableSSAO();
	void setSSAOBias(float value);
	float getSSAOBias();
	void setSSAORadius(float value);
	float getSSAORadius();
	void setSSAOStrength(float value);
	float getSSAOStrength();
	void setSSAODebug(bool enabled);
	bool isSSAODebug();
	bool isSSAOAllocated();
	void beginSSAO();
	void endSSAO();

	bool isGammaCorrectionEnabled();
	void enableGammaCorrection();
	void disableGammaCorrection();

	bool isHDREnabled();
	void enableHDR();
	void disableHDR();

	bool isSoftShadowsEnabled();
	void enableSoftShadows();
	void disableSoftShadows();

	gShader* getColorShader();
	gShader* getTextureShader();
	gShader* getFontShader();
	gShader* getImageShader();
	gShader* getSkyboxShader();
	gShader* getShadowmapShader();
	gShader* getPbrShader();
	gShader* getEquirectangularShader();
	gShader* getIrradianceShader();
	gShader* getPrefilterShader();
	gShader* getBrdfShader();
	gShader* getFboShader();
	gShader* getGridShader();

	void setProjectionMatrix(glm::mat4 projectionMatrix);
	void setProjectionMatrix2d(glm::mat4 projectionMatrix2d);
	void setViewMatrix(glm::mat4 viewMatrix);
	void setCameraPosition(glm::vec3 cameraPosition);
	void setCamera(gCamera* camera);
	const glm::mat4& getProjectionMatrix() const;
	const glm::mat4& getProjectionMatrix2d() const;
	const glm::mat4& getViewMatrix() const;
	const glm::vec3& getCameraPosition() const;
	const gCamera* getCamera() const;
	void backupMatrices();
	void restoreMatrices();

	/*
	 * Takes Screen Shot of the current Rendered Screen and returns it as an gImage class
	 */
	virtual void takeScreenshot(gImage& img) = 0;

	/*
	 * Takes Screen Shot of the part of current Rendered Screen and returns it as an gImage class
	 */
	virtual void takeScreenshot(gImage& img, int x, int y, int width, int height) = 0;

	/* -------------- Drawing ------------- */
	virtual void drawArrays(int drawMode, int count) = 0;
	virtual void drawElements(int drawMode, int count) = 0;
	virtual void setViewport(int x, int y, int width, int height) = 0;

	/* ------------ Screen / Framebuffer ------------- */
	virtual void clearScreen(bool color = true, bool depth = true) = 0;
	void bindQuadVAO();
	void drawFullscreenQuad();
	virtual void bindDefaultFramebuffer() = 0;

	/* -------------- Depth / Skybox --------------- */
	virtual void enableDepthTestEqual() = 0;
	virtual void enableCubeMap() = 0;

	/* ---------------- gRenderObject ---------------- */
	virtual void pushMatrix() = 0;
	virtual void popMatrix() = 0;

	/* ---------------- Utilities ---------------- */
	void drawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f);
	void drawLine(float x1, float y1, float z1, float x2, float y2, float z2, float thickness = 1.0f);
	void drawTriangle(float px, float py, float qx, float qy, float rx, float ry, bool is_filled = true);
	void drawCircle(float xCenter, float yCenter, float radius, bool isFilled = false, float numberOfSides = 64.0f);
	void drawCross(float x, float y, float width, float height, float thickness, bool isFilled);
	void drawArc(float xCenter, float yCenter, float radius, bool isFilled = true, int numberOfSides = 60, float degree = 360.0f, float rotate = 360.0f);
	void drawArrow(float x1, float y1, float length, float angle, float tipLength, float tipAngle);
	void drawRectangle(float x, float y, float w, float h, bool isFilled = false);
	void drawRoundedRectangle(float x, float y, float w, float h, int radius, bool isFilled);
	void drawBox(float x, float y, float z, float w = 1.0f, float h = 1.0f, float d = 1.0f, bool isFilled = true);
	void drawBox(glm::mat4 transformationMatrix, bool isFilled = true);
	void drawSphere(float xPos, float yPos, float zPos, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int xSegmentNum = 64, int ySegmentNum = 32, bool isFilled = true);
	void drawCylinder(float x, float y, float z, int r, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawCylinderOblique(float x, float y, float z, int r, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawCylinderTrapezodial(float x, float y, float z, int r1, int r2, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawCylinderObliqueTrapezodial(float x, float y, float z, int r1, int r2, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0), int segmentnum = 32, bool isFilled = true);
	void drawCone(float x, float y, float z, int r, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawConeOblique(float x, float y, float z, int r, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawPyramid(float x, float y, float z, int r, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int numberofsides = 4, bool isFilled = true);
	void drawPyramidOblique(float x, float y, float z, int r, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int numberofsides = 4, bool isFilled = true);
	void drawTube(float x, float y, float z, int outerradius,int innerradious, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawTubeOblique(float x, float y, float z, int outerradius,int innerradious, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawTubeTrapezodial(float x, float y, float z, int topouterradius,int topinnerradious, int buttomouterradious, int buttominnerradious, int h, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);
	void drawTubeObliqueTrapezodial(float x, float y, float z, int topouterradius,int topinnerradious, int buttomouterradious, int buttominnerradious, int h, glm::vec2 shiftdistance, glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f), int segmentnum = 32, bool isFilled = true);

protected:
	friend class gRenderObject; // this is where renderer->init() is called from
	friend class gAppManager;

	static int width, height;
	static int unitwidth, unitheight;
	static int screenscaling;
	static int currentresolution, unitresolution;

	gColor* rendercolor;

	bool isfogenabled;
	int fogno;
	gColor fogcolor;
	float fogdensity;
	float foggradient;
	int fogmode;
	float foglinearstart;
	float foglinearend;

	std::deque<gLight*> scenelights;
	gUbo<gSceneLights>* lightsubo;
	gUbo<gSceneData>* sceneubo;
	bool islightingenabled;
	glm::vec3 lightingposition;
	gColor lightingcolor;
	gColor globalambientcolor;
	bool isglobalambientcolorchanged;

	bool isdepthtestenabled;
	int depthtesttype;
	unsigned int depthtesttypeid[2];
	bool isalphablendingenabled, isalphatestenabled;

	bool isssaoenabled;
	float ssaobias;
	float ssaoradius;
	float ssaostrength;
	bool isssaoallocated;
	bool isssaodebug;
	gFbo* ssaofbo;
	gFbo* ssaoresultfbo;
	gShader* ssaoshader;
	gShader* ssaoblurshader;
	uint32_t ssaonoisetexid;
	std::vector<glm::vec3> ssaokernel;
	int ssaorealdefaultfbo;
	bool isssaorendering;
	void initSSAOResources();
	void cleanupSSAOResources();
	bool isgammacorrectionenabled;
	bool ishdrenabled;
	bool issoftshadowsenabled;

	gShader* colorshader;
	gShader* textureshader;
	gShader* fontshader;
	gShader* imageshader;
	gShader* skyboxshader;
	gShader* shadowmapshader;
	gShader* pbrshader;
	gShader* equirectangularshader;
	gShader* irradianceshader;
	gShader* prefiltershader;
	gShader* brdfshader;
	gShader* fboshader;
	gShader* gridshader;

	glm::mat4 projectionmatrix;
	glm::mat4 projectionmatrixold;
	glm::mat4 projectionmatrix2d;
	glm::mat4 viewmatrix;
	glm::mat4 viewmatrixold;
	glm::vec3 cameraposition;
	gCamera* camera;

	gGrid* grid;
	gGrid* originalgrid;

	gVbo* fullscreenquad;

	// std::unique_ptr is automatically deletes the underlying object when this gRenderer object is deleted
	std::unique_ptr<gLine> linemesh, linemesh2, linemesh3;
	std::unique_ptr<gTriangle> trianglemesh;
	std::unique_ptr<gCircle> circlemesh;
	std::unique_ptr<gCross> crossmesh;
	std::unique_ptr<gArc> arcmesh;
	std::unique_ptr<gRectangle> rectanglemesh;
	std::unique_ptr<gRoundedRectangle> roundedrectanglemesh;
	std::unique_ptr<gBox> boxmesh;

	virtual void init();
	virtual void cleanup();
	virtual void updatePackUnpackAlignment(int i) = 0;

	static const std::string& getShaderSrcGridVertex();
	static const std::string& getShaderSrcGridFragment();
	static const std::string& getShaderSrcColorVertex();
	static const std::string& getShaderSrcColorFragment();
	static const std::string& getShaderSrcTextureVertex();
	static const std::string& getShaderSrcTextureFragment();
	static const std::string& getShaderSrcImageVertex();
	static const std::string& getShaderSrcImageFragment();
	static const std::string& getShaderSrcFontVertex();
	static const std::string& getShaderSrcFontFragment();
	static const std::string& getShaderSrcSkyboxVertex();
	static const std::string& getShaderSrcSkyboxFragment();
	static const std::string& getShaderSrcShadowmapVertex();
	static const std::string& getShaderSrcShadowmapFragment();
	static const std::string& getShaderSrcPbrVertex();
	static const std::string& getShaderSrcPbrFragment();
	static const std::string& getShaderSrcCubemapVertex();
	static const std::string& getShaderSrcEquirectangularFragment();
	static const std::string& getShaderSrcIrradianceFragment();
	static const std::string& getShaderSrcPrefilterFragment();
	static const std::string& getShaderSrcBrdfVertex();
	static const std::string& getShaderSrcBrdfFragment();
	static const std::string& getShaderSrcFboVertex();
	static const std::string& getShaderSrcFboFragment();
	static const std::string& getShaderSrcSSAOVertex();
	static const std::string& getShaderSrcSSAOFragment();
	static const std::string& getShaderSrcSSAOBlurFragment();


};

#endif /* CORE_GRENDERER_H_ */
