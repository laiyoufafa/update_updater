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

#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#define private public
#include "../applypatch/command_process.h"
#undef private
#include "log/log.h"
#include "applypatch/transfer_manager.h"


using namespace testing::ext;
using namespace Updater;
using namespace std;
namespace UpdaterUt{
class CommandFunctionUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp();
    void TearDown();
};

void CommandFunctionUnitTest::SetUpTestCase()
{
    cout << "Updater Unit CommandFunctionUnitTest Setup!" << endl;
}

void CommandFunctionUnitTest::SetUp()
{
    cout << "Updater Unit CommandFunctionUnitTest Begin!" << endl;
}

void CommandFunctionUnitTest::TearDown()
{
    cout << "Updater Unit CommandFunctionUnitTest End!" << endl;
}

HWTEST_F(CommandFunctionUnitTest, command_function_test_001, TestSize.Level1){
    Command* cmd = new Command();
    std::string cmd_line = std::string("abort");
    cmd->Init(cmd_line);
    AbortCommandFn cmd_abort;
    EXPECT_EQ(cmd_abort.Execute(*cmd), 0);
    cmd->SetFileDescriptor(0);
    cmd_line = "new 2,0,1";
    cmd->Init(cmd_line);
    NewCommandFn cmd_new;
    EXPECT_EQ(cmd_new.Execute(*cmd), -1);
    cmd_line = "erase 1,1";
    cmd->Init(cmd_line);
    ZeroAndEraseCommandFn cmd_erase;
    EXPECT_EQ(cmd_erase.Execute(*cmd), -1);
    delete cmd;
}
}
