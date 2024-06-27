/*
 * gLine.cpp
 *
 *  Created on: May 7, 2021
 *      Author: noyan
 */

#include "gLine.h"


gLine::gLine() {
	thickness = 1.0f;
}

gLine::gLine(float x1, float y1, float x2, float y2) {
	isprojection2d = true;
	setLinePoints(x1, y1, 0.0f, x2, y2, 0.0f);
}

gLine::gLine(float x1, float y1, float z1, float x2, float y2, float z2) {
	isprojection2d = false;
	setLinePoints(x1, y1, z1, x2, y2, z2);
}

void gLine::setPoints(float x1, float y1, float x2, float y2) {
	isprojection2d = true;
	setLinePoints(x1, y1, 0.0f, x2, y2, 0.0f);
}

void gLine::setPoints(float x1, float y1, float z1, float x2, float y2, float z2) {
	isprojection2d = false;
	setLinePoints(x1, y1, z1, x2, y2, z2);
}

void gLine::draw() {
	gMesh::draw();
}

void gLine::draw(float x1, float y1, float x2, float y2) {
	isprojection2d = true;
	setLinePoints(x1, y1, 0.0f, x2, y2, 0.0f);
	gMesh::draw();
}

void gLine::draw(float x1, float y1, float z1, float x2, float y2, float z2) {
	isprojection2d = false;
	setLinePoints(x1, y1, z1, x2, y2, z2);
	gMesh::draw();
}

void gLine::setThickness(float value) {
	thickness = value;
}

void gLine::setLinePoints(float x1, float y1, float z1, float x2, float y2, float z2) {
	if(!verticessb.empty()) {
		verticessb.clear();
		indicessb.clear();
	}

	if(thickness > 1.0f) {
		glm::vec3 v1 = glm::vec3(x1, y1, z1);
		glm::vec3 v2 = glm::vec3(x2, y2, z2);
		glm::vec3 d = glm::normalize(v2 - v1);
		glm::vec3 tangent = glm::cross(d, glm::vec3(0.0f, 0.0f, 1.0f));
		tangent = glm::normalize(tangent);

		verticessb.push_back({{x1 + tangent.x * thickness, y1 + tangent.y * thickness, z1 + tangent.z * thickness}});
		verticessb.push_back({{x2 + tangent.x * thickness, y2 + tangent.y * thickness, z2 + tangent.z * thickness}});
		verticessb.push_back({{x1 - tangent.x * thickness, y1 - tangent.y * thickness, z1 - tangent.z * thickness}});
		verticessb.push_back({{x2 - tangent.x * thickness, y2 - tangent.y * thickness, z2 - tangent.z * thickness}});

		indicessb = {0, 1, 3, 0, 2, 3};
		setDrawMode(gMesh::DRAWMODE_TRIANGLES);
	} else {
		verticessb.push_back({{x1, y1, z1}});
		verticessb.push_back({{x2, y2, z2}});

		setDrawMode(gMesh::DRAWMODE_LINES);
	}

	setVertices(verticessb, indicessb);
}
