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
#include "../applypatch/command_process.h"
#include "log/log.h"
#include "applypatch/transfer_manager.h"


using namespace testing::ext;
using namespace Updater;
using namespace std;
namespace UpdaterUt {
class CommandFunctionUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp();
    void TearDown();
    CommandResult TestCommandFnExec(std::shared_ptr<Command> cmd, std::string cmdLine)
    {
        cmd->Init(cmdLine);
        std::unique_ptr<CommandFunction> cf = CommandFunctionFactory::GetCommandFunction(cmd->GetCommandType);
        CommandResult ret = cf->Execute(const_cast<Command &>(*cmd.get()));
        CommandFunctionFactory::ReleaseCommandFunction(cf);
        return ret;
    }
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

HWTEST_F(CommandFunctionUnitTest, command_function_test_001, TestSize.Level1)
{
    Command* cmd = new Command();
    std::string cmdLine = std::string("abort");
    cmd->Init(cmdLine);
    AbortCommandFn cmdAbort;
    EXPECT_EQ(cmdAbort.Execute(*cmd), 0);
    cmd->SetFileDescriptor(0);
    cmdLine = "new 2,0,1";
    cmd->Init(cmdLine);
    NewCommandFn cmdNew;
    EXPECT_EQ(cmdNew.Execute(*cmd), -1);
    cmdLine = "erase 1,1";
    cmd->Init(cmdLine);
    ZeroAndEraseCommandFn cmdErase;
    EXPECT_EQ(cmdErase.Execute(*cmd), -1);
    delete cmd;
}

HWTEST_F(CommandFunctionUnitTest, command_function_test_002, TestSize.Level1)
{
    std::string filepath = "/data/updater/updater/allCmdUnitTest.bin";
    std::unique_ptr<Command> cmd;
    cmd = std::make_unique<Command>();
    TransferManagerPtr tm = TransferManager::GetTransferManagerInstance();
    mode_t dirMode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    tm->GetGlobalParams()->storeBase = "data/updater/updater/tmp/cmdtest";
    Store::DoFreeSpace(TransferManager::GetTransferManagerInstance()->GetGlobalParams()->storeBase);
    Utils::MkdirRecursive(TransferManager::GetTransferManagerInstance()->GetGlobalParams()->storeBase, dirMode);
    int fd = open(filePath.c_str(), O_RDWR | O_CREAT, dirMode);
    if (fd < 0) {
        printf("Failed to open block %s, errno: %d\n", filePath.c_str(), errno);
        return;
    }
    lseek64(fd, 0, SEEK_SET);
    cmd->SetFileDescriptor(fd);
    std::string cmdLine = std::string("erase 2,0,1");
    CommandResult ret = CommandFunctionUnitTest::TestCommandFnExec(cmd, cmdLine);
    EXPECT_EQ(ret, 0);
    cmdLine = "free 2,0,1";
    ret = CommandFunctionUnitTest::TestCommandFnExec(cmd, cmdLine);
    tm->GetGlobalParams()->storeCreated = 0;
    EXPECT_EQ(ret, 0);
}
}
