/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#include "host_updater.h"

#include <cstring>
#include "common.h"
#include "flash_define.h"
#include "transfer.h"
#include "serial_struct.h"

namespace {
static const std::string helpCmd = "flash";
static const std::string updateCmd = "update ";
static const std::string flashCmd = "flash ";
static const std::string eraseCmd = "erase ";
static const std::string formatCmd = "format ";
static const int PERCENT_FINISH = 100;
static const uint32_t PERCENT_CLEAR = ((uint32_t)-1);
}
namespace Hdc {
HostUpdater::HostUpdater(HTaskInfo hTaskInfo) : HdcTransferBase(hTaskInfo)
{
    commandBegin = CMD_UPDATER_BEGIN;
    commandData = CMD_UPDATER_DATA;
}

HostUpdater::~HostUpdater() {}

void HostUpdater::RunQueue(CtxFile &context)
{
    refCount++;
    context.localPath = context.taskQueue.back();
    uv_fs_open(loopTask, &context.fsOpenReq, context.localPath.c_str(), O_RDONLY, 0, OnFileOpen);
    context.master = true;
}

bool HostUpdater::BeginTransfer(CtxFile &context,
    const std::string &function, const char *payload, int minParam, int fileIndex)
{
    int argc = 0;
    char **argv = Base::SplitCommandToArgs(payload, &argc);
    HOSTUPDATER_CHECK(!(argv == nullptr || argc < minParam || fileIndex >= argc), delete[]((char *)argv);
        return false, "Invalid param for cmd \"%s\"", function.c_str());

    int maxParam = minParam;
    bool force = strstr(payload, "-f") != nullptr;
    if (force) {
        maxParam += 1;
    }
    HOSTUPDATER_CHECK(argc == maxParam, delete[]((char *)argv);
        return false, "Invalid param for cmd \"%s\" %d", function.c_str(), maxParam);

    context.transferConfig.functionName = function;
    context.transferConfig.options = payload;
    if (force && (fileIndex + 1 < argc) && strcmp(argv[fileIndex + 1], "-f") != 0) {
        context.localPath = argv[fileIndex + 1];
    } else {
        context.localPath = argv[fileIndex];
    }

    if (MatchPackageExtendName(context.localPath, ".img")) {
        context.transferConfig.compressType = COMPRESS_NONE;
    } else if (MatchPackageExtendName(context.localPath, ".bin")) {
        const char *part = strstr(payload, "fastboot");
        HOSTUPDATER_CHECK(part != nullptr, delete[]((char *)argv);
            return false, "Invalid image %s for cmd \"%s\"", context.localPath.c_str(), function.c_str());
        context.transferConfig.compressType = COMPRESS_NONE;
    } else {
        HOSTUPDATER_CHECK((MatchPackageExtendName(context.localPath, ".zip") ||
            MatchPackageExtendName(context.localPath, ".lz4") ||
            MatchPackageExtendName(context.localPath, ".gz2")), delete[]((char *)argv);
            return false,
            "Invaid extend name \"%s\" for cmd \"%s\"", context.localPath.c_str(), function.c_str());
    }

    WRITE_LOG(LOG_DEBUG, "BeginTransfer function: %s localPath: %s command: %s ",
        context.transferConfig.functionName.c_str(), context.localPath.c_str(), payload);
    // check path
    bool ret = Base::CheckDirectoryOrPath(context.localPath.c_str(), true, true);
    HOSTUPDATER_CHECK(ret, delete[]((char *)argv);
        return false,
        "Invaid path \"%s\" for cmd \"%s\"", context.localPath.c_str(), function.c_str());
    context.taskQueue.push_back(context.localPath);
    RunQueue(context);
    return true;
}

std::string HostUpdater::GetFileName(const std::string &fileName) const
{
    int32_t pos = fileName.find_last_of('/');
    if (pos < 0) { // win32
        pos = fileName.find_last_of('\\');
    }
    return fileName.substr(pos + 1, fileName.size());
}

void HostUpdater::CheckMaster(CtxFile *context)
{
    uv_fs_t fs;
    Base::ZeroStruct(fs.statbuf);
    uv_fs_fstat(nullptr, &fs, context->fsOpenReq.result, nullptr);
    context->transferConfig.fileSize = fs.statbuf.st_size;
    uv_fs_req_cleanup(&fs);

    WRITE_LOG(LOG_DEBUG, "CheckMaster %s %llu", context->transferConfig.functionName.c_str(), fs.statbuf.st_size);
    context->transferConfig.optionalName = GetFileName(context->localPath);
    std::string bufString = SerialStruct::SerializeToString(context->transferConfig);

    const uint64_t verdorSize = static_cast<uint64_t>(1024 * 1024) * 256;
    const uint64_t systemSize = static_cast<uint64_t>(1024 * 1024) * 1500;
    const uint64_t minSize = static_cast<uint64_t>(1024 * 1024) * 10;
    uint64_t realSize = verdorSize;
    if (fs.statbuf.st_size > minSize) {
        realSize += systemSize;
    }
    std::vector<uint8_t> buffer(sizeof(realSize) + bufString.size());
    int ret = memcpy_s(buffer.data(), buffer.size(), &realSize, sizeof(realSize));
    int ret2 = memcpy_s(buffer.data() + sizeof(realSize), buffer.size(), bufString.c_str(), bufString.size());
    if ((ret == 0) && (ret2 == 0)) {
        SendToAnother(CMD_UPDATER_CHECK, (uint8_t *)buffer.data(), buffer.size());
    }
}

bool HostUpdater::CheckCmd(const std::string &function, const char *payload, int param)
{
    int argc = 0;
    char **argv = Base::SplitCommandToArgs(payload, &argc);
    HOSTUPDATER_CHECK(argv != nullptr, return false, "Can not parser cmd \"%s\"", function.c_str());
    delete[]((char *)argv);
    HOSTUPDATER_CHECK(argc >= param, return false, "Invalid param for cmd \"%s\" %d", function.c_str(), argc);
    WRITE_LOG(LOG_DEBUG, "CheckCmd command: %s ", payload);

    int maxParam = param;
    if (strstr(payload, "-f") != nullptr) {
        maxParam += 1;
    }
    if (strstr(payload, "-t") != nullptr) {
        maxParam += 1;
        maxParam += 1;
    }
    HOSTUPDATER_CHECK(argc == maxParam, return false,
        "Invalid param for cmd \"%s\" %d %d", function.c_str(), argc, maxParam);
    return true;
}

bool HostUpdater::CommandDispatch(const uint16_t command, uint8_t *payload, const int payloadSize)
{
    const int cmdFroErase = 2;
    const int cmdFroFormat = 2;
#ifndef UPDATER_UT
    if (!HdcTransferBase::CommandDispatch(command, payload, payloadSize)) {
        return false;
    }
#endif
    bool ret = true;
    switch (command) {
        case CMD_UPDATER_BEGIN: {
            std::string s("Processing:    0%%");
            sendProgress = true;
            SendRawData(reinterpret_cast<uint8_t *>(s.data()), s.size());
            break;
        }
        case CMD_UPDATER_UPDATE_INIT:
            ret = BeginTransfer(ctxNow, CMDSTR_UPDATE_SYSTEM, reinterpret_cast<const char *>(payload), 1, 0);
            break;
        case CMD_UPDATER_FLASH_INIT:
            ret = BeginTransfer(ctxNow, CMDSTR_FLASH_PARTITION,
                reinterpret_cast<const char *>(payload), 2, 1); // 2 cmd min param for flash
            break;
        case CMD_UPDATER_FINISH:
            ret = CheckUpdateContinue(command, payload, payloadSize);
            break;
        case CMD_UPDATER_ERASE: {
            if (!CheckCmd(CMDSTR_ERASE_PARTITION, reinterpret_cast<const char *>(payload), cmdFroErase)) {
                return false;
            }
            SendToAnother(CMD_UPDATER_ERASE, payload, payloadSize);
            ctxNow.taskQueue.push_back(reinterpret_cast<char *>(payload));
            break;
        }
        case CMD_UPDATER_FORMAT: {
            if (!CheckCmd(CMDSTR_FORMAT_PARTITION, reinterpret_cast<const char *>(payload), cmdFroFormat)) {
                return false;
            }
            SendToAnother(CMD_UPDATER_FORMAT, payload, payloadSize);
            ctxNow.taskQueue.push_back(reinterpret_cast<char *>(payload));
            break;
        }
        case CMD_UPDATER_PROGRESS:
            if (payloadSize >= (int)sizeof(uint32_t)) {
                ProcessProgress(*(uint32_t *)payload);
            }
            break;
        default:
            break;
    }
    return ret;
}

void HostUpdater::ProcessProgress(uint32_t percentage)
{
    if (!sendProgress) {
        return;
    }
    std::string backStr = "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
    std::string breakStr = "\n";
    WRITE_LOG(LOG_INFO, "ProcessProgress %d", percentage);
    const int bufferSize = 128;
    std::vector<char> buffer(bufferSize);
    if (percentage == PERCENT_CLEAR) { // clear
        SendRawData(reinterpret_cast<uint8_t *>(backStr.data()), backStr.size());
        SendRawData(reinterpret_cast<uint8_t *>(breakStr.data()), breakStr.size());
        sendProgress = false;
        return;
    }
    int len = sprintf_s(buffer.data(), buffer.size() - 1, "%sProcessing:   %3d%%", backStr.c_str(), percentage);
    HOSTUPDATER_CHECK(len > 0, return, "Failed to format progress info ");
    SendRawData(reinterpret_cast<uint8_t *>(buffer.data()), len);
    if (percentage == PERCENT_FINISH) {
        SendRawData(reinterpret_cast<uint8_t *>(breakStr.data()), breakStr.size());
        sendProgress = false;
    }
}

bool HostUpdater::CheckUpdateContinue(const uint16_t command, const uint8_t *payload, int payloadSize)
{
    HOSTUPDATER_CHECK(static_cast<size_t>(payloadSize) >= sizeof(uint16_t),
        return false, "Failed to check payload size %d ", payloadSize);
    MessageLevel level = (MessageLevel)payload[1];
    if ((level == MSG_OK) && sendProgress) {
        ProcessProgress(PERCENT_FINISH);
    }
    std::string info((char*)(payload + sizeof(uint16_t)), payloadSize - sizeof(uint16_t));
    if (!info.empty()) {
        LogMsg(level, "%s", info.c_str());
    }
    WRITE_LOG(LOG_DEBUG, "CheckUpdateContinue payloadSize %d %d %s", payloadSize, level, info.c_str());
    if (ctxNow.taskQueue.size() != 0) {
        ctxNow.taskQueue.pop_back();
    }
    if (singalStop || !ctxNow.taskQueue.size()) {
        return false;
    }
    RunQueue(ctxNow);
    return true;
}

bool HostUpdater::CheckMatchUpdate(const std::string &input,
    std::string &stringError, uint16_t &cmdFlag, bool &bJumpDo)
{
    WRITE_LOG(LOG_DEBUG, "CheckMatchUpdate command:%s", input.c_str());
    size_t cmdLen = updateCmd.size();
    if (!strncmp(input.c_str(), updateCmd.c_str(), updateCmd.size())) {
        cmdFlag = CMD_UPDATER_UPDATE_INIT;
        cmdLen = updateCmd.size();
    } else if (!strncmp(input.c_str(), flashCmd.c_str(), flashCmd.size())) {
        cmdFlag = CMD_UPDATER_FLASH_INIT;
        cmdLen = flashCmd.size();
    } else if (!strncmp(input.c_str(), eraseCmd.c_str(), eraseCmd.size())) {
        cmdFlag = CMD_UPDATER_ERASE;
        cmdLen = eraseCmd.size();
    } else if (!strncmp(input.c_str(), formatCmd.c_str(), formatCmd.size())) {
        cmdFlag = CMD_UPDATER_FORMAT;
        cmdLen = formatCmd.size();
    } else {
        return false;
    }
    if (input.size() <= cmdLen) {
        stringError = "Incorrect command";
        bJumpDo = true;
    }
    return true;
}

#ifdef UPDATER_UT
static std::string g_input = "yes";
void HostUpdater::SetInput(const std::string &input)
{
    g_input = input;
}
#endif
bool HostUpdater::ConfirmCommand(const string &commandIn, bool &closeInput)
{
    std::string tip = "";
    if (!strncmp(commandIn.c_str(), updateCmd.c_str(), updateCmd.size())) {
        closeInput = true;
    } else if (!strncmp(commandIn.c_str(), flashCmd.c_str(), flashCmd.size())) {
        tip = "Confirm flash partition";
        closeInput = true;
    } else if (!strncmp(commandIn.c_str(), eraseCmd.c_str(), eraseCmd.size())) {
        tip = "Confirm erase partition";
    } else if (!strncmp(commandIn.c_str(), formatCmd.c_str(), formatCmd.size())) {
        tip = "Confirm format partition";
    }
    if (tip.empty() || strstr(commandIn.c_str(), " -f") != nullptr) { // check if -f
        return true;
    }
    const size_t minLen = strlen("yes");
    int retryCount = 0;
    do {
        printf("%s ? (Yes/No) ", tip.c_str());
        fflush(stdin);
        std::string info = {};
        size_t i = 0;
        while (1) {
#ifndef UPDATER_UT
            char c = getchar();
#else
            char c = '\n';
            info = g_input;
#endif
            if (c == '\r' || c == '\n') {
                break;
            }
            if (c == ' ') {
                continue;
            }
            if (i < minLen && isprint(c)) {
                info.append(1, std::tolower(c));
                i++;
            }
        }
        if (info == "n" || info == "no") {
            return false;
        }
        if (info == "y" || info == "yes") {
            return true;
        }
        retryCount++;
    } while (retryCount < 3); // 3 retry max count
    return (retryCount >= 3) ? false : true; // 3 retry max count
}

void HostUpdater::SendRawData(uint8_t *bufPtr, const int size)
{
#ifndef UPDATER_UT
    HdcSessionBase *sessionBase = (HdcSessionBase *)clsSession;
    sessionBase->ServerCommand(taskInfo->sessionId,
        taskInfo->channelId, CMD_KERNEL_ECHO_RAW, bufPtr, size);
#endif
}
} // namespace Hdc