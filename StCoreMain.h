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
#define CORE_LOGBOOK_NAME "StCoreLog"
#define CORE_LOGBOOK_FACILITY 1
#define CORE_FORMAT_SIZE 125
#define CORE_COMMANDBUFFER_SIZE 4U
#define CORE_SECTION_MAX 64

/* Macros */
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x]))))) /* https://stackoverflow.com/a/1598827 */
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define ROUND_UP_MULTIPLE(x,y) ((x) / (y) + (size_t)((x) % (y) != 0)) * (y)
#define GET_BIT(x,y) ((x) & 1U << (y) ? true : false)
#define SET_BIT(x,y) ((x) |= 1U << (y))
#define CLEAR_BIT(x,y) ((x) &= ~(1U << (y)))
#define TOGGLE_BIT(x,y) ((x) ^= 1U << (y))

/* Structures */
typedef struct coreCommandBufferType {
	unsigned char read; /* Index to execute user requests */
	unsigned char write; /* Index to submit user requests */
	unsigned char full; /* Flag when buffer is full (read = write) */
	unsigned char active; /* The buffer is currently executing a command */
	unsigned long timer; /* Count till timeout */
	SuperTrakCommand_t command[CORE_COMMANDBUFFER_SIZE]; /* Command buffer */
} coreCommandBufferType;

/* Global variables */
extern unsigned char *pCoreCyclicControl, *pCoreCyclicStatus;
extern SuperTrakControlIfConfig_t coreInterfaceConfig;
extern unsigned char coreError, coreTargetCount, corePalletCount, coreNetworkIOCount;
extern long coreStatusID;
extern coreCommandBufferType *pCoreCommandBuffer;

/* Function prototypes */
void coreLogMessage(UserLogSeverityEnum severity, unsigned short code, char *message);
void coreLogFormatMessage(UserLogSeverityEnum severity, unsigned short code, char *message, FormatStringArgumentsType *args);
unsigned short coreEventCode(long eventID);
char* coreStringCopy(char *destination, const char *source, unsigned long size);
void coreLogServiceChannel(unsigned short result, unsigned short parameter);
void coreLogFaultWarning(unsigned char index, unsigned char section);
void coreProcessCommand(void);

#ifdef __cplusplus
};
#endif

#endif /* ST_CORE_MAIN_H */
