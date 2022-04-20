/*******************************************************************************
 * File: StCoreMain.h
 * Author: Tyler Matijevich
 * Date: 2022-02-15
********************************************************************************
 * Description: Internal header with private constants, types, and functions
*******************************************************************************/

#ifndef ST_CORE_MAIN_H
#define ST_CORE_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/******* 
 Headers 
*******/
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include "StCore.h"

/********* 
 Constants 
*********/
#define CORE_LOGBOOK_NAME "StCoreLog"
#define CORE_LOGBOOK_FACILITY 1
#define CORE_FORMAT_SIZE 125
#define CORE_COMMAND_COUNT 48
#define CORE_COMMANDBUFFER_SIZE 4U
#define CORE_SECTION_MAX 64

/******
 Macros
******/
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x]))))) /* https://stackoverflow.com/a/1598827 */
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define ROUND_UP_MULTIPLE(x,y) ((x) / (y) + (size_t)((x) % (y) != 0)) * (y)
#define GET_BIT(x,y) ((x) & 1U << (y) ? true : false)
#define SET_BIT(x,y) ((x) |= 1U << (y))
#define CLEAR_BIT(x,y) ((x) &= ~(1U << (y)))
#define TOGGLE_BIT(x,y) ((x) ^= 1U << (y))

/************
 Enumerations
************/
typedef enum coreCommandStatusEnum { /* Command states for status byte, do not exceed 0-7 */
	CORE_COMMAND_PENDING = 0, /* The command request is pending */
	CORE_COMMAND_BUSY, /* The command request is executing */
	CORE_COMMAND_DONE, /* The command request has been acknowledged */
	CORE_COMMAND_ERROR = 7 /* The command request has acknowledged with error */
} coreCommandStatusEnum;

typedef enum coreFunctionStateEnum {
	CORE_FUNCTION_DISABLED = 0, /* The function block is disabled */
	CORE_FUNCTION_EXECUTING, /* The function block is executing successfully */
	CORE_FUNCTION_ERROR = 255 /* The function block is enabled but in error */
} coreFunctionStateEnum;

/**********
 Structures
**********/
/* Command management */
typedef struct coreCommandCreateType {
	unsigned char commandID; /* See TrakMaster help's PLC control interface for available IDs */
	unsigned char context; /* Target or pallet */
	unsigned char index; /* Manager index for allocated pallet command buffer */
} coreCommandCreateType;

typedef struct coreCommandType {
	SuperTrakCommand_t command; /* SuperTrak command bytes */
	unsigned char status; /* Command progess status */
	void *pInstance; /* Record instance if called from function block */
} coreCommandType;

typedef struct coreCommandBufferType {
	unsigned char read; /* Index to execute user requests */
	unsigned char write; /* Index to submit user requests */
	unsigned long timer; /* Count until timeout */
	coreCommandType buffer[CORE_COMMANDBUFFER_SIZE]; /* Command buffer */
} coreCommandBufferType;

/* Global private structure */
struct coreGlobalType {
	unsigned char *pCyclicControl;
	unsigned char *pCyclicStatus;
	coreCommandBufferType *pCommandBuffer;
	SuperTrakControlIfConfig_t interface;
	unsigned char targetCount;
	unsigned char palletCount;
	unsigned char networkIOCount;
	unsigned char error;
	long statusID;
};

/****************
 Global variables
****************/
extern unsigned char *pCoreCyclicControl, *pCoreCyclicStatus;
extern SuperTrakControlIfConfig_t coreInterfaceConfig;
extern unsigned char coreError, coreTargetCount, corePalletCount, coreNetworkIOCount;
extern long coreStatusID;
extern coreCommandBufferType *pCoreCommandManager;

/******************* 
 Function prototypes 
*******************/
/* Logging and string handling */
void coreLogMessage(UserLogSeverityEnum severity, unsigned short code, char *message);
void coreLogFormat(UserLogSeverityEnum severity, unsigned short code, char *message, FormatStringArgumentsType *args);
unsigned short coreLogCode(long event);
char* coreStringCopy(char *destination, const char *source, unsigned long size);
void coreLogServiceChannel(unsigned short result, unsigned short parameter);
void coreLogFaultWarning(unsigned char index, unsigned char section);

/* Commands */
long coreReleasePallet(unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, void *pInstance, coreCommandType **ppRequest);

/* Command management */
long coreCommandCreate(unsigned char start, unsigned char target, unsigned char pallet, unsigned short direction, coreCommandCreateType *create);
long coreCommandRequest(unsigned char index, SuperTrakCommand_t command, void *pInstance, coreCommandType **ppRequest);
void coreCommandManager(void);

/* Miscellaneous */
void coreAssignUInt16(unsigned short *pInt, unsigned char bit, unsigned char value);

#ifdef __cplusplus
};
#endif

#endif /* ST_CORE_MAIN_H */
