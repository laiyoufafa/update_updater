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

#include "updater_ui_config.h"

#include "common/screen.h"
#include "control/callback_manager.h"
#include "language/language_ui.h"
#include "layout/layout_parser.h"
#include "log/log.h"
#include "utils.h"

namespace Updater {
PagePath UpdaterUiConfig::pagePath_;
std::vector<UxPageInfo> UpdaterUiConfig::pageInfos_;
bool UpdaterUiConfig::isFocusEnable_;
constexpr auto UI_CFG_FILE = "/resources/pages/config.json";
constexpr auto UI_CFG_KEY = "config";
constexpr auto WIDTH_KEY = "screenWidth";
constexpr auto HEIGHT_KEY = "screenHeight";
constexpr auto FOCUS_CFG_FIELD = "enableFoucs";
namespace Fs = std::filesystem;
namespace {
bool CanonicalPagePath(PagePath &pagePath)
{
    std::string realPath {};
    if (bool res = Utils::PathToRealPath(pagePath.dir, realPath); res) {
        pagePath.dir = std::move(realPath);
        for (auto &file : pagePath.pages) {
            file = pagePath.dir + "/" + file;
        }
        return true;
    }
    LOG(ERROR) << "page path canonical failed, please check your config";
    return false;
}

std::ostream &operator<<(std::ostream &os, const UxViewCommonInfo &info)
{
    os << info.x << " " << info.y << " " << info.w << " " << info.h << " ";
    os << info.id << " " << info.parent << " " << info.type << " " << info.visible;
    return os;
}

std::ostream &operator<<(std::ostream &os, const UxPageInfo &info)
{
    LOG(INFO) << "page:" << info.id;
    for (auto &it : info.viewInfos) {
        LOG(INFO) << it.commonInfo;
    }
    return os;
}

void PrintInfoVec(const std::vector<UxPageInfo> &infoVec)
{
    LOG(INFO) << "=====print start=====";
    for (auto &iter : infoVec) {
        LOG(INFO) << iter;
    }
    LOG(INFO) << "=====print end=====";
}

std::string SelectConfig(const JsonNode &node)
{
    using namespace OHOS;
    for (const auto &iter : node) {
        const JsonNode &subCfgPathNode = iter.get();
        auto optStr = subCfgPathNode.As<std::string>();
        if (!optStr.has_value()) {
            LOG(ERROR) << "config array's element should be string";
            return "";
        }
        std::string subConfigPath = *optStr;
        const JsonNode &subCfg = JsonNode { Fs::path { subConfigPath }};
        auto screenW = subCfg[WIDTH_KEY].As<int16_t>();
        auto screenH = subCfg[HEIGHT_KEY].As<int16_t>();
        if (!screenW.has_value() || !screenH.has_value()) {
            LOG(ERROR) << "real config file should has screenW and screenH key";
            return "";
        }
        if (screenW != Screen::GetInstance().GetWidth() || screenH != Screen::GetInstance().GetHeight()) {
            LOG(INFO) << "screen size not matched" << subConfigPath;
            continue;
        }
        LOG(INFO) << "select config: " << subConfigPath;
        return subConfigPath;
    }
    LOG(ERROR) << "no config matched";
    return "";
}
}

bool UpdaterUiConfig::Init()
{
    JsonNode node { Fs::path { UI_CFG_FILE }};
    const JsonNode &cfgNode = node[UI_CFG_KEY];
    switch (cfgNode.Type()) {
        case NodeType::STRING: {
            auto optString = cfgNode.As<std::string>();
            if (!optString.has_value()) {
                LOG(ERROR) << "config path should be string";
                break;
            }
            JsonNode realNode { Fs::path { *optString }};
            return Init(realNode);
        }
        case NodeType::ARRAY: {
            std::string realConfig = SelectConfig(cfgNode);
            if (realConfig.empty()) {
                break;
            }
            JsonNode realNode { Fs::path { realConfig }};
            return Init(realNode);
        }
        default:
            break;
    }
    LOG(ERROR) << "config file parse failed: " << UI_CFG_FILE;
    return false;
}

bool UpdaterUiConfig::Init(const JsonNode &node)
{
    return LoadLayout(node) && LoadLangRes(node) && LoadStrategy(node) && LoadCallbacks(node) && LoadFocusCfg(node);
}

std::string_view UpdaterUiConfig::GetMainPage()
{
    return pagePath_.entry;
}

std::unordered_map<UpdaterMode, UiStrategyCfg> &UpdaterUiConfig::GetStrategy()
{
    return UiStrategy::GetStrategy();
}

std::vector<UxPageInfo> &UpdaterUiConfig::GetPageInfos()
{
    return pageInfos_;
}

bool UpdaterUiConfig::GetFocusCfg()
{
    return isFocusEnable_;
}

bool UpdaterUiConfig::LoadLayout(const JsonNode &node)
{
    if (!Visit<SETVAL>(node, pagePath_)) {
        LOG(ERROR) << "parse page path error: " << pagePath_.dir;
        return false;
    }

    if (!CanonicalPagePath(pagePath_)) {
        return false;
    }

    if (!LayoutParser::GetInstance().LoadLayout(pagePath_.pages, pageInfos_)) {
        LOG(ERROR) << "load layout error: " << UI_CFG_FILE;
        return false;
    }
    PrintInfoVec(pageInfos_);
    return true;
}

bool UpdaterUiConfig::LoadLangRes(const JsonNode &node)
{
    return Lang::LanguageUI::GetInstance().LoadLangRes(node);
}

bool UpdaterUiConfig::LoadStrategy(const JsonNode &node)
{
    return UiStrategy::LoadStrategy(node);
}

bool UpdaterUiConfig::LoadCallbacks(const JsonNode &node)
{
    return CallbackManager::LoadCallbacks(node);
}
bool UpdaterUiConfig::LoadFocusCfg(const JsonNode &node)
{
    // disable focus by default
    isFocusEnable_ = node[FOCUS_CFG_FIELD].As<bool>().value_or(false);
    return true;
}
}