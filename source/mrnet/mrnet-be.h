/**
 * Copyright (c) 2014-2015 Los Alamos National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 *
 * Copyright (c) 2003-2012 Dorian C. Arnold, Philip C. Roth, Barton P. Miller
 * Detailed MRNet usage rights in "LICENSE" file in the MRNet distribution.
 *
 */

#ifndef GLADIUS_MRNET_MRNET_BE_H_INCLUDED
#define GLADIUS_MRNET_MRNET_BE_H_INCLUDED

#include "mrnet/MRNet.h"

#include "tool-common/tool-common.h"

namespace gladius {
namespace mrnetbe {
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/**
 * Implements the MRNet interface for a tool front-end.
 */
class MRNetBE {
private:

public:
    MRNetBE(void);
    //
    ~MRNetBE(void);
};

} // end mrnetbe namespace
} // end gladius namespace

#endif