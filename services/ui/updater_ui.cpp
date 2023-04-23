/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "updater_ui.h"
#include <mutex>
#include <thread>
#include "callback_manager.h"
#include "language/language_ui.h"
#include "log/log.h"
#include "page/page_manager.h"
#include "scope_guard.h"
#include "updater_main.h"
#include "updater_ui_facade.h"
#include "utils.h"

namespace Updater {
namespace {
constexpr uint32_t DISPLAY_TIME = 1 * 1000 * 1000; /* 1s */
constexpr uint32_t FAIL_DELAY = 5 * 1000 * 1000;
constexpr uint32_t SUCCESS_DELAY = 3 * 1000 * 1000;
constexpr int CALLBACK_DELAY = 20 * 1000; /* 20ms */
bool g_timerStopped { false };

inline auto &GetFacade()
{
    return UpdaterUiFacade::GetInstance();
}
}  // namespace

std::ostream &operator<<(std::ostream &os, const ComInfo &com)
{
    os << "pageId: " << com.pageId << " comId: " << com.comId;
    return os;
}

void DoProgress()
{
    constexpr int maxSleepMs = 1000 * 1000;
    constexpr int minSleepMs = 3000;
    constexpr float ratio = 10.0;
    // if 100 as fullpercent, then 0.3 per step
    constexpr int progressValueStep = static_cast<int>(0.3 * ratio);
    constexpr int maxProgressValue = static_cast<int>(100 * ratio);
    int progressvalueTmp = 0;
    if (GetFacade().GetMode() != UpdaterMode::FACTORYRST && GetFacade().GetMode() != UpdaterMode::REBOOTFACTORYRST) {
        return;
    }
    GetFacade().ShowProgress(0);
    while (progressvalueTmp <= maxProgressValue) {
        progressvalueTmp = progressvalueTmp + progressValueStep;
        GetFacade().ShowProgress(progressvalueTmp / ratio);
        Utils::UsSleep(minSleepMs);
        if (progressvalueTmp >= maxProgressValue) {
            Utils::UsSleep(maxSleepMs);
            return;
        }
    }
}

DEFINE_ASYN_CALLBACK(OnRebootEvt)
{
    LOG(INFO) << "On Label Reboot";
    PostUpdater(false);
    Utils::DoReboot("");
}

DEFINE_SYNC_CALLBACK(OnLabelResetEvt)
{
    LOG(INFO) << "On Label Reset";
    if (!GetFacade().SetMode(UpdaterMode::FACTORYRST)) {
        return;
    }
    GetFacade().ShowFactoryConfirmPage();
}

DEFINE_ASYN_CALLBACK(OnLabelSDCardEvt)
{
    LOG(INFO) << "On Label SDCard";
    if (!GetFacade().SetMode(UpdaterMode::SDCARD)) {
        return;
    }
    Utils::UsSleep(CALLBACK_DELAY);
    GetFacade().ClearText();
    GetFacade().ShowProgress(0);
    GetFacade().ShowLog(TR(LOG_SDCARD_NOTMOVE));
    Utils::UsSleep(DISPLAY_TIME);
    UpdaterParams upParams;
    upParams.sdcardUpdate = true;
    if (UpdaterFromSdcard(upParams) != UPDATE_SUCCESS) {
        GetFacade().ShowMainpage();
        return;
    }
    PostUpdater(true);
    Utils::DoReboot("");
}

DEFINE_ASYN_CALLBACK(OnLabelSDCardNoDelayEvt)
{
    LOG(INFO) << "On Label SDCard";
    if (!GetFacade().SetMode(UpdaterMode::SDCARD)) {
        return;
    }
    Utils::UsSleep(CALLBACK_DELAY);
    UpdaterParams upParams;
    upParams.sdcardUpdate = true;
    if (auto res = UpdaterFromSdcard(upParams); res != UPDATE_SUCCESS) {
        GetFacade().ShowLogRes(res == UPDATE_CORRUPT ? TR(LOGRES_VERIFY_FAILED) : TR(LOGRES_UPDATE_FAILED));
        GetFacade().ShowFailedPage();
        Utils::UsSleep(FAIL_DELAY);
        GetFacade().ShowMainpage();
        return;
    }
    GetFacade().ShowLogRes(TR(LABEL_UPD_OK_DONE));
    GetFacade().ShowSuccessPage();
    Utils::UsSleep(SUCCESS_DELAY);
    PostUpdater(true);
    Utils::DoReboot("");
}

DEFINE_SYNC_CALLBACK(OnLabelCancelEvt)
{
    LOG(INFO) << "On Label Cancel";
    PageManager::GetInstance().GoBack();
}

DEFINE_ASYN_CALLBACK(OnLabelOkEvt)
{
    LOG(INFO) << "On Label Ok";
    Utils::UsSleep(CALLBACK_DELAY);
    GetFacade().ShowMainpage();
    GetFacade().ClearText();
    GetFacade().ShowLog(TR(LOG_WIPE_DATA));
    if (!GetFacade().SetMode(UpdaterMode::FACTORYRST)) {
        return;
    }
    GetFacade().ShowProgress(0);
    GetFacade().ShowProgressPage();
    DoProgress();
    if (FactoryReset(USER_WIPE_DATA, "/data") == 0) {
        GetFacade().ShowLog(TR(LOG_WIPE_DONE));
        GetFacade().ShowSuccessPage();
    } else {
        GetFacade().ShowLog(TR(LOG_WIPE_FAIL));
        GetFacade().ShowFailedPage();
    }
}

DEFINE_ASYN_CALLBACK(OnConfirmRstEvt)
{
    LOG(INFO) << "On Label Ok";
    if (!GetFacade().SetMode(UpdaterMode::FACTORYRST)) {
        return;
    }
    GetFacade().ShowUpdInfo(TR(LABEL_RESET_PROGRESS_INFO));
    GetFacade().ShowProgressPage();
    DoProgress();
    if (FactoryReset(USER_WIPE_DATA, "/data") != 0) {
        GetFacade().ShowLogRes(TR(LOG_WIPE_FAIL));
        GetFacade().ShowFailedPage();
        Utils::UsSleep(FAIL_DELAY);
        GetFacade().ShowMainpage();
    } else {
        GetFacade().ShowUpdInfo(TR(LOGRES_WIPE_FINISH));
        Utils::UsSleep(DISPLAY_TIME);
        GetFacade().ShowSuccessPage();
    }
}

DEFINE_ASYN_CALLBACK(OnMenuShutdownEvt)
{
    LOG(DEBUG) << "shutdown";
    Utils::DoShutdown();
}

DEFINE_ASYN_CALLBACK(OnMenuClearCacheEvt)
{
    LOG(INFO) << "On clear cache";
    GetFacade().ClearText();
    if (!GetFacade().SetMode(UpdaterMode::FACTORYRST)) {
        return;
    }
    Utils::UsSleep(CALLBACK_DELAY);
    GetFacade().ShowUpdInfo(TR(LOG_CLEAR_CAHCE));
    GetFacade().ShowProgressPage();
    ClearMisc();
    DoProgress();
    GetFacade().ShowMainpage();
}

void StartLongPressTimer()
{
    static int downCount { 0 };
    if (!GetFacade().IsInProgress()) {
        return;
    }
    ++downCount;
    g_timerStopped = false;
    using namespace std::literals::chrono_literals;
    std::thread t { [lastdownCount = downCount] () {
        constexpr auto threshold = 2s;
        std::this_thread::sleep_for(threshold);
        /**
         * When the downCount of the last power key press changes,
         * it means that the last press has been released before
         * the timeout, then you can exit the callback directly
         */
        if (g_timerStopped || lastdownCount != downCount) {
            return;
        }
        // show warning
        GetFacade().ShowProgressWarning(true);
    }};
    t.detach();
}

void StopLongPressTimer()
{
    // no need to judge whether in progress page,
    // because may press power key in progress
    // page and release power key in other page
    g_timerStopped = true;
    // hide warning
    GetFacade().ShowProgressWarning(false);
}
} // namespace Updater
