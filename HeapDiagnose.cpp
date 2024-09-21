/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: heap diagnose
 */
#include "HeapDiagnose.h"
#include <unistd.h>
#include <dlfcn.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <syslog.h>
#include "HeapDump.h"
#include "HeapInfoOutput.h"
#include "HeapManage.h"

struct heap_cmd_list {
    OSP_CHAR* cmdName;
    FN_DBG_CMD_PROC pFunc;
};

extern "C" void show_config_help(OSP_CHAR* v_szCommand, OSP_S32 iShowDetail)
{
    static const OSP_CHAR* configUsage =
        "heap startrecord\n"\
        "heap stoprecord\n"\
        "heap showall\n"\
        "heap showtop [num]\n"\
        "heap dumptolog [path]\n"\
        "heap setstackdepth [num]\n";
    auto printFunc = HeapProfile::HeapDiagnose::GetInstance().prinfInfoFunc;
    if (!printFunc) {
        return;
    }
    printFunc("%s", configUsage);
}

extern "C"  void do_config_cmd(int32_t argc, char* argv[])
{
    // 2 is argc num
    if (argc < 2) {
        show_config_help(NULL, 0);
        return;
    }

    if (!strcmp(argv[1], "showall")) {
        HeapProfile::HeapDiagnose::GetInstance().PrintHeapProfileToDiagnose(true, 0);
    } else if (!strcmp(argv[1], "showtop") && argc == 3) { // 3 is argc num
        HeapProfile::HeapDiagnose::GetInstance().PrintHeapProfileToDiagnose(false, atoi(argv[2])); // 2 is argc num
    } else if (!strcmp(argv[1], "dumptolog") && argc == 3) { // 3 is argc num
        HeapProfile::HeapDump::GetInstance().DumpHeapProfileToLog(argv[2]); // 2 is argc num
    } else if (!strcmp(argv[1], "startrecord")) {
        HeapProfile::HeapRecord::GetInstance().StartHeapRecord();
    } else if (!strcmp(argv[1], "stoprecord")) {
        HeapProfile::HeapRecord::GetInstance().StopHeapRecord();
    } else if (!strcmp(argv[1], "setstackdepth") && argc == 3) { // 3 is argc num
        HeapProfile::HeapInfoOutput::SetMaxSymbolDepth(atoi(argv[2])); // 2 is argc num
    } else {
        show_config_help(NULL, 0);
    }
}

static DBG_CMD_S g_HeapProfileCmd = {
    "heap",
    "config heap profile information",
    do_config_cmd,
    show_config_help,
};

namespace HeapProfile {
    HeapDiagnose::HeapDiagnose() : HeapInfoOutput(), regCmdFunc(nullptr), prinfInfoFunc(nullptr)
    {
    }

    HeapDiagnose::~HeapDiagnose()
    {
    }

    bool HeapDiagnose::FindDiagnoseSymbol()
    {
        if (regCmdFunc && prinfInfoFunc) {
            return true;
        }
#ifdef _DEBUG
        regCmdFunc = reinterpret_cast<RegDiagnoseCmdFunc>(dlsym(RTLD_NEXT, "DBG_RegCmd"));
#else
        regCmdFunc = reinterpret_cast<RegDiagnoseCmdFunc>(dlsym(RTLD_NEXT, "DBG_RegCmdToCLI"));
#endif
        prinfInfoFunc = reinterpret_cast<PrintDiagnoseInfoFunc>(dlsym(RTLD_NEXT, "DBG_Print"));
        auto handle = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
        if (!regCmdFunc) {
#ifdef _DEBUG
            regCmdFunc = reinterpret_cast<RegDiagnoseCmdFunc>(dlsym(handle, "DBG_RegCmd"));
#else
            regCmdFunc = reinterpret_cast<RegDiagnoseCmdFunc>(dlsym(handle, "DBG_RegCmdToCLI"));
#endif
        }

        if (!prinfInfoFunc) {
            prinfInfoFunc = reinterpret_cast<PrintDiagnoseInfoFunc>(dlsym(handle, "DBG_Print"));
        }

        if (!regCmdFunc || !prinfInfoFunc) {
            return false;
        }
        return true;
    }

    void HeapDiagnose::StartRegisterDiagnoseCmd()
    {
        if (FindDiagnoseSymbol() && regCmdFunc(&g_HeapProfileCmd) == 0) {
            return;
        }
        regDiagThrd = std::make_shared<std::thread>(&HeapDiagnose::RegDiagnoseThrd, this);
        regDiagThrd->detach();
    }

    void HeapDiagnose::RegDiagnoseThrd()
    {
        HeapManage::SetThrdRecordFlag(false);
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5 seconds
            if (FindDiagnoseSymbol() && regCmdFunc(&g_HeapProfileCmd) == 0) {
                break;
            }
        }
    }

    void HeapDiagnose::PrintHeapProfileToDiagnose(bool showall, unsigned int top)
    {
        bool old = HeapManage::GetAndSetThrdRecordFlag(false);
        long totalAlloc;
        std::vector<HeapDumpInfo> dumpInfo;
        HeapRecord::GetInstance().GetHeapProfileInfo(totalAlloc, dumpInfo);

        if (dumpInfo.empty() || totalAlloc <= 0) {
            HeapManage::SetThrdRecordFlag(old);
            return;
        }

        if (!prinfInfoFunc) {
            HeapManage::SetThrdRecordFlag(old);
            return;
        }

        std::sort(dumpInfo.begin(), dumpInfo.end(), [](const HeapDumpInfo& a, const HeapDumpInfo& b) 
            {return a.allocsize_ > b.allocsize_; });
        
        prinfInfoFunc("Total:%lu bytes\n", totalAlloc);
        prinfInfoFunc("%s%9s%s%9s%s%9s%s\n", "AllocSize(bytes)", " ", "AllocNum", " ", "FreeNum", " ", "Stack");
        top = top < dumpInfo.size() ? top : dumpInfo.size();
        unsigned int end = showall ? dumpInfo.size() : top;
        for (auto i = 0; i < end; ++i) {
            std::string str;
            FormatOneLineAllocInfo(dumpInfo[i], str);
            prinfInfoFunc("%s\n", str.c_str());
        }
        HeapManage::SetThrdRecordFlag(old);
    }
}

