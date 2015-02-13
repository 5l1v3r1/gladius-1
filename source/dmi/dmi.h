/*
 * Copyright (c) 2015      Los Alamos National Security, LLC
 *                         All rights reserved.
 *
 * This file is part of the Gladius project. See the LICENSE.txt file at the
 * top-level directory of this distribution.
 */

/**
 * The Debugger Machine Interface (DMI).
 */

#ifndef GLADIUS_DMI_DMI_H_INCLUDED
#define GLADIUS_DMI_DMI_H_INCLUDED

#include "mi_gdb.h"

namespace gladius {
namespace dmi {

/**
 * Debugger Machine Interface.
 */
class DMI {
public:
    //
    DMI(void);
    //
    ~DMI(void);
};


} // end dmi namespace
} // end gladius namespace

#endif