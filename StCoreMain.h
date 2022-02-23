/*******************************************************************************
 * File: StCoreMain.h
 * Author: Tyler Matijevich
 * Date: 2022-02-15
********************************************************************************
 * Description: Internal header addendum to the automatically generated StCore.h
*******************************************************************************/

#ifndef ST_CORE_MAIN_H
#define ST_CORE_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Headers */
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include "StCore.h"

/* Macros */
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x]))))) /* https://stackoverflow.com/a/1598827 */
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

/* Type definitions */
struct StCorePLCControlIFConfig_typ {
	unsigned short options; /* (Par 1430) */
	unsigned short sectionStart; /* (Par 1431) */
	unsigned short sectionCount; /* (Par 1432) */
	unsigned short targetStart; /* (Par 1433) */
	unsigned short targetCount; /* (Par 1434) */
	unsigned short commandCount; /* (Par 1436) */
	unsigned short networkIOStart; /* (Par 1437) */
	unsigned short networkIOCount; /* (Par 1438) */
	unsigned short revision; /* (Par 1444) */
};

/* Function prototypes */
void StCoreLogPosition(enum SuperTrakPositionErrorEnum error, struct SuperTrakPositionInfoType info);
long StCoreLogServChan(unsigned short result, unsigned short parameter);

#ifdef __cplusplus
};
#endif

#endif /* ST_CORE_MAIN_H */
