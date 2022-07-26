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

#ifndef UPDATER_UI_GRAPHIC_DRV_H
#define UPDATER_UI_GRAPHIC_DRV_H

namespace Updater {
using GrSurface = struct GrSurface_ {
    int width;
    int height;
    unsigned int rowBytes;
    unsigned int pixelBytes;
};
class IGraphicDriver {
public:
    IGraphicDriver() : fd_(-1) {}
    virtual ~IGraphicDriver() {}
    virtual void Init() = 0;
    virtual void Flip(const uint8_t *buf) = 0;
    virtual void GetGrSurface(GrSurface &surface) = 0;
protected:
    int fd_;
};
}; // namespace Updater
#endif
