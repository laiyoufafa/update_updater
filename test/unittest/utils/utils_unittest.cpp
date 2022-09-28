/*
* Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "unittest_comm.h"
#include "utils.h"

using namespace Updater;
using namespace testing::ext;
using namespace std;

namespace UpdaterUt {
class UtilsUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(UtilsUnitTest, updater_utils_test_001, TestSize.Level0)
{
    string emptyStr = Utils::Trim("");
    EXPECT_STREQ(emptyStr.c_str(), "");
    emptyStr = Utils::Trim("   ");
    EXPECT_STREQ(emptyStr.c_str(), "");
    emptyStr = Utils::Trim("aa   ");
    EXPECT_STREQ(emptyStr.c_str(), "aa");
}

HWTEST_F(UtilsUnitTest, updater_utils_test_002, TestSize.Level0)
{
    uint8_t a[1] = {0};
    a[0] = 1;
    string newStr = Utils::ConvertSha256Hex(a, 1);
    EXPECT_STREQ(newStr.c_str(), "01");
}

HWTEST_F(UtilsUnitTest, updater_utils_test_003, TestSize.Level0)
{
    string str = "aaa\nbbb";
    vector<string> newStr = Utils::SplitString(str, "\n");
    EXPECT_EQ(newStr[0], "aaa");
    EXPECT_EQ(newStr[1], "bbb");
}

HWTEST_F(UtilsUnitTest, updater_utils_test_004, TestSize.Level0)
{
    EXPECT_EQ(Utils::MkdirRecursive("/data/xx?xx", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH), 0);
}

HWTEST_F(UtilsUnitTest, updater_utils_test_005, TestSize.Level0)
{
    string input = "";
    int output = Utils::String2Int<int>(input, 10);
    EXPECT_EQ(output, 0);
    input = "0x01";
    output = Utils::String2Int<int>(input, 10);
    EXPECT_EQ(output, 1);
}

HWTEST_F(UtilsUnitTest, updater_utils_test_006, TestSize.Level0)
{
    std::vector<std::string> files;
    string path = "/data";
    Utils::GetFilesFromDirectory(path, files, true);
}

HWTEST_F(UtilsUnitTest, RemoveDirTest, TestSize.Level0)
{
    string path = "";
    EXPECT_EQ(Utils::RemoveDir(path), false);
    path = TEST_PATH_FROM + "../utils/nonExistDir";
    EXPECT_EQ(Utils::RemoveDir(path), false);
    path = "/data/updater/rmDir";
    int ret = mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (ret == 0) {
        ofstream tmpFile;
        string filePath = path + "/tmpFile";
        tmpFile.open(filePath.c_str());
        if (tmpFile.is_open()) {
            tmpFile.close();
            EXPECT_EQ(Utils::RemoveDir(path), true);
        }
    }
}

HWTEST_F(UtilsUnitTest, IsUpdaterMode, TestSize.Level0)
{
    EXPECT_EQ(Utils::IsUpdaterMode(), false);
}
} // updater_ut
