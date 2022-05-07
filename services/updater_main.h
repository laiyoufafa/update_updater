/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef UPDATER_MAIN_H
#define UPDATER_MAIN_H

#include <iostream>
#include <string>
#include <vector>
#include "macros.h"
#include "updater/updater.h"

namespace updater {
enum UpdaterInitEvent {
    UPDATER_PRE_INIT_EVENT = 0,
    UPDATER_INIT_EVENT,
    UPDATER_POST_INIT_EVENT,

    UPDATER_INIT_EVENT_BUTT
};

using InitHandler = void (*)(void);

class UpdaterInit {
    DEFINE_COMMON_FOR_SINGLETON(UpdaterInit)
public:
    void InvokeEvent(enum UpdaterInitEvent eventId)
    {
        if (eventId >= UPDATER_INIT_EVENT_BUTT) {
            return;
        }
        for (const auto &handler : initEvent_[eventId]) {
            if (handler != nullptr) {
                handler();
            }
        }
    }
    void SubscribeEvent(enum UpdaterInitEvent eventId, InitHandler handler)
    {
        if (eventId < UPDATER_INIT_EVENT_BUTT) {
            initEvent_[eventId].push_back(handler);
        }
    }
private:
    UpdaterInit() = default;
    ~UpdaterInit() = default;
    std::vector<InitHandler> initEvent_[UPDATER_INIT_EVENT_BUTT];
};

enum FactoryResetMode {
    USER_WIPE_DATA = 0,
    FACTORY_WIPE_DATA,
};

struct UpdaterParams {
    bool factoryWipeData;
    bool userWipeData;
    int retryCount;
    std::string updatePackage;
};

int UpdaterMain(int argc, char **argv);

int FactoryReset(FactoryResetMode mode, const std::string &path);

UpdaterStatus UpdaterFromSdcard();

bool IsBatteryCapacitySufficient();
} // namespace updater
#endif // UPDATER_MAIN_H
