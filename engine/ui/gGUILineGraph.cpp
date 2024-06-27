/*
 * gGUILineGraph.cpp
 *
 *  Created on: 28 July 2022
 *      Author: Berke Adil
 */


#include "gGUILineGraph.h"
#include "gGUISlider.h"

gGUILineGraph::gGUILineGraph() {
	title = "Graph";
	pointsenabled = true;

	linecolors[0] = {0.20f, 0.20f, 0.96f};
	linecolors[1] = {0.96f, 0.46f, 0.55f};
	linecolors[2] = {0.62f, 0.80f, 0.41f};
	linecolors[3] = {0.47f, 0.63f, 0.96f};
	linecolors[4] = {0.87f, 0.68f, 0.40f};
	linecolors[5] = {0.73f, 0.60f, 0.96f};
}

gGUILineGraph::~gGUILineGraph() {

}

void gGUILineGraph::set(gBaseApp* root, gBaseGUIObject* topParentGUIObject, gBaseGUIObject* parentGUIObject, int parentSlotLineNo, int parentSlotColumnNo, int x, int y, int w, int h) {
	gGUIGraph::set(root, topParentGUIObject, parentGUIObject, parentSlotLineNo, parentSlotColumnNo, x, y, w, h);
	needsupdate = true;
}

void gGUILineGraph::setMaxX(int maxX){
	gGUIGraph::setMaxX(maxX);
	needsupdate = true;
}


void gGUILineGraph::setMinX(int minX) {
	gGUIGraph::setMinX(minX);
	needsupdate = true;
}

void gGUILineGraph::setMaxY(int maxY) {
	gGUIGraph::setMaxY(maxY - 1);
	needsupdate = true;
}


void gGUILineGraph::setMinY(int minY) {
	gGUIGraph::setMinY(minY);
	needsupdate = true;
}

void gGUILineGraph::setLabelCountX(int labelCount){
	gGUIGraph::setLabelCountX(labelCount);
	needsupdate = true;
}

void gGUILineGraph::setLabelCountY(int labelCount) {
	gGUIGraph::setLabelCountY(labelCount);
	needsupdate = true;
}

void gGUILineGraph::enablePoints(bool arePointsEnabled) {
	pointsenabled = arePointsEnabled;
}

void gGUILineGraph::setLineColor(int lineIndex, gColor lineColor) {
	linecolors[lineIndex] = lineColor;
}

gColor gGUILineGraph::getLineColor(int lineIndex) {
	return linecolors[lineIndex];
}

void gGUILineGraph::addLine() {
	std::deque<std::array<float, 4>> newline;
	graphlines.push_back(newline);
}

void gGUILineGraph::addData(int lineIndex, std::deque<std::array<float, 2>> dataToAdd) {
	int datasize = dataToAdd.size();
	for(int i = 0; i < datasize; i++) addPointToLine(lineIndex, dataToAdd[i][0], dataToAdd[i][1]);
}

void gGUILineGraph::addPointToLine(int lineIndex, float x, float y) {
	if(graphlines.size() - 1 < lineIndex) return;
	if(x < smallestvaluex) setMinX(x);
	else if(x > largestvaluex) setMaxX(x);
	if(y < smallestvaluey) setMinY(y);
	else if(y > largestvaluey) setMaxY(y);
	setLabelCountX(getLabelCountX());
	setLabelCountY(getLabelCountY());
	int pointcount = graphlines[lineIndex].size();
	if(pointcount == 0) {
		graphlines[lineIndex].push_back({x, y, axisx1 + axisxw * (x - minx) / (maxx - minx), axisy2 - axisyh * (y - miny) / (maxy - miny)});
		return;
	}
	int index = 0;
	while(index < pointcount) {
		if(graphlines[lineIndex][index++][0] < x) continue;
		index--;
		break;
	}
	graphlines[lineIndex].insert(graphlines[lineIndex].begin() + index, {x, y, axisx1 + axisxw * (x - minx) / (maxx - minx), axisy2 - axisyh * (y - miny) / (maxy - miny)});
}

void gGUILineGraph::setPointValues(int lineIndex, int pointIndex, float newX, float newY) {
	if(graphlines.size() - 1 < lineIndex) return;
		if(newX < smallestvaluex) setMinX(newX);
		else if(newX > largestvaluex) setMaxX(newX);
		if(newY < smallestvaluey) setMinY(newY);
		else if(newY > largestvaluey) setMaxY(newY);
		setLabelCountX(getLabelCountX());
		setLabelCountY(getLabelCountY());
		int pointcount = graphlines[lineIndex].size();
		int index = 0;
		while(index < pointcount) {
			if(graphlines[lineIndex][index++][0] < newX) continue;
			index--;
			break;
		}
		graphlines[lineIndex][pointIndex - 1] = {newX,newY, axisx1 + axisxw * (newX - minx) / (maxx - minx), axisy2 - axisyh * (newY - miny) / (maxy - miny)};
}

void gGUILineGraph::setPointValues(int lineIndex, float oldX, float oldY, float newX, float newY) {
    if(graphlines.size() - 1 < lineIndex) return;
    if(newX < smallestvaluex) {
		setMinX(newX);
	} else if(newX > largestvaluex) {
		setMaxX(newX);
	}
	if(newY < smallestvaluey) {
		setMinY(newY);
	} else if(newY > largestvaluey) {
		setMaxY(newY);
	}
    setLabelCountX(getLabelCountX());
    setLabelCountY(getLabelCountY());
    int pointcount = graphlines[lineIndex].size();
    int index = 0;
    while(index < pointcount) {
        if(graphlines[lineIndex][index++][0] < newX) continue;
        index--;
        break;
    }
    for (int i = 0; i < pointcount; i++) {
        if (graphlines[lineIndex][i][0] == oldX && graphlines[lineIndex][i][1] == oldY) {
            graphlines[lineIndex][i][0] = newX;
            graphlines[lineIndex][i][1] = newY;
            graphlines[lineIndex][i][2] = axisx1 + axisxw * (newX - minx) / (maxx - minx);
            graphlines[lineIndex][i][3] = axisy2 - axisyh * (newY - miny) / (maxy - miny);
        }
    }
	needsupdate = true;
}

void gGUILineGraph::drawGraph() {
	if(needsupdate) {
		updatePoints();
		needsupdate = false;
	}
	if(graphlines.empty()) {
		return;
	}

	gColor* oldcolor = renderer->getColor();

	size_t linecount = graphlines.size();
	for(size_t i = 0; i < linecount; i++) {
		renderer->setColor(linecolors[i % linecolornum]);

		size_t pointcount = graphlines[i].size();
		bool skipped = false;
		for(size_t j = 1; j < pointcount; j++) {
			if(rangeenabled) {
				if(graphlines[i][j][0] < rangestart || graphlines[i][j][0] > rangeend) {
					skipped = true;
					continue;
				}
			}
			//if(pointsenabled) gDrawCircle(graphlines[i][j][2], graphlines[i][j][3], 5, true);
			//if(!skipped) gDrawLine(graphlines[i][j-1][2], graphlines[i][j-1][3], graphlines[i][j][2], graphlines[i][j][3]);
			//skipped = false;

			if(skipped) {
				skipped = false;
				continue;
			}

			if(pointsenabled) {
				size_t index = hash(i, j);
				gCircle* circle = circlesmap[index];
				if (circle != nullptr) {
					circle->draw();
				}
			}

			size_t index = hash(i, j);
			gLine* line = linesmap[index];
			if (line != nullptr) {
				line->draw();
			}
		}
	}

	renderer->setColor(oldcolor);
}

void gGUILineGraph::updatePoints() {
	size_t linecount = graphlines.size();
	size_t points = 0;
	for(size_t i = 0; i < linecount; i++) {
		size_t pointcount = graphlines[i].size();
		for(size_t j = 1; j < pointcount; j++) {
			graphlines[i][j][2] = axisx1 + axisxw * (graphlines[i][j][0] - minx) / (maxx - minx);
			graphlines[i][j][3] = axisy2 - axisyh * (graphlines[i][j][1] - miny) / (maxy - miny);
			points++;
		}
	}

	// resize the cached lines, create or destroy them as necessary
	ssize_t diff = cachedlines.size() - points;
	if (diff > 0) {
		// we have too many lines, so we destroy the last ones
		for (size_t i = 0; i < diff; i++) {
			gLine& line = cachedlines.back();
			line.clear();
			cachedlines.pop_back();
		}
	} else {
		// we need more lines, so we create them
		for (size_t i = 0; i < -diff; i++) {
			cachedlines.emplace_back();
		}
	}

	// update the lines
	linesmap.clear(); // clears the cached index to line pointer map

	auto lineit = cachedlines.begin();
	// iterate over all lines, and update the points
	for(size_t i = 0; i < linecount; i++) {
		size_t pointcount = graphlines[i].size();
		for(size_t j = 1; j < pointcount; j++) {
			if (lineit == cachedlines.end()) {
				// no cached lines left, this should not happen as we have already resized the cache
				// but kept here for safety, it would crash otherwise
				break;
			}
			gLine* line = lineit.base();
			line->setThickness(1.0f);
			line->setPoints(graphlines[i][j-1][2], graphlines[i][j-1][3], graphlines[i][j][2], graphlines[i][j][3]);
			linesmap[hash(i, j)] = line;
			lineit++; // get the next line from the list
		}
	}

	if (!pointsenabled) {
		// clear everything
		for (gCircle& item : cachedcircles) {
			item.clear();
		}
		cachedcircles.clear();
		circlesmap.clear();
		return;
	}

	// resize the cached circles, create or destroy them as necessary
	diff = cachedcircles.size() - points;
	if (diff > 0) {
		// we have too many lines, so we destroy the last ones
		for (size_t i = 0; i < diff; i++) {
			gCircle& circle = cachedcircles.back();
			circle.clear();
			cachedcircles.pop_back();
		}
	} else {
		// we need more circles, so we create them
		for (size_t i = 0; i < -diff; i++) {
			cachedcircles.emplace_back();
		}
	}

	// update the circles
	circlesmap.clear(); // clears the cached index to line pointer map

	auto circleit = cachedcircles.begin();
	// iterate over all circles, and update the circles
	for(size_t i = 0; i < linecount; i++) {
		size_t pointcount = graphlines[i].size();
		for(size_t j = 1; j < pointcount; j++) {
			if (circleit == cachedcircles.end()) {
				// no cached circles left, this should not happen as we have already resized the cache
				// but kept here for safety, it would crash otherwise
				break;
			}
			gCircle* circle = circleit.base();
			circle->setPoints(graphlines[i][j][2], graphlines[i][j][3], 5, true, 64);
			circlesmap[hash(i, j)] = circle;

			circleit++; // get the next circle from the list
		}
	}
}

void gGUILineGraph::removeFirstPointsFromLine(int lineIndex, int pointNumLimit) {
    if (!graphlines.empty()) {
        if (graphlines[lineIndex].size() >= pointNumLimit) {
            for(int i = 0; i < pointNumLimit; i++) {
				graphlines[lineIndex].pop_front();
			}
			needsupdate = true;
        }
    }
}

int gGUILineGraph::getPointNum(int lineIndex) {
	return graphlines[lineIndex].size();
}

float gGUILineGraph::getPointXValue(int lineIndex, int pointIndex) {
	return graphlines[lineIndex][pointIndex][0];
}

float gGUILineGraph::getPointYValue(int lineIndex, int pointIndex) {
	return graphlines[lineIndex][pointIndex][1];
}

void gGUILineGraph::clear() {
	graphlines.clear();
	needsupdate = true;
}
