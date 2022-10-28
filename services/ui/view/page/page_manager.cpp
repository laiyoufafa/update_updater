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

#include "page_manager.h"
#include "common/screen.h"
#include "components/root_view.h"
#include "sub_page.h"

namespace Updater {
using namespace OHOS;
PageManager &PageManager::GetInstance()
{
    static PageManager instance;
    return instance;
}

void PageManager::InitImpl(UxPageInfo &pageInfo, std::string_view entry)
{
    if (!BasePage::IsPageInfoValid(pageInfo)) {
        return;
    }
    auto basePage = Page::Create<BasePage>(Screen::GetInstance().GetWidth(), Screen::GetInstance().GetHeight());
    if (basePage == nullptr || basePage->GetView() == nullptr) {
        LOG(ERROR) << "create base page failed";
        return;
    }
    if (!basePage->BuildPage(pageInfo)) {
        LOG(ERROR) << "Build page failed";
        return;
    }
    BuildSubPages(basePage->GetPageId(), basePage, pageInfo.subpages, entry);
    basePage->SetVisible(false);
    OHOS::RootView::GetInstance()->Add(basePage->GetView());
    if (!pageMap_.emplace(pageInfo.id, basePage).second) {
        LOG(ERROR) << "base page id duplicated:" << pageInfo.id;
        return;
    }
    pages_.push_back(basePage);
    if (pageInfo.id == entry) {
        mainPage_ = basePage;
    }
}

bool PageManager::Init(std::vector<UxPageInfo> &pageInfos, std::string_view entry)
{
    Reset();
    for (auto &pageInfo : pageInfos) {
        InitImpl(pageInfo, entry);
    }
    if (!IsValidPage(mainPage_)) {
        LOG(ERROR) << "entry " << entry << " is invalid ";
        return false;
    }
    curPage_ = mainPage_;
    return true;
}

void PageManager::BuildSubPages(const std::string &pageId, const std::shared_ptr<Page> &basePage,
    std::vector<UxSubPageInfo> &subPageInfos, std::string_view entry)
{
    for (auto &subPageInfo : subPageInfos) {
        if (!SubPage::IsPageInfoValid(subPageInfo)) {
            continue;
        }
        const std::string &subPageId = pageId + ":" + subPageInfo.id;
        auto subPage = Page::Create<SubPage>(basePage, subPageId);
        if (subPage == nullptr) {
            LOG(ERROR) << "create sub page failed";
            continue;
        }
        if (!subPage->BuildSubPage(subPageInfo)) {
            LOG(ERROR) << "build sub page failed";
            continue;
        }
        if (!pageMap_.emplace(subPageId, subPage).second) {
            LOG(ERROR) << "sub page id duplicated:" << subPageId;
            continue;
        }
        pages_.push_back(subPage);
        LOG(INFO) << subPageId << " builded";
        if (subPageId == entry) {
            mainPage_ = subPage;
        }
    }
}

bool PageManager::IsValidCom(const ComInfo &pageComId) const
{
    const std::string &pageId = pageComId.pageId;
    const std::string &comId = pageComId.comId;
    auto it = pageMap_.find(pageId);
    if (it == pageMap_.end() || it->second == nullptr) {
        LOG(ERROR) << "page id " << pageId << "not valid";
        return false;
    }
    const Page &page = *(it->second);
    return page.IsValidCom(comId);
}

bool PageManager::IsValidPage(const std::shared_ptr<Page> &pg) const
{
    return pg != nullptr && pg->IsValid();
}

void PageManager::ShowPage(const std::string &id)
{
    if (!IsValidPage(curPage_)) {
        LOG(ERROR) << "cur page is null";
        return;
    }
    if (id == curPage_->GetPageId()) {
        curPage_->SetVisible(true);
        LOG(WARNING) << "show cur page again";
        return;
    }
    auto it = pageMap_.find(id);
    if (it == pageMap_.end()) {
        LOG(ERROR) << "show page failed, id = " << id;
        return;
    }
    curPage_->SetVisible(false);
    EnQueuePage(curPage_);
    curPage_ = it->second;
    curPage_->SetVisible(true);
}

void PageManager::ShowMainPage()
{
    if (!IsValidPage(mainPage_)) {
        LOG(ERROR) << "main page invalid, can't show main page";
        return;
    }
    ShowPage(mainPage_->GetPageId());
}

void PageManager::GoBack()
{
    if (!IsValidPage(curPage_)) {
        LOG(ERROR) << "cur page is null";
        return;
    }
    if (pageQueue_.empty()) {
        LOG(ERROR) << "queue empty, can't go back";
        return;
    }
    curPage_->SetVisible(false);
    curPage_ = pageQueue_.front();
    pageQueue_.pop_front();
    curPage_->SetVisible(true);
}

Page &PageManager::operator[](const std::string &id) const
{
    static BasePage dummy;
    auto it = pageMap_.find(id);
    if (it == pageMap_.end() || it->second == nullptr) {
        return dummy;
    }
    return *(it->second);
}

ViewProxy &PageManager::operator[](const ComInfo &comInfo) const
{
    return (*this)[comInfo.pageId][comInfo.comId];
}

void PageManager::EnQueuePage(const std::shared_ptr<Page> &page)
{
    if (!IsValidPage(page)) {
        LOG(ERROR) << "enqueue invalid page";
        return;
    }
    pageQueue_.push_front(page);
    if (pageQueue_.size() > MAX_PAGE_QUEUE_SZ) {
        pageQueue_.pop_back();
    }
}

void PageManager::Reset()
{
    OHOS::RootView::GetInstance()->RemoveAll();
    pageQueue_.clear();
    pageMap_.clear();
    pages_.clear();
    curPage_ = nullptr;
    mainPage_ = nullptr;
}
} // namespace Updater