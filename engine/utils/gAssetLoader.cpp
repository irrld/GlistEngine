/*
 * gAssetLoader.cpp
 *
 *  Created on: May 11, 2021
 *      Author: noyan
 */

#include "gAssetLoader.h"


gAssetLoader::gAssetLoader() {
	needs_update = false;
	running = true;
	start();
}

gAssetLoader::~gAssetLoader() {
	running = false;
	stop();
}

void gAssetLoader::threadFunction() {
	while (running) {
		std::unique_lock<std::mutex> lock(mutex);
		if(queue.empty()) {
			continue;
		}
		for (std::unique_ptr<QueuedAsset>& asset : queue) {
			if (asset->state == LOADSTATE_ASYNC_WAIT) {
				asset->state = LOADSTATE_ASYNC_LOADING;
				asset->async_caller();
				asset->state = LOADSTATE_SYNC_WAIT;
				needs_update = true;
			}
		}
		sleep(std::chrono::milliseconds(5));
	}
}

void gAssetLoader::update() {
	std::unique_lock<std::mutex> lock(mutex);
	if(queue.empty()) {
		return;
	}
	bool has_completed = false;
	for (std::unique_ptr<QueuedAsset>& asset : queue) {
		if (asset->state == LOADSTATE_SYNC_WAIT) {
			asset->state = LOADSTATE_SYNC_LOADING;
			asset->sync_caller();
			asset->state = LOADSTATE_DONE;
			has_completed = true;
			loaded_count++;
		}
	}
	if (has_completed) {
		queue.remove_if([](std::unique_ptr<QueuedAsset>& asset) {
			return asset->state == LOADSTATE_DONE;
		});
	}
}


void gAssetLoader::reset() {
	queue.clear();
	needs_update = false;
}



