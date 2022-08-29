/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "box_progress_adapter.h"
#include "graphic_engine.h"
#include "log/log.h"
#include "scope_guard.h"
#include "updater/updater_const.h"

namespace Updater {
BoxProgressAdapter::BoxProgressAdapter(const UxViewInfo &info)
{
    const UxViewCommonInfo *commonPtr = &info.commonInfo;
    const UxBoxProgressInfo *specPtr = &std::get<UxBoxProgressInfo>(info.specificInfo);
    viewId_ = commonPtr->id;
    this->SetPosition(commonPtr->x, commonPtr->y, commonPtr->w, commonPtr->h);
    this->SetVisible(commonPtr->visible);
    this->SetViewId(viewId_.c_str());
    this->SetValue(specPtr->defaultValue);

    auto bgColor = StrToColor(specPtr->bgColor);
    this->SetBackgroundStyle(OHOS::STYLE_BACKGROUND_COLOR, bgColor.full);
    this->SetBackgroundStyle(OHOS::STYLE_BACKGROUND_OPA, bgColor.alpha);

    auto fgColor = StrToColor(specPtr->fgColor);
    this->SetForegroundStyle(OHOS::STYLE_BACKGROUND_COLOR, fgColor.full);
    this->SetForegroundStyle(OHOS::STYLE_BACKGROUND_OPA, fgColor.alpha);
    this->SetRange(progressWidth_ - 1, 0);
    hasEp_ = specPtr->hasEp;
    epId_ = specPtr->endPoint;
}

bool BoxProgressAdapter::IsValid(const UxBoxProgressInfo &info)
{
    if (info.defaultValue > MAX_PROGRESS_VALUE) {
        LOG(ERROR) << "progress viewinfo check failed, defaultValue: " << info.defaultValue;
        return false;
    }

    if (!CheckColor(info.bgColor) || !CheckColor(info.fgColor)) {
        LOG(ERROR) << "progress viewinfo check failed, bgColor:" << info.bgColor <<
            " fgColor:" << info.fgColor;
        return false;
    }

    return true;
}

void BoxProgressAdapter::SetValue(float value)
{
    ON_SCOPE_EXIT(flush) {
        GraphicEngine::GetInstance().Flush();
    };
    OHOS::UIBoxProgress::SetValue(static_cast<int>((value / FULL_PERCENT_PROGRESS) * (progressWidth_ - 1)));
    if (!hasEp_ || ep_ == nullptr) {
        return;
    }
    auto pos = GetPosOfEp();
    ep_->SetVisible(false);
    ep_->SetPosition(pos.x, pos.y);
    ep_->SetVisible(true);
}

bool BoxProgressAdapter::InitEp()
{
    if (!hasEp_) {
        return true;
    }
    if (this->GetParent() == nullptr) {
        LOG(ERROR) << "box progress's parent is nullptr";
        return false;
    }
    auto child = this->GetParent()->GetChildById(epId_.c_str());
    if (child == nullptr || child->GetViewType() != OHOS::UI_IMAGE_VIEW) {
        LOG(ERROR) << "box progress has not an end point children or is not a img view";
        return false;
    }
    ep_ = static_cast<ImgViewAdapter *>(child);
    return true;
}

OHOS::Point BoxProgressAdapter::GetPosOfEp()
{
    float rate = static_cast<float>(GetValue()) / (progressWidth_ - 1);
    constexpr float halfDivisor = 2.0;
    return OHOS::Point {
        static_cast<int16_t>(GetX() - ep_->GetWidth() / halfDivisor + GetWidth() * rate),
        static_cast<int16_t>(GetY() - ep_->GetHeight() / 2 + GetHeight() / 2)
    };
}

void BoxProgressAdapter::SetVisible(bool isVisible)
{
    OHOS::UIBoxProgress::SetVisible(isVisible);
    if (!hasEp_ || ep_ == nullptr) {
        return;
    }
    isVisible ?  ep_->Start() : ep_->Stop();
    ep_->SetVisible(isVisible);
    ep_->Invalidate();
}
} // namespace Updater