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
#include "language/language_ui.h"
#include "log/log.h"
#include "text_label_adapter.h"
#include "updater_ui_const.h"

namespace Updater {
TextLabelAdapter::TextLabelAdapter(const UxViewInfo &info)
{
    const UxViewCommonInfo *commonPtr = &info.commonInfo;
    const UxLabelInfo *specPtr = &std::get<UxLabelInfo>(info.specificInfo);
    viewId_ = commonPtr->id;
    this->SetPosition(commonPtr->x, commonPtr->y, commonPtr->w, commonPtr->h);
    this->SetVisible(commonPtr->visible);
    this->SetViewId(viewId_.c_str());
    this->SetAlign(GetAlign(specPtr->align), OHOS::TEXT_ALIGNMENT_CENTER);
    this->SetText(TranslateText(specPtr->text).c_str());
    this->SetFont(DEFAULT_FONT_FILENAME, specPtr->fontSize);
    auto fontColor = StrToColor(specPtr->fontColor);
    this->SetStyle(OHOS::STYLE_TEXT_COLOR, fontColor.full);
    this->SetStyle(OHOS::STYLE_TEXT_OPA, fontColor.alpha);

    auto bgColor = StrToColor(specPtr->bgColor);
    this->SetStyle(OHOS::STYLE_BACKGROUND_COLOR, bgColor.full);
    this->SetStyle(OHOS::STYLE_BACKGROUND_OPA, bgColor.alpha);

}

bool TextLabelAdapter::IsValid(const UxLabelInfo &info)
{
    if (info.fontSize > MAX_FONT_SIZE) {
        LOG(ERROR) << "label viewinfo check failed, fontSize: " << info.fontSize;
        return false;
    }

    if (!CheckColor(info.bgColor) || !CheckColor(info.fontColor)) {
        LOG(ERROR) << "label viewinfo check failed, bgColor:" << info.bgColor <<
            " fontColor:" << info.fontColor;
        return false;
    }
    return true;
}

void TextLabelAdapter::SetText(const std::string &txt)
{
    // usage of "*" is same as label button's SetText
    if (txt == "*") {
        return;
    }
    OHOS::UILabel::SetText(txt.c_str());
}
} // namespace Updater
