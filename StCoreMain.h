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
#define ROUND_UP_MULTIPLE(x,y) ((x) / (y) + (size_t)((x) % (y) != 0)) * (y)

/* Global variables */
extern SuperTrakControlIfConfig_t configPLCInterface;
extern unsigned char configUserPalletCount, configUserNetworkIOCount, configError;
extern unsigned char *control, *status;

/* Function prototypes */
void StCoreLogPosition(enum SuperTrakPositionErrorEnum error, struct SuperTrakPositionInfoType info);
long StCoreLogServChan(unsigned short result, unsigned short parameter);

#ifdef __cplusplus
};
#endif

#endif /* ST_CORE_MAIN_H */
