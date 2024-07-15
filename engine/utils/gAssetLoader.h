/*
 * Copyright (C) 2016 Nitra Games Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTILS_GASSETLOADER_H_
#define UTILS_GASSETLOADER_H_

#include "gThread.h"
#include <list>
#include <map>
#include <utility>

class gAssetLoader : public gThread {
public:
	gAssetLoader();
	virtual ~gAssetLoader();

	void submit(std::function<void()> async_caller, std::function<void()> sync_caller) {
		std::unique_lock<std::mutex> lock(mutex);
		queue.emplace_back(std::make_unique<QueuedAsset>(async_caller, sync_caller, LOADSTATE_ASYNC_WAIT));
	}

	template<typename T, typename... Args>
	void submit(T& asset, Args&&... args) {
		submit([&asset, args...]() {
			asset.loadAsync(std::forward<Args>(args)...);
		}, [&asset]() {
			asset.loadSync();
		});
	}

	void reset();

	int getLoadedAssetCount() const {
		return loaded_count;
	}

	void update();

	void threadFunction() override;

private:
	enum LoadState {
		LOADSTATE_ASYNC_WAIT,
		LOADSTATE_ASYNC_LOADING,
		LOADSTATE_SYNC_WAIT,
		LOADSTATE_SYNC_LOADING,
		LOADSTATE_DONE,
	};
	struct QueuedAsset {
		std::function<void()> sync_caller;
		std::function<void()> async_caller;
		LoadState state;

		QueuedAsset(std::function<void()> async_caller, std::function<void()> sync_caller, LoadState state)
			: async_caller(std::move(async_caller)), sync_caller(std::move(sync_caller)), state(state) {
		}
	};

	int loaded_count;
	std::list<std::unique_ptr<QueuedAsset>> queue;
	bool needs_update;
	bool running;

};

#endif /* UTILS_GASSETLOADER_H_ */
