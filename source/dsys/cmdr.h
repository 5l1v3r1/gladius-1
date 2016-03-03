/**
 * Copyright (c)      2016 Los Alamos National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 */

/**
 * Implements generating launch commands for various parallel launchers like
 * orterun, aprun, srun, etc.
 */
#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "core/ret-codes.h"
#include "core/args.h"
#include "core/utils.h"

#include <vector>
#include <string>

namespace gladius {
namespace cmdr {

class Commandr {
public:
    /**
     * Supported launcher types.
     */
    enum AppLauncherPersonality {
        ORTE, /* orte */
        NONE  /* none specified/unknown */
    };

private:
    // Name of the launcher, e.g. mpirun, srun, aprun
    std::string mName;
    // Absolute that to the launcher.
    std::string mAbsolutePath;
    // The launcher "personality", i.e. what kind of launcher.
    AppLauncherPersonality mPersonality;
    // All arguments supplied to launch request.
    core::Args mLauncherArgs;

public:
    /**
     *
     */
    Commandr(
        void
    ) : mName("")
      , mAbsolutePath("")
      , mPersonality(NONE) { ; }

    /**
     *
     */
    int
    init(const core::Args &args) {
        mLauncherArgs = args;
        // First argument should be launcher name
        mName = mLauncherArgs.argv()[0];
        mPersonality = getPersonalityByName(mName);
        //
        if (cmdr::Commandr::NONE == mPersonality) {
            static const std::string errs =
                "Cannot determine launcher type by name: '" + mName + "'";
            GLADIUS_CERR << errs << std::endl;
            return GLADIUS_ERR;
        }
        //
        auto status =  core::utils::which(mName, mAbsolutePath);
        if (GLADIUS_SUCCESS != status) {
            static const std::string errs =
                "It appears as if " + std::string(mName) + " is either "
                "not installed or not in your $PATH. "
                " Please fix this and try again.";
            GLADIUS_CERR << errs << std::endl;
            return GLADIUS_ERR;
        }
        return GLADIUS_SUCCESS;
    }

    /**
     *
     */
    ~Commandr(void) { ; }

    /**
     *
     */
    AppLauncherPersonality
    getPersonality(void) const { return mPersonality; }

    /**
     *
     */
    std::string
    getPersonalityName(void) const {
        switch(mPersonality) {
            // TODO mpich v. open mpi's mpirun? Add a more robust check here.
            case (ORTE): return "orte";
            case (NONE): return "none";
            default: return "???";
        }
    }

    /**
     *
     */
    std::string
    which(void) const { return mAbsolutePath; }

    /**
     *
     */
    core::Args
    getLaunchCMDFor(
        const core::Args &appArgs
    ) {
        using namespace std;
        //
        vector<string> args  = mLauncherArgs.toArgv();
        vector<string> aargs = appArgs.toArgv();
        args.insert(end(args), begin(aargs), end(aargs));
        //
        return core::Args(args);
    }

    /**
     * Returns personality based on launcher name.
     */
    static AppLauncherPersonality
    getPersonalityByName(const std::string &name) {
        // TODO deal with all mpiruns
        if ("mpirun" == name) return ORTE;
        else return NONE;
    }
};

} // end gladius cmdr
} // end gladius namespace
