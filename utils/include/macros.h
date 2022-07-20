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

#ifndef MACRO_H
#define MACRO_H

#define DISALLOW_COPY_MOVE_ASSIGN(ClassName)            \
    ClassName &operator = (const ClassName &) = delete; \
    ClassName &operator = (ClassName &&) = delete

#define DISALLOW_COPY_MOVE_CONSTRUCT(ClassName) \
    ClassName(const ClassName &) = delete;      \
    ClassName(ClassName &&) = delete

#define DISALLOW_COPY_MOVE(ClassName)     \
    DISALLOW_COPY_MOVE_ASSIGN(ClassName); \
    DISALLOW_COPY_MOVE_CONSTRUCT(ClassName)

#define DEFINE_COMMON_FOR_SINGLETON(className)     \
        DISALLOW_COPY_MOVE(className);             \
    public:                                        \
        static className &GetInstance()            \
        {                                          \
            static className instance;             \
            return instance;                       \
        }

#endif