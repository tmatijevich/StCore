/*******************************************************************************
 * File: StCore\Main.h
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
#include "StCore.h" /* Automatically generated by Automation Studio for the IEC library */

/********* 
 Constants 
*********/
#define CORE_LOGBOOK_NAME 					"StCoreLog"
#define CORE_LOGBOOK_FACILITY 				1
#define CORE_FORMAT_SIZE 					125
#define CORE_FORMAT_ARGUMENT_COUNT 			6
#define CORE_FORMAT_ARGUMENT_SIZE 			81
#define CORE_FAULT_INSTANCE_MAX 			64 		/* Up to 64 instances of faults/warnings per context */
#define CORE_FAULT_DETAIL_MAX 				4 		/* Four 32-bit signed detail data storage per fault instance */
#define CORE_FAULT_MAX 						32 		/* 32 faults and 32 warnings per context */
#define CORE_COMMAND_COUNT 					48 		/* Default value, max is 64 */
#define CORE_COMMAND_BYTE_MAX 				8 		/* 64 commands max (8 bytes max) */
#define CORE_COMMAND_BUFFER_SIZE 			4U
#define CORE_SECTION_MAX 					64 		/* SuperTrak is allowed up to 64 gateway communication boards */
#define CORE_SECTION_ADDRESS_MAX 			99 		/* Users can number sections with 1-99 */
#define CORE_SECTION_SENSOR_MAX 			16 		/* 16 sensor values are available per section (some are reserved) */
#define CORE_TARGET_MAX 					256 	/* Users can define up to 255 targets (target 0 is a placeholder) */
#define CORE_PALLET_MAX 					256 	/* SuperTrak memory structure has up to 256 pallets */
#define CORE_PALLET_ID_MAX 					254 	/* Users can number pallets 1-254, 0 for unidentified */
#define CORE_CYCLE_TIME 					800U 	/* 800 us cycle time */
#define CORE_COMMAND_TIMEOUT 				500000U /* 500 ms command request timeout */
#define CORE_TARGET_RELEASE_PER_BYTE 		4U
#define CORE_TARGET_RELEASE_BIT_COUNT 		2U
#define CORE_COMMAND_DATA_BYTE_COUNT 		8U
#define CORE_COMMAND_FLAG_PER_BYTE 			8U 		/* This is an internal count to store flags for each command channel */
#define CORE_TARGET_STATUS_BYTE_COUNT 		3U
#define CORE_COMMAND_STATUS_PER_BYTE 		4U
#define CORE_COMMAND_STATUS_BIT_COUNT 		2U
#define CORE_NETWORK_IO_PER_BYTE 			8U
/* These command IDs are defined by the SuperTrak PLC communication protocol */
#define CORE_COMMAND_ID_RELEASE 			16 		/* 16, 17, 18, 19 target/pallet? left/right? */
#define CORE_COMMAND_ID_OFFSET 				24 		/* 24, 25, 26, 27 */
#define CORE_COMMAND_ID_INCREMENT 			28 		/* 28, 29, 30, 31 */
#define CORE_COMMAND_ID_CONTINUE 			60 		/* 60, 62 */
#define CORE_COMMAND_ID_PALLET_ID 			64 		/* 64 */
#define CORE_COMMAND_ID_MOTION 				68 		/* 68, 70 target/pallet? */
#define CORE_COMMAND_ID_MECHANICAL 			72 		/* 72, 74 */
#define CORE_COMMAND_ID_CONTROL 			76  	/* 76, 78 */

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
typedef enum coreLogSeverityEnum {
	CORE_LOG_SEVERITY_ERROR = 0,
	CORE_LOG_SEVERITY_WARNING,
	CORE_LOG_SEVERITY_INFO,
	CORE_LOG_SEVERITY_SUCCESS,
	CORE_LOG_SEVERITY_DEBUG
} coreLogSeverityEnum;

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

typedef enum coreCommandSelectEnum { /* List of commands, do not exceed 0-15 */
	CORE_COMMAND_SIMPLE = 0,
	CORE_COMMAND_RELEASE,
	CORE_COMMAND_OFFSET,
	CORE_COMMAND_INCREMENT,
	CORE_COMMAND_CONTINUE,
	CORE_COMMAND_ID,
	CORE_COMMAND_MOTION,
	CORE_COMMAND_MECHANICAL,
	CORE_COMMAND_CONTROL
} coreCommandSelectEnum;

/**********
 Structures
**********/
/* String format type */
typedef struct coreFormatArgumentType {
	unsigned char b[CORE_FORMAT_ARGUMENT_COUNT]; /* Boolean */
	signed long i[CORE_FORMAT_ARGUMENT_COUNT]; /* Integer */
	double f[CORE_FORMAT_ARGUMENT_COUNT]; /* Floating point */
	char s[CORE_FORMAT_ARGUMENT_COUNT][CORE_FORMAT_ARGUMENT_SIZE]; /* Strings */
} coreFormatArgumentType;

/* Command management */
typedef struct coreCommandCreateType {
	unsigned char commandID; /* See TrakMaster help's PLC control interface for available IDs */
	unsigned char context; /* Target or pallet */
	unsigned char index; /* Manager index for allocated pallet command buffer */
} coreCommandCreateType;

typedef struct coreCommandType {
	SuperTrakCommand_t command; /* SuperTrak command data */
	unsigned char status; /* Command progess status */
	void *pInstance; /* Record instance if called from function block */
} coreCommandType;

typedef struct coreCommandBufferType {
	unsigned char read; /* Index to execute user requests */
	unsigned char write; /* Index to submit user requests */
	unsigned char channel; /* Cyclic command channel index */
	coreCommandType buffer[CORE_COMMAND_BUFFER_SIZE]; /* Command buffer */
} coreCommandBufferType;

/* Global private structure */
struct coreGlobalType {
	unsigned char *pCyclicControl;
	unsigned char *pCyclicStatus;
	coreCommandType *pSimpleRelease;
	coreCommandBufferType *pCommandBuffer;
	SuperTrakControlIfConfig_t interface;
	signed char sectionMap[UCHAR_MAX + 1]; /* Map user address 1-99 to offset 0-63, -1 for unused */
	signed short palletMap[UCHAR_MAX + 1]; /* Map pallet ID 1-254 to memory structure 0-255, -1 for unused */
	SuperTrakPalletInfo_t *pPalletData;
	unsigned char targetCount;
	unsigned char palletCount;
	unsigned char networkIOCount;
	unsigned char ready;
	unsigned char error;
	long statusID;
	ArEventLogIdentType ident;
	unsigned char debug;
};

/****************
 Global Variables
****************/
extern struct coreGlobalType core;

/******************* 
 Function Prototypes 
*******************/
/* Logging and string handling */
char* coreStringCopy(char *destination, const char *source, unsigned long size);
unsigned long coreFormat(char *str, unsigned long size, char *format, coreFormatArgumentType *args);
long coreLog(ArEventLogIdentType ident, coreLogSeverityEnum severity, unsigned char facility, unsigned short code, char *object, char *message, coreFormatArgumentType *args);
unsigned short coreLogCode(long event);
void coreLogServiceChannel(unsigned short result, unsigned short parameter, char *object);
void coreMonitorSuperTrakFault(void);

/* Commands */
long coreSimpleRelease(unsigned char target, unsigned char localMove, void *pInstance, coreCommandType **ppCommand);
long coreReleasePallet(unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, void *pInstance, coreCommandType **ppCommand);
long coreReleaseTargetOffset(unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, double targetOffset, void *pInstance, coreCommandType **ppCommand);
long coreReleaseIncrementalOffset(unsigned char target, unsigned char pallet, double incrementalOffset, void *pInstance, coreCommandType **ppCommand);
long coreContinueMove(unsigned char target, unsigned char pallet, void *pInstance, coreCommandType **ppCommand);
long coreSetPalletID(unsigned char target, unsigned char palletID, void *pInstance, coreCommandType **ppCommand);
long coreSetMotionParameters(unsigned char target, unsigned char pallet, double velocity, double acceleration, void *pInstance, coreCommandType **ppCommand);
long coreSetMechanicalParameters(unsigned char target, unsigned char pallet, double shelfWidth, double centerOffset, void *pInstance, coreCommandType **ppCommand);
long coreSetControlParameters(unsigned char target, unsigned char pallet, unsigned char controlGainSet, double movingFilter, double stationaryFilter, void *pInstance, coreCommandType **ppCommand);

/* Command management */
void coreAssign16(unsigned short *pInteger, unsigned char bit, unsigned char value);
long coreCommandCreate(unsigned char start, unsigned char target, unsigned char pallet, unsigned short direction, coreCommandCreateType *create);
long coreCommandRequest(unsigned char index, SuperTrakCommand_t command, void *pInstance, coreCommandType **ppCommand);
void coreCommandManager(void);

/* Miscellaneous */
void coreAssignUInt16(unsigned short *pInt, unsigned char bit, unsigned char value);

#ifdef __cplusplus
};
#endif

#endif /* ST_CORE_MAIN_H */
