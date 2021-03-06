/*
 * Copyright (c) 2014-2016 Triad National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 */

/**
 * To fully understand the entire set of interactions here, you'll also need to
 * see the back-end versions of the tool, LaunchMon, and MRNet.
 */

#include "tool-fe.h"

#include "core/utils.h"
#include "core/env.h"

#include <cassert>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;
using namespace gladius;
using namespace gladius::core;
using namespace gladius::toolfe;

namespace {
// This component's name.
const string CNAME = "tool-fe";
//
const auto COMPC = core::colors::GREEN;
// CNAME's color code.
const string NAMEC = core::colors::color().ansiBeginColor(COMPC);
// Convenience macro to decorate this component's output.
#define COMP_COUT GLADIUS_COMP_COUT(CNAME, NAMEC)
// Output if this component is being verbose.
#define VCOMP_COUT(streamInsertions)                                           \
do {                                                                           \
    if (this->mBeVerbose) {                                                    \
        COMP_COUT << streamInsertions;                                         \
    }                                                                          \
} while (0)

////////////////////////////////////////////////////////////////////////////////
// Place for ToolFE-specific environment variables.
////////////////////////////////////////////////////////////////////////////////
//
#define ENV_VAR_CONNECT_TIMEOUT_IN_SEC "GLADIUS_TOOL_FE_CONNECT_TIMEOUT_S"
#define ENV_VAR_CONNECT_MAX_RETRIES    "GLADIUS_TOOL_FE_CONNECT_MAX_RETRIES"

static const std::vector<core::EnvironmentVar> compEnvVars = {
    {ENV_VAR_CONNECT_TIMEOUT_IN_SEC,
     "Connection timeout in seconds. Default: " +
     std::to_string(ToolFE::sDefaultTimeout) + "."
    },
    {ENV_VAR_CONNECT_MAX_RETRIES,
     "Maximum number of connection retries. Default: " +
     std::to_string(ToolFE::sDefaultMaxRetries) + "."
    }
};

/**
 *
 */
void
echoLaunchStart(
    const gladius::core::Args &largs,
    const gladius::core::Args &aargs
) {
    // Construct the entire launch command.
    auto argv = largs.toArgv();
    const auto aArgv = aargs.toArgv();
    argv.insert(end(argv), begin(aArgv), end(aArgv));
    // To a single string
    string lstr;
    for (const auto & arg : argv) {
        lstr += (arg + " ");
    }
    GLADIUS_COUT_STAT << "Launch Sequence Initiated..." << std::endl;
    GLADIUS_COUT_STAT << "Starting: " << lstr << std::endl;
}
} // end namespace

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/**
 * Tool front-end constructor.
 */
ToolFE::ToolFE(
    void
) : mBeVerbose(false)
  , mConnectionTimeoutInSec(toolcommon::unlimitedTimeout)
  , mMaxRetries(toolcommon::unlimitedRetries)
  , mPathToPluginPack("")
{
    memset(mSessionKey, '\0', sizeof(mSessionKey));
}

/**
 * Component registration.
 */
void
ToolFE::registerComponent(void)
{
    // Register this component's environment variables with the central
    // registry.
    core::Environment::TheEnvironment().addToRegistry(
        CNAME,
        compEnvVars
    );
}

/**
 *
 */
int
ToolFE::mGetStateFromEnvs(void)
{
    if (core::utils::envVarSet(GLADIUS_ENV_TOOL_FE_VERBOSE_NAME)) {
        mBeVerbose = true;
    }
    //
    auto rc = core::utils::getEnvAs(
                  ENV_VAR_CONNECT_MAX_RETRIES,
                  mMaxRetries
              );
    // Not set, so default to some number of retries.
    if (GLADIUS_ENV_NOT_SET == rc) {
        mMaxRetries = sDefaultMaxRetries;
    }
    else if (GLADIUS_SUCCESS != rc) {
        GLADIUS_CERR << "Error encountered while trying to "
                        "read environment variable: "
                     << ENV_VAR_CONNECT_MAX_RETRIES << std::endl;
        return rc;
    }
    if (mMaxRetries <= 0) mMaxRetries = toolcommon::unlimitedRetries;
    //
    rc = core::utils::getEnvAs(
             ENV_VAR_CONNECT_TIMEOUT_IN_SEC,
             mConnectionTimeoutInSec
         );
    // Not set, so default to some timeout.
    if (GLADIUS_ENV_NOT_SET == rc) {
        mConnectionTimeoutInSec = sDefaultTimeout;
    }
    else if (GLADIUS_SUCCESS != rc) {
        GLADIUS_CERR << "Error encountered while trying to "
                        "read environment variable: "
                     << ENV_VAR_CONNECT_TIMEOUT_IN_SEC << std::endl;
        return rc;
    }
    if (mConnectionTimeoutInSec <= 0) {
        mConnectionTimeoutInSec = toolcommon::unlimitedTimeout;
    }
    //
    return GLADIUS_SUCCESS;
}

/**
 * Returns whether or not the tool-fe's environment setup is sane.
 */
int
ToolFE::mSetupCore(void)
{
    string whatsWrong;
    static const auto envMode = GLADIUS_ENV_DOMAIN_MODE_NAME;
    int rc = GLADIUS_SUCCESS;
    //
    if (GLADIUS_SUCCESS != (rc = mGetStateFromEnvs())) {
        return rc;
    }
    //
    if (!core::utils::envVarSet(envMode)) {
        whatsWrong = "Cannot determine current mode.\nPlease set '"
                   + string(envMode) +  "' and try again.";
        GLADIUS_CERR << whatsWrong << endl;
        return GLADIUS_ERR;
    }
    auto modeName = core::utils::getEnv(envMode);
    // Initialize the plugin manager.
    mPluginManager = gpa::GladiusPluginManager(modeName, mBeVerbose);
    // The path to the plugin pack if we find a usable one.
    string pathToPluginPackIfAvail;
    if (!mPluginManager.pluginPackAvailable(pathToPluginPackIfAvail)) {
        // TODO Improve. Provide an example.
        whatsWrong = "Cannot find a usable plugin pack for '"
                   + modeName + "'.\nPlease make sure that the directory "
                     "where this plugin pack lives is in\n"
                     GLADIUS_ENV_PLUGIN_PATH_NAME " and all required plugins "
                     "are installed." ;
        GLADIUS_CERR << whatsWrong << endl;
        return GLADIUS_ERR;
    }
    // Set member, so we can get the plugin pack later...
    mPathToPluginPack = pathToPluginPackIfAvail;
    //
    return mInitializeParallelLauncher();
}

/**
 *
 */
int
ToolFE::mPreToolInitActons(void)
{
#if 0
    // Dup here before we start tool infrastructure lash-up. Someone in
    // there makes stdio act funny. This is a workaround to fix that.
    mStdInCopy = dup(STDIN_FILENO);
    if (-1 == mStdInCopy) {
        int err = errno;
        GLADIUS_CERR << "dup(2): " + core::utils::getStrError(err) << endl;
        return GLADIUS_ERR;
    }
    if (-1 == close(STDIN_FILENO)) {
        int err = errno;
        GLADIUS_CERR << "close(2): " + core::utils::getStrError(err) << endl;
        return GLADIUS_ERR;
    }
#endif
    return GLADIUS_SUCCESS;
}

/**
 *
 */
int
ToolFE::mPostToolInitActons(void)
{
#if 0
    // Restore stdin. This is the counterpart to the workaround in
    // mPreToolInitActons.
    if (-1 == dup2(mStdInCopy, STDIN_FILENO)) {
        int err = errno;
        GLADIUS_CERR << "Call to dup2(2) failed: "
                     << core::utils::getStrError(err) << endl;
        return GLADIUS_ERR;
    }
    close(mStdInCopy);
#endif
    return GLADIUS_SUCCESS;
}

/**
 *
 */
int
ToolFE::mInitializeParallelLauncher(void)
{
    int rc = GLADIUS_SUCCESS;
    if (GLADIUS_SUCCESS != (rc = mLauncherPersonality.init(mLauncherArgs))) {
        return rc;
    }
    VCOMP_COUT(
        "Application launcher personality: " <<
        mLauncherPersonality.getPersonalityName() << endl
    );
    VCOMP_COUT("Which launcher: " << mLauncherPersonality.which() << endl);
    return rc;
}

/**
 * Responsible for running the tool front-end instance. This is the tool-fe
 * entry point from a caller's perspective.
 */
int
ToolFE::main(
    const core::Args &appArgv,
    const core::Args &launcherArgv
) {
    VCOMP_COUT("Entering main." << endl);
    int rc = GLADIUS_SUCCESS;
    //
    try {
        mAppArgs = appArgv;
        mLauncherArgs = launcherArgv;
        // Make sure that all the required bits are set before we get to
        // launching anything.
        if (GLADIUS_SUCCESS != (rc = mSetupCore())) return rc;
        // Perform any actions that need to take place before lash-up.
        if (GLADIUS_SUCCESS != (rc = mPreToolInitActons())) return rc;
        // Figure out the relevant job characteristics.
        if (GLADIUS_SUCCESS != (rc = mDetermineProcLandscape())) return rc;
        // Build MRNet network.
        if (GLADIUS_SUCCESS != (rc = mBuildNetwork())) return rc;
        // Start lash-up.
        if (GLADIUS_SUCCESS != (rc = mInitiateToolLashUp())) return rc;
        //
        if (GLADIUS_SUCCESS != (rc = mPostToolInitActons())) return rc;
        // Now that the base infrastructure is up, now load the user-specified
        // plugin pack.
        if (GLADIUS_SUCCESS != (rc = mLoadPlugins())) return rc;
        // Let the BEs know what plugins they are loading.
        if (GLADIUS_SUCCESS != (rc = mSendPluginInfoToBEs())) return rc;
        // Now turn it over to the plugin.
        if (GLADIUS_SUCCESS != (rc = mEnterPluginMain())) return rc;
    }
    catch (const exception &e) {
        GLADIUS_THROW(e.what());
    }
    //
    return rc;
}

/**
 *
 */
int
ToolFE::mDetermineProcLandscape(void)
{
    VCOMP_COUT("Determining process landscape..." << endl);
    //
    int rc = GLADIUS_SUCCESS;
    try {
        if (GLADIUS_SUCCESS !=
            (rc = mDSI.init(mLauncherPersonality, mBeVerbose))) {
            return rc;
        }
        if (GLADIUS_SUCCESS !=
            (rc = mDSI.getProcessLandscape(mProcLandscape))) {
            return rc;
        }
        //
        GLADIUS_COUT_STAT
             << "::: Job Statistics :::::::::::::::::::::::::::::::::";
        cout << endl;
        GLADIUS_COUT_STAT << "Number of Application Processes: "
                          << mProcLandscape.nProcesses() << endl;
        GLADIUS_COUT_STAT << "Number of Hosts                : "
                          << mProcLandscape.nHosts() << endl;
        GLADIUS_COUT_STAT
             << "::::::::::::::::::::::::::::::::::::::::::::::::::::";
        cout << endl;
    }
    catch (const exception &e) {
        GLADIUS_THROW(e.what());
    }
    //
    return rc;
}

/**
 *
 */
int
ToolFE::mBuildNetwork(void)
{
    int rc = GLADIUS_SUCCESS;
    try {
        if (GLADIUS_SUCCESS != (rc = mMRNFE.init(mBeVerbose))) return rc;
        // Create MRNet network FE.
        if (GLADIUS_SUCCESS != (rc = mMRNFE.createNetworkFE(mProcLandscape))) {
            return rc;
        }
    }
    catch (const exception &e) {
        GLADIUS_THROW(e.what());
    }
    return rc;
}

/**
 *
 */
int
ToolFE::mConnectMRNetTree(void)
{
    using namespace core;
    // TODO add a timer here
    decltype(mMaxRetries) attempt = 0;
    bool connectSuccess = false;
    do {
        VCOMP_COUT("Connection attempt: " << attempt << endl);
        // Take a break and let things happen...
        sleep(1);
        // Try to connect.
        const int status = mMRNFE.connect();
        // All done - Get outta here...
        if (GLADIUS_SUCCESS == status) {
            connectSuccess = true;
            break;
        }
        // Something bad happened.
        else if (GLADIUS_NOT_CONNECTED != status) {
            GLADIUS_CERR << utils::formatCallFailed(
                                "MRNetFE::connect", GLADIUS_WHERE
                            )
                         << endl;
            return GLADIUS_ERR;
        }
        // Unlimited retries, so just continue.
        if (toolcommon::unlimitedRetries == mMaxRetries) continue;
        if (++attempt >= mMaxRetries) {
            GLADIUS_CERR << "Giving up after " << attempt
                         << ((attempt > 1) ? " attempts. " : " attempt. ")
                         << "Not all tool processes reported back..." << endl;
            return GLADIUS_ERR;
        }
    } while (true);
    //
    if (connectSuccess) {
        GLADIUS_COUT_STAT << "MRNet Network Connected." << endl;
    }
    else {
        GLADIUS_CERR << "Could not setup mrnet network." << endl;
        return GLADIUS_ERR;
    }
    //
    return GLADIUS_SUCCESS;
}

/**
 * Construct a list of environment variables that we would like the parallel
 * launcher to forward to the remote environments during target application
 * startup.
 */
vector <pair<string, string> >
ToolFE::mForwardEnvsToBEsIfSetOnFE(void)
{
    // Environment variable forwarding to daemons. Note that this is a complete
    // list of environment variables that we would forward if they are set
    // within the tool front-end's environment.
    static const vector<string> envVars = {
        GLADIUS_ENV_GLADIUS_SESSION_KEY,
        GLADIUS_ENV_TOOL_BE_LOG_DIR_NAME,
        GLADIUS_ENV_TOOL_BE_VERBOSE_NAME
    };
    //
    vector <pair<string, string> > envTups;
    // If the environment variable is set, then capture its value.
    for (const auto &envVar : envVars) {
        // Filter out those that are not set.
        if (core::utils::envVarSet(envVar)) {
            envTups.push_back(make_pair(envVar, core::utils::getEnv(envVar)));
        }
    }
    //
    return envTups;
}

/**
 *
 */
int
ToolFE::mPublishConnectionInfo(void)
{
    VCOMP_COUT("Publishing connection information..." << endl);
    // Set session key. Not ideal, but good enough for now...
    snprintf(
        mSessionKey,
        sizeof(mSessionKey),
        "%s-%s-%s",
        "gladius",
        utils::getHostname().c_str(),
        to_string(getpid()).c_str()
    );
    // First get the connection map from the TBON.
    int rc = GLADIUS_SUCCESS;
    vector<toolcommon::ToolLeafInfoT> leafInfos;
    if (GLADIUS_SUCCESS != (rc = mMRNFE.generateConnectionMap(leafInfos))) {
        return rc;
    }
    // We better have at least one item here.
    assert(leafInfos.size() > 0);
    // Serialize the data
    vector<string> sLeafInfos;
    // Serialization buffer. Items should all be the same size.
    char sBuf[sizeof(toolcommon::ToolLeafInfoT)];
    for (const auto &li : leafInfos) {
        memset(sBuf, 0, sizeof(sBuf));
        memmove(sBuf, &li, sizeof(sBuf));
        const string tmp(sBuf, sizeof(sBuf));
        // Stash encoded buffer.
        sLeafInfos.push_back(utils::base64Encode(tmp));
    }
    // Now pushlish to distributed resources
    if (GLADIUS_SUCCESS !=
        (rc = mDSI.publishConnectionInfo(mSessionKey, sLeafInfos))) {
        return rc;
    }
    // Done with DSI, so shut it down
    if (GLADIUS_SUCCESS != (rc = mDSI.shutdown())) {
        return rc;
    }
    //
    return GLADIUS_SUCCESS;
}

/**
 *
 */
int
ToolFE::mLaunchUserApp(void)
{
    // Push session key into the environment.
    int rc = utils::setEnv(
                 GLADIUS_ENV_GLADIUS_SESSION_KEY,
                 mSessionKey
             );
    if (GLADIUS_SUCCESS != rc) return rc;
    // TODO FIXME
    vector<string> argv = mLauncherArgs.toArgv();
    vector<string> aargv = mAppArgs.toArgv();
    argv.insert(end(argv), begin(aargv), end(aargv));
    core::Args a(argv);
    //
    pid_t p = fork();
    // child
    if (0 == p) {
        execvp(a.argv()[0], a.argv());
        perror("execvp");
        _exit(127);
    }
    else if (-1 == p) {
        return GLADIUS_ERR;
    }
    //
    return GLADIUS_SUCCESS;
}

/**
 * Initiates the tool lash-up bits.
 */
int
ToolFE::mInitiateToolLashUp(void)
{
    VCOMP_COUT("Initiating tool lashup..." << endl);
    try {
        echoLaunchStart(mLauncherArgs, mAppArgs);
        // Publish connection information across the system.
        int rc = mPublishConnectionInfo();
        if (GLADIUS_SUCCESS != rc) return rc;
        // Launch user application containing links into our tool
        // infrastructure.
        if (GLADIUS_SUCCESS != (rc = mLaunchUserApp())) return rc;
        // Wait for MRNet tree connections.
        if (GLADIUS_SUCCESS != (rc = mConnectMRNetTree())) return rc;
        // Setup connected MRNet network for core infrastructure.
        if (GLADIUS_SUCCESS != (rc = mMRNFE.networkInit())) return rc;
        // Make sure that our core filters are working by performing a handshake
        // between the tool front-end and all the tool leaves (where all
        // communication is going through a set of core filters).
        if (GLADIUS_SUCCESS != (rc = mMRNFE.handshake())) return rc;
    }
    catch (const exception &e) {
        GLADIUS_THROW(e.what());
    }
    //
    return GLADIUS_SUCCESS;
}

/**
 *
 */
int
ToolFE::mLoadPlugins(void)
{
    VCOMP_COUT("Loading plugins..." << endl);
    // Get the front-end plugin pack.
    mPluginPack = mPluginManager.getPluginPackFrom(
                      gpa::GladiusPluginPack::PluginFE,
                      mPathToPluginPack
                  );
    auto *fePluginInfo = mPluginPack.pluginInfo;
    GLADIUS_COUT_STAT << "Plugin Info:"  << endl;
    GLADIUS_COUT_STAT << "*Name      : " << fePluginInfo->pluginName << endl;
    GLADIUS_COUT_STAT << "*Version   : " << fePluginInfo->pluginVersion << endl;
    GLADIUS_COUT_STAT << "*Plugin ABI: " << fePluginInfo->pluginABI << endl;
    mFEPlugin = fePluginInfo->pluginConstruct();
    //
    return GLADIUS_SUCCESS;
}

/**
 *
 */
int
ToolFE::mSendPluginInfoToBEs(void)
{
    VCOMP_COUT("Sending plugin info to back-ends..." << endl);
    // MRNet knows how to do this...
    return mMRNFE.pluginInfoBCast(
        string(mPluginPack.pluginInfo->pluginName),
        mPathToPluginPack
    );
}

/**
 *
 */
int
ToolFE::mEnterPluginMain(void)
{
    VCOMP_COUT("Entering plugin main..." << endl);

    try {
        gpi::GladiusPluginArgs pluginArgs(
            mPathToPluginPack,
            mAppArgs,
            mMRNFE.getProtoStream(),
            mMRNFE.getNetwork()
        );
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        // Front-end Plugin Entry Point.
        mFEPlugin->pluginMain(pluginArgs);
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
    }
    catch (const exception &e) {
        GLADIUS_THROW(e.what());
    }
    //
    return GLADIUS_SUCCESS;
}
