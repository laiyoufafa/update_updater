/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "updater_main.h"
#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <string>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include "applypatch/partition_record.h"
#include "cert_verify.h"
#include "fs_manager/mount.h"
#include "include/updater/updater.h"
#include "language/language_ui.h"
#include "log/dump.h"
#include "log/log.h"
#include "misc_info/misc_info.h"
#include "package/pkg_manager.h"
#include "pkg_manager.h"
#include "pkg_utils.h"
#include "securec.h"
#include "ui/updater_ui.h"
#include "ui/updater_ui_env.h"
#include "updater/updater_const.h"
#include "updater_ui_facade.h"
#include "updater_ui_tools.h"
#include "utils.h"

namespace Updater {
using Utils::String2Int;
using namespace Hpackage;
using namespace Updater::Utils;
using namespace std::literals::chrono_literals;

constexpr int DISPLAY_TIME = 1000 * 1000;
constexpr struct option OPTIONS[] = {
    { "update_package", required_argument, nullptr, 0 },
    { "retry_count", required_argument, nullptr, 0 },
    { "factory_wipe_data", no_argument, nullptr, 0 },
    { "user_wipe_data", no_argument, nullptr, 0 },
    { nullptr, 0, nullptr, 0 },
};

static void SetRetryCountToMisc(int retryCount, const std::vector<std::string> args)
{
    struct UpdateMessage msg {};
    char buffer[20];
    if (strncpy_s(msg.command, sizeof(msg.command), "boot_updater", strlen("boot_updater") + 1) != EOK) {
        LOG(ERROR) << "SetRetryCountToMisc strncpy_s failed";
        return;
    }
    for (const auto& arg : args) {
        if (arg.find("--retry_count") == std::string::npos) {
            if (strncat_s(msg.update, sizeof(msg.update), arg.c_str(), strlen(arg.c_str()) + 1) != EOK) {
                LOG(ERROR) << "SetRetryCountToMisc strncat_s failed";
                return;
            }
            if (strncat_s(msg.update, sizeof(msg.update), "\n", strlen("\n") + 1) != EOK) {
                LOG(ERROR) << "SetRetryCountToMisc strncat_s failed";
                return;
            }
        }
    }
    if (snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, "--retry_count=%d", retryCount) == -1) {
        LOG(ERROR) << "SetRetryCountToMisc snprintf_s failed";
        return;
    }
    if (strncat_s(msg.update, sizeof(msg.update), buffer, strlen(buffer) + 1) != EOK) {
        LOG(ERROR) << "SetRetryCountToMisc strncat_s failed";
        return;
    }
    if (WriteUpdaterMiscMsg(msg) != true) {
        LOG(ERROR) << "Write command to misc failed.";
    }
}

static int DoFactoryReset(FactoryResetMode mode, const std::string &path)
{
    if (mode == USER_WIPE_DATA) {
        STAGE(UPDATE_STAGE_BEGIN) << "User FactoryReset";
        LOG(INFO) << "Begin erasing /data";
        if (FormatPartition(path, true) != 0) {
            LOG(ERROR) << "User level FactoryReset failed";
            STAGE(UPDATE_STAGE_FAIL) << "User FactoryReset";
            ERROR_CODE(CODE_FACTORY_RESET_FAIL);
            return 1;
        }
        LOG(INFO) << "User level FactoryReset success";
        STAGE(UPDATE_STAGE_SUCCESS) << "User FactoryReset";
    }
    return 0;
}

int FactoryReset(FactoryResetMode mode, const std::string &path)
{
    return DoFactoryReset(mode, path);
}

UpdaterStatus UpdaterFromSdcard()
{
#ifndef UPDATER_UT
    // sdcard fsType only support ext4/vfat
    std::string sdcardStr = GetBlockDeviceByMountPoint(SDCARD_PATH);
    if (!IsSDCardExist(sdcardStr)) {
        UpdaterUiFacade::GetInstance().ShowLog(
            (errno == ENOENT) ? TR(LOG_SDCARD_NOTFIND) : TR(LOG_SDCARD_ABNORMAL), true);
        return UPDATE_ERROR;
    }
    if (MountForPath(SDCARD_PATH) != 0) {
        int ret = mount(sdcardStr.c_str(), SDCARD_PATH, "vfat", 0, NULL);
        if (ret != 0) {
            LOG(WARNING) << "MountForPath /sdcard failed!";
            return UPDATE_ERROR;
        }
    }
#endif
    if (access(SDCARD_CARD_PKG_PATH, 0) != 0) {
        LOG(ERROR) << "package is not exist";
        UpdaterUiFacade::GetInstance().ShowLog(TR(LOG_NOPKG), true);
        return UPDATE_ERROR;
    }
    PkgManager::PkgManagerPtr pkgManager = PkgManager::GetPackageInstance();
    if (pkgManager == nullptr) {
        LOG(ERROR) << "pkgManager is nullptr";
        return UPDATE_ERROR;
    }

    STAGE(UPDATE_STAGE_BEGIN) << "UpdaterFromSdcard";
    LOG(INFO) << "UpdaterFromSdcard start, sdcard updaterPath : " << SDCARD_CARD_PKG_PATH;

    UpdaterUiFacade::GetInstance().ShowLog(TR(LOG_SDCARD_NOTMOVE));
    UpdaterStatus updateRet = DoInstallUpdaterPackage(pkgManager, SDCARD_CARD_PKG_PATH, 0, SDCARD_UPDATE);
    if (updateRet != UPDATE_SUCCESS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(UI_SHOW_DURATION));
        UpdaterUiFacade::GetInstance().ShowLog(TR(LOG_SDCARD_FAIL));
        STAGE(UPDATE_STAGE_FAIL) << "UpdaterFromSdcard failed";
    } else {
        LOG(INFO) << "Update from SD Card successfully!";
        STAGE(UPDATE_STAGE_SUCCESS) << "UpdaterFromSdcard success";
    }
    PkgManager::ReleasePackageInstance(pkgManager);
    return updateRet;
}

bool GetBatteryCapacity(int &capacity)
{
    const static std::vector<const char *> vec = {
        "/sys/class/power_supply/battery/capacity",
        "/sys/class/power_supply/Battery/capacity"
    };
    for (auto &it : vec) {
        std::ifstream ifs { it };
        if (!ifs.is_open()) {
            continue;
        }

        int tmpCapacity = 0;
        ifs >> tmpCapacity;
        if ((ifs.fail()) || (ifs.bad())) {
            continue;
        }

        capacity = tmpCapacity;
        return true;
    }

    return false;
}

bool IsBatteryCapacitySufficient()
{
    static constexpr auto levelIdx = "lowBatteryLevel";
    static constexpr auto jsonPath = "/etc/product_cfg.json";

    int capacity = 0;
    bool ret = GetBatteryCapacity(capacity);
    if (!ret) {
        return true; /* maybe no battery or err value return default true */
    }

    JsonNode node { Fs::path { jsonPath }};
    auto item = node[levelIdx].As<int>();
    if (!item.has_value()) {
        return true; /* maybe no value return default true */
    }

    int lowLevel = *item;
    if (lowLevel > 100 || lowLevel < 0) { /* full percent is 100 */
        LOG(ERROR) << "load battery level error:" << lowLevel;
        return false; /* config err not allow to update */
    }

    LOG(INFO) << "current capacity:" << capacity << ", low level:" << lowLevel;

    return capacity > lowLevel;
}

static UpdaterStatus InstallUpdaterPackage(UpdaterParams &upParams, const std::vector<std::string> &args,
    PkgManager::PkgManagerPtr manager)
{
    UPDATER_INIT_RECORD;
    UpdaterStatus status = UPDATE_UNKNOWN;
    if (IsBatteryCapacitySufficient() == false) {
        UpdaterUiFacade::GetInstance().ShowUpdInfo(TR(LOG_LOWPOWER));
        std::this_thread::sleep_for(std::chrono::milliseconds(UI_SHOW_DURATION));
        UPDATER_LAST_WORD(UPDATE_ERROR);
        LOG(ERROR) << "Battery is not sufficient for install package.";
        status = UPDATE_SKIP;
    } else {
        STAGE(UPDATE_STAGE_BEGIN) << "Install package";
        if (upParams.retryCount == 0) {
            // First time enter updater, record retryCount in case of abnormal reset.
            if (!PartitionRecord::GetInstance().ClearRecordPartitionOffset()) {
                LOG(ERROR) << "ClearRecordPartitionOffset failed";
                return UPDATE_ERROR;
            }
            SetRetryCountToMisc(upParams.retryCount + 1, args);
        }
        if (SetupPartitions() != 0) {
            UpdaterUiFacade::GetInstance().ShowUpdInfo(TR(UPD_SETPART_FAIL), true);
            return UPDATE_ERROR;
        }
        status = DoInstallUpdaterPackage(manager, upParams.updatePackage, upParams.retryCount, HOTA_UPDATE);
        if (status != UPDATE_SUCCESS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(UI_SHOW_DURATION));
            UpdaterUiFacade::GetInstance().ShowLog(TR(LOG_UPDFAIL));
            STAGE(UPDATE_STAGE_FAIL) << "Install failed";
            if (status == UPDATE_RETRY && upParams.retryCount < MAX_RETRY_COUNT) {
                upParams.retryCount += 1;
                UpdaterUiFacade::GetInstance().ShowFailedPage();
                SetRetryCountToMisc(upParams.retryCount, args);
                Utils::DoReboot("updater");
            }
        } else {
            LOG(INFO) << "Install package success.";
            STAGE(UPDATE_STAGE_SUCCESS) << "Install package";
            UpdaterUiFacade::GetInstance().ShowSuccessPage();
        }
    }
    return status;
}

static UpdaterStatus StartUpdaterEntry(PkgManager::PkgManagerPtr manager,
    const std::vector<std::string> &args, UpdaterParams &upParams)
{
    UpdaterStatus status = UPDATE_UNKNOWN;
    if (upParams.updatePackage != "") {
        UpdaterUiFacade::GetInstance().ShowProgressPage();
        status = InstallUpdaterPackage(upParams, args, manager);
        if (status != UPDATE_SUCCESS) {
            if (!CheckDumpResult()) {
                UPDATER_LAST_WORD(status);
            }
            return status;
        }
        WriteDumpResult("pass");
    } else if (upParams.factoryWipeData) {
        UpdaterUiFacade::GetInstance().ShowProgressPage();
        LOG(INFO) << "Factory level FactoryReset begin";
        status = UPDATE_SUCCESS;
        DoProgress();
        if (FactoryReset(FACTORY_WIPE_DATA, "/data") != 0) {
            LOG(ERROR) << "FactoryReset factory level failed";
            status = UPDATE_ERROR;
        }
        UpdaterUiFacade::GetInstance().ShowLogRes(
            (status != UPDATE_SUCCESS) ? TR(LOGRES_FACTORY_FAIL) : TR(LOGRES_FACTORY_DONE));
    } else if (upParams.userWipeData) {
        UpdaterUiFacade::GetInstance().ShowProgressPage();
        LOG(INFO) << "User level FactoryReset begin";
        status = UPDATE_SUCCESS;
        DoProgress();
        if (FactoryReset(USER_WIPE_DATA, "/data") != 0) {
            LOG(ERROR) << "FactoryReset user level failed";
            status = UPDATE_ERROR;
        }
        if (status != UPDATE_SUCCESS) {
            UpdaterUiFacade::GetInstance().ShowLogRes(TR(LOGRES_WIPE_FAIL));
        } else {
            UpdaterUiFacade::GetInstance().ShowSuccessPage();
            UpdaterUiFacade::GetInstance().ShowLogRes(TR(LOGRES_WIPE_FINISH));
            PostUpdater(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(UI_SHOW_DURATION));
        }
    }
    return status;
}

static UpdaterStatus StartUpdater(PkgManager::PkgManagerPtr manager, const std::vector<std::string> &args,
    char **argv, PackageUpdateMode &mode)
{
    UpdaterParams upParams {
        false, false, 0, ""
    };
    std::vector<char *> extractedArgs;
    int rc;
    int optionIndex;

    for (const auto &arg : args) {
        extractedArgs.push_back(const_cast<char *>(arg.c_str()));
    }
    extractedArgs.push_back(nullptr);
    extractedArgs.insert(extractedArgs.begin(), argv[0]);
    while ((rc = getopt_long(extractedArgs.size() - 1, extractedArgs.data(), "", OPTIONS, &optionIndex)) != -1) {
        switch (rc) {
            case 0: {
                std::string option = OPTIONS[optionIndex].name;
                if (option == "update_package") {
                    upParams.updatePackage = optarg;
                    (void)UpdaterUiFacade::GetInstance().SetMode(UpdaterMode::OTA);
                    mode = HOTA_UPDATE;
                } else if (option == "retry_count") {
                    upParams.retryCount = atoi(optarg);
                    (void)UpdaterUiFacade::GetInstance().SetMode(UpdaterMode::OTA);
                    mode = HOTA_UPDATE;
                } else if (option == "factory_wipe_data") {
                    (void)UpdaterUiFacade::GetInstance().SetMode(UpdaterMode::REBOOTFACTORYRST);
                    upParams.factoryWipeData = true;
                } else if (option == "user_wipe_data") {
                    (void)UpdaterUiFacade::GetInstance().SetMode(UpdaterMode::REBOOTFACTORYRST);
                    upParams.userWipeData = true;
                }
                break;
            }
            case '?':
                LOG(ERROR) << "Invalid argument.";
                break;
            default:
                LOG(ERROR) << "Invalid argument.";
                break;
        }
    }
    optind = 1;
    // Sanity checks
    if (upParams.factoryWipeData && upParams.userWipeData) {
        LOG(WARNING) << "Factory level reset and user level reset both set. use user level reset.";
        upParams.factoryWipeData = false;
    }
    return StartUpdaterEntry(manager, args, upParams);
}

int UpdaterMain(int argc, char **argv)
{
    UpdaterStatus status = UPDATE_UNKNOWN;
    PkgManager::PkgManagerPtr manager = PkgManager::GetPackageInstance();
    UpdaterInit::GetInstance().InvokeEvent(UPDATER_PRE_INIT_EVENT);
    Dump::GetInstance().RegisterDump("DumpHelperLog", std::make_unique<DumpHelperLog>());
    CertVerify::GetInstance().RegisterCertHelper(std::make_unique<SingleCertHelper>());
    std::vector<std::string> args = ParseParams(argc, argv);

    LOG(INFO) << "Ready to start";
#ifndef UPDATER_UT
    UpdaterUiEnv::Init();
#endif
    UpdaterInit::GetInstance().InvokeEvent(UPDATER_INIT_EVENT);
    PackageUpdateMode mode = UNKNOWN_UPDATE;
    status = StartUpdater(manager, args, argv, mode);
    std::this_thread::sleep_for(std::chrono::milliseconds(UI_SHOW_DURATION));
#ifndef UPDATER_UT
    if (status != UPDATE_SUCCESS && status != UPDATE_SKIP) {
        if (mode == HOTA_UPDATE) {
            UpdaterUiFacade::GetInstance().ShowFailedPage();
        } else {
            UpdaterUiFacade::GetInstance().ShowMainpage();
            std::this_thread::sleep_for(50ms); /* wait for page flush 50ms */
            UpdaterUiTools::SaveUxBuffToFile("/tmp/mainpage.png");
        }
        // Wait for user input
        while (true) {
            Utils::UsSleep(DISPLAY_TIME);
        }
        return 0;
    }
#endif
    PostUpdater(true);
    Utils::DoReboot("");
    return 0;
}
} // Updater
