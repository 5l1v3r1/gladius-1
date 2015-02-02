/*
 * Copyright (c) 2014-2015 Los Alamos National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 */

#include "core/utils.h"

using namespace gladius::core;

const std::string utils::osPathSep = "/";

/**
 * Takes an argv-like structure and returns a duplicate that needs to be
 * freed by the caller by calling freeDupArgv.
 */
char **
utils::dupArgv(
    int argc,
    const char **argv
) {
    if (!argv) GLADIUS_THROW_INVLD_ARG();
    int i, strLen;
    char **dup = NULL;
    // allocate an extra spot for cap
    dup = (char **)calloc(argc + 1, sizeof(char *));
    if (!dup) {
        GLADIUS_THROW_OOR();
        // Never reached
        return NULL;
    }
    for (i = 0; i < argc; ++i) {
        strLen = strlen(argv[i]) + 1;
        dup[i] = (char *)calloc(strLen, sizeof(char *));
        if (NULL == dup[i]) {
            GLADIUS_THROW_OOR();
            // Never reached
            return NULL;
        }
        (void)memmove(dup[i], argv[i], strLen);
    }
    dup[i] = NULL;
    return dup;
}

/**
 * Frees the memory allocated by dupArgv.
 */
void
utils::freeDupArgv(char **dup)
{
    char **str = NULL;
    if (NULL == dup) return;
    for (str = dup; NULL != *str; ++str) {
        free(*str);
        *str = NULL;
    }
    free(dup);
}

/**
 * Returns a string containing the host name of the calling process.
 */
std::string
utils::getHostname(void)
{
    using namespace std;

    char hnBuf[HOST_NAME_MAX];
    (void)memset(hnBuf, '\0', sizeof(hnBuf));
    if (-1 == gethostname(hnBuf, HOST_NAME_MAX)) {
        int err = errno;
        string errStr = "gethostname. Why: " + getStrError(err) + ".";
        GLADIUS_THROW_CALL_FAILED(errStr);
    }
    return string(hnBuf);
}

/**
 * Returns a string containing the value of envVarName.
 */
std::string
utils::getEnv(const std::string &envVarName)
{
    char *cEnvP = getenv(envVarName.c_str());
    if (!cEnvP) {
        auto errStr = "getEnv. Why: Value for '" + envVarName
                    + "' requested, but not set.";
        GLADIUS_THROW_CALL_FAILED(errStr);
    }
    return std::string(cEnvP);
}

/**
 * Wrapper for setenv(3).
 */
void
utils::setEnv(
    const std::string &envVarName,
    const std::string &value,
    bool overwrite
) {
    int overwriteI = overwrite ? 1 : 0;
    auto rc = setenv(envVarName.c_str(), value.c_str(), overwriteI);
    if (-1 == rc) {
        GLADIUS_THROW_CALL_FAILED("setEnv");
    }
}

/**
 * Wrapper for unsetenv(3).
 */
int
utils::unsetEnv(
    const std::string &envVarName,
    int &errNo
) {
    auto rc = unsetenv(envVarName.c_str());
    if (-1 == rc) {
        errNo = errno;
        return GLADIUS_ERR;
    }
    return GLADIUS_SUCCESS;
}

/**
 * Replicates 'which'.
 * Searches the user's $PATH for a program file.
 */
int
utils::which(std::string execName,
      std::string &result)
{
    std::string cmdString = "which " + execName;
    FILE *fp = popen(cmdString.c_str(), "r");
    if (!fp) {
        return GLADIUS_ERR_IO;
    }
    // Else capture the output. Should either be the path or nothing on
    // first line.
    char lineBuff[PATH_MAX];
    fgets(lineBuff, sizeof(lineBuff), fp);
    auto pcrc = pclose(fp);
    // The exit status of this will tell us whether or not our command
    // succeeded or not. If success, then finish prepping the answer.
    if (0 != WEXITSTATUS(pcrc)) {
        return GLADIUS_ERR;
    }
    // Stash into result string.
    result = lineBuff;
    // Trim
    trim(result);
    // Remove newline from end of string.
    result.erase(std::remove(result.begin(), result.end(), '\n'),
                 result.end());
    return GLADIUS_SUCCESS;
}

/**
 * Returns the absolute path to the caller's executable.
 */
int
utils::getSelfPath(
    std::string &maybeRes,
    int &errnoIfNotSuccess
) {
    char buff[PATH_MAX];
    (void)memset(buff, '\0', sizeof(buff));
    auto len = ::readlink("/proc/self/exe", buff, sizeof(buff) - 1);
    if (len != -1) {
        buff[len] = '\0';
        maybeRes = std::string(buff);
        return GLADIUS_SUCCESS;
    }
    else {
        errnoIfNotSuccess = errno;
        return GLADIUS_ERR;
    }
}

/**
 * Creates directories like: mkdir PATH.
 */
int
utils::mkDir(
    const std::string &path,
    int &errnoOnFailure
) {
    errnoOnFailure = 0;
    auto status = mkdir(path.c_str(),
                        S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH
                  );
    if (0 != status) {
        errnoOnFailure = errno;
        return GLADIUS_ERR;
    }
    return GLADIUS_SUCCESS;
}
