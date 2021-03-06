/*
 * Copyright (c)      2016 Triad National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 */

/**
 * The Distributed System (dsys) Interface (DSI).
 */

#pragma once

#include "dsys/palp.h"

#include "core/process-landscape.h"
#include "tool-common/session-key.h"

#include <string>
#include <vector>

namespace gladius {
namespace dsi {

/**
 *
 */
class DSI {
private:
    //
    static const char sDSysName[];
    //
    static const char sPromptString[];
     //The initial size of the output buffer. 16k should be plenty.
    static constexpr size_t sInitBufSize = 1024 * 16;
    //
    dsys::AppLauncherPersonality mLauncherPersonality;
    //
    size_t mCurLineBufSize = 0;
    //
    bool mBeVerbose = false;
    //
    int mToAppl[2];
    //
    int mFromAppl[2];
    // PID of application launcher process.
    pid_t mApplPID = 0;
    //
    char *mFromDSysLineBuf = nullptr;
    //
    FILE *mTo = nullptr;
    //
    size_t
    mGetRespLine(void);
    //
    void
    mWaitForPrompt(void);
    //
    int
    mDrainToString(
        std::string &result
    );
    //
    int
    mSendCommand(
        const std::string &rawCMD
    );
    //
    int
    mRecvResp(
        std::string &outputIfSuccess
    );
    //
    void
    mChildCleanup(void);

public:
    //
    DSI(void);
    //
    ~DSI(void);
    //
    int
    init(
        const dsys::AppLauncherPersonality &palp,
        bool beVerbose
    );
    //
    int
    getProcessLandscape(
        core::ProcessLandscape &pl
    );
    //
    int
    publishConnectionInfo(
        toolcommon::SessionKey sessionKey,
        const std::vector<std::string> &leafInfos
    );
    //
    int
    shutdown(void);
};

} // end dsi namespace
} // end gladius namespace
