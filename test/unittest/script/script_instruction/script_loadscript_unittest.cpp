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

#include <gtest/gtest.h>
#include <vector>
#include "script_context.h"
#include "script_instruction.h"
#include "script_instruction_unittest.h"
#include "script_instructionhelper.h"
#include "script_loadscript.h"
#include "script_manager_impl.h"
#include "script/script_unittest.h"

using namespace Uscript;
using namespace BasicInstruction;
using namespace Hpackage;
using namespace testing::ext;

class TestPkgManager : public TestScriptPkgManager {
public:
    int32_t ExtractFile(const std::string &fileId, StreamPtr output) override
    {
        return 0;
    }
    const FileInfo *GetFileInfo(const std::string &fileId) override
    {
        static FileInfo fileInfo {};
        if (fileId == "script") {
            return &fileInfo;
        }
        return nullptr;
    }
};

class LoadScriptInstructionUnittest : public ::testing::Test {
public:
    LoadScriptInstructionUnittest() {}
    ~LoadScriptInstructionUnittest() {}
    void TestLoadScript01() const
    {
        TestPkgManager pkgManager;
        UTestScriptEnv env {&pkgManager};
        ScriptManagerImpl scriptManager {&env};
        ScriptInstructionHelper::GetBasicInstructionHelper(&scriptManager);
        UScriptInstructionContext context {};
        context.AddInputParam(std::make_shared<IntegerValue>(1));
        auto instruction = std::make_unique<ScriptLoadScript>();
        EXPECT_EQ(instruction->Execute(env, context), USCRIPT_INVALID_PARAM);
        std::vector<UScriptValuePtr> output = context.GetOutVar();
        EXPECT_EQ(output.size(), 0);
        ScriptInstructionHelper::ReleaseBasicInstructionHelper();
    }
    void TestLoadScript02() const
    {
        TestPkgManager pkgManager;
        UTestScriptEnv env {&pkgManager};
        ScriptManagerImpl scriptManager {&env};
        ScriptInstructionHelper::GetBasicInstructionHelper(&scriptManager);
        UScriptInstructionContext context {};
        context.AddInputParam(std::make_shared<StringValue>("script1"));
        context.AddInputParam(std::make_shared<StringValue>("invalid param"));
        auto instruction = std::make_unique<ScriptLoadScript>();
        EXPECT_EQ(instruction->Execute(env, context), USCRIPT_INVALID_PARAM);
        std::vector<UScriptValuePtr> output = context.GetOutVar();
        EXPECT_EQ(output.size(), 0);
        ScriptInstructionHelper::ReleaseBasicInstructionHelper();
    }
    void TestLoadScript03() const
    {
        TestPkgManager pkgManager;
        UTestScriptEnv env {&pkgManager};
        ScriptManagerImpl scriptManager {&env};
        ScriptInstructionHelper::GetBasicInstructionHelper(&scriptManager);
        UScriptInstructionContext context {};
        context.AddInputParam(std::make_shared<StringValue>("script"));
        context.AddInputParam(std::make_shared<IntegerValue>(1));
        auto instruction = std::make_unique<ScriptLoadScript>();
        EXPECT_EQ(instruction->Execute(env, context), USCRIPT_SUCCESS);
        std::vector<UScriptValuePtr> output = context.GetOutVar();
        EXPECT_EQ(output.size(), 0);
        ScriptInstructionHelper::ReleaseBasicInstructionHelper();
    }
protected:
    void SetUp() {}
    void TearDown() {}
    void TestBody() {}
};

HWTEST_F(LoadScriptInstructionUnittest, TestLoadScript01, TestSize.Level1)
{
    LoadScriptInstructionUnittest {}.TestLoadScript01();
}

HWTEST_F(LoadScriptInstructionUnittest, TestLoadScript02, TestSize.Level1)
{
    LoadScriptInstructionUnittest {}.TestLoadScript02();
}

HWTEST_F(LoadScriptInstructionUnittest, TestLoadScript03, TestSize.Level1)
{
    LoadScriptInstructionUnittest {}.TestLoadScript03();
}