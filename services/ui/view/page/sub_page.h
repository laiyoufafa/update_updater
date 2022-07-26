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

#ifndef SUB_PAGE_H
#define SUB_PAGE_H

#include "base_page.h"
#include "page.h"
#include "view_api.h"

namespace Updater {
class SubPage : public Page {
public:
    SubPage(UxSubPageInfo &subpageInfo, BasePage &basePage, const std::string &pageId);
    virtual ~SubPage() = default;
    std::string &GetPageId() override;
    void SetVisible(bool isVisible) override;
    bool IsVisible() const override;
    OHOS::UIViewGroup *GetView() const override;
    bool IsValid() const override;
    bool IsValidCom(const std::string &id) const override;
    ViewProxy operator[](const std::string &id) override;
private:
    BasePage &basePage_;
    std::string pageId_;
    std::vector<std::string> comsId_;
    bool isVisible_;
    UxBRGAPixel color_;
};
}
#endif
