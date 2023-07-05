/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "keys_input_device.h"
#include "log/log.h"

namespace Updater {
enum KeyUpDownEvent {
    EVENT_KEY_UP_VALUE,
    EVENT_KEY_DOWN_VALUE
};

KeysInputDevice &KeysInputDevice::GetInstance()
{
    static KeysInputDevice instance;
    return instance;
}

bool KeysInputDevice::Read(OHOS::DeviceData& data)
{
    data.keyId = lastKeyId_;
    data.state = keyState_;
    keyState_ = OHOS::INVALID_KEY_STATE;
    return false;
}

int KeysInputDevice::HandleKeyEvent(const input_event &ev, uint32_t type)
{
    if (ev.type != EV_KEY || ev.code > KEY_MAX) {
        return 0;
    }
    if (ev.code == BTN_TOUCH || ev.code == BTN_TOOL_FINGER) {
        return 0;
    }
    if (ev.code == BTN_MOUSE || ev.code == BTN_LEFT || ev.code == BTN_RIGHT || ev.code == BTN_MIDDLE) {
        return 0;
    }

    // KEY_VOLUMEDOWN = 114, KEY_VOLUMEUP = 115, KEY_POWER = 116
    if (ev.code == KEY_VOLUMEDOWN || ev.code == KEY_VOLUMEUP || ev.code == KEY_POWER) {
        keyState_ = (ev.value == EVENT_KEY_DOWN_VALUE) ?
            OHOS::InputDevice::STATE_PRESS : OHOS::InputDevice::STATE_RELEASE;
    } else {
        keyState_ = ev.value;
    }

    lastKeyId_ = ev.code;
    return 0;
}
} // namespace Updater