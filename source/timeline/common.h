#ifndef TIMELINE_COMMON_H
#define TIMELINE_COMMON_H

#define APP_NAME      "timeline"
#define APP_WIN_TITLE "Task Execution Timeline"

// Keep This In Sync With Legion
enum ProcType {
    TOC_PROC,   // Throughput core
    LOC_PROC,   // Latency core
    UTIL_PROC,  // Utility core
    IO_PROC,    // I/O core
    PROC_GROUP, // Processor group
    UNKNOWN     // ???
};

#include <QtGlobal>
// TODO - Grab from user configury.
static const qreal sMicroSecPerPixel = 1e4;

#endif // TIMELINE_COMMON_H
