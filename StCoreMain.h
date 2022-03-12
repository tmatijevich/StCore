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

/* Constants */
#define stCORE_LOGBOOK_NAME "StCoreLog"
#define stCORE_LOGBOOK_FACILITY 1
#define stCORE_COMMANDBUFFER_SIZE 4U
#define stCORE_SECTION_MAX 64

/* Macros */
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x]))))) /* https://stackoverflow.com/a/1598827 */
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define ROUND_UP_MULTIPLE(x,y) ((x) / (y) + (size_t)((x) % (y) != 0)) * (y)
#define GET_BIT(x,y) ((x) & 1U << (y) ? true : false)
#define SET_BIT(x,y) ((x) |= 1U << (y))
#define CLEAR_BIT(x,y) ((x) &= ~(1U << (y)))
#define TOGGLE_BIT(x,y) ((x) ^= 1U << (y))

/* Global variables */
extern unsigned char *pCyclicControlData, *pCyclicStatusData;
extern SuperTrakControlIfConfig_t coreControlInterfaceConfig;

/* Function prototypes */
void StCoreLogMessage(UserLogSeverityEnum severity, unsigned short code, char *message);
void StCoreFormatMessage(UserLogSeverityEnum severity, unsigned short code, char *message, FormatStringArgumentsType *args);
unsigned short StCoreEventCode(long eventID);

#ifdef __cplusplus
};
#endif

#endif /* ST_CORE_MAIN_H */
