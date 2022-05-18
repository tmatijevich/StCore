/*******************************************************************************
 * File: StCore\Command.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "Command"

/* Prototypes */
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);
static void getCommand(char *str, unsigned long size, SuperTrakCommand_t data);
static void getContext(char *str, unsigned long size, SuperTrakCommand_t data);
static void getDirection(char *str, unsigned long size, SuperTrakCommand_t data);
static void getParameter(char *str, unsigned long size, SuperTrakCommand_t data);

/* Create command ID, context, and buffer assignment */
long coreCommandCreate(unsigned char start, unsigned char target, unsigned char pallet, unsigned short direction, coreCommandCreateType *create) {
	
	/***********************
	 Declare Local Variables
	***********************/
	unsigned char *pPalletPresent;
	coreFormatArgumentType args;
	SuperTrakCommand_t data;
	
	/*****************
	 Derive Command ID
	*****************/
	/* Add 1 if direction is right, add 2 if pallet context */
	/* Exception: There is no direction choice for configuration (start >= 64) */
	/* Exception: The set pallet ID command can only be in context of target (start != 64) */
	create->commandID = start + (unsigned char)(direction > 0 && start < CORE_COMMAND_ID_PALLET_ID) + 2 * (unsigned char)(target == 0 && start != CORE_COMMAND_ID_PALLET_ID);
	
	/***************
	 Check Reference
	***************/
	data.u1[0] = create->commandID;
	getCommand(args.s[0], sizeof(args.s[0]), data);
	if(core.pCyclicStatus == NULL) {
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "%s command assignment cannot reference cyclic data", &args);
		return stCORE_ERROR_ALLOC;
	}
	
	/*************************
	 Assign Context and Buffer
	*************************/
	if(target != 0) {
		if(target > core.targetCount) {
			args.i[0] = target;
			args.i[1] = core.targetCount;
			logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INPUT), "%s command's target %i context exceeds limits [1, %i]", &args);
			return stCORE_ERROR_INPUT;
		}
			
		pPalletPresent = core.pCyclicStatus + core.interface.targetStatusOffset + CORE_TARGET_STATUS_BYTE_COUNT * target + 1;
		if(*pPalletPresent < 1 || core.palletCount < *pPalletPresent) {
			args.i[0] = target;
			args.i[1] = *pPalletPresent;
			args.i[2] = core.palletCount;
			logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INPUT), "%s command's target %i context with pallet %i present exceeding limits [1, %i]", &args);
			return stCORE_ERROR_INPUT;
		}
		
		create->index = *pPalletPresent;
		create->context = target;
	}
	else {
		if(pallet < 1 || core.palletCount < pallet) {
			args.i[0] = pallet;
			args.i[1] = core.palletCount;
			logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INPUT), "%s command's pallet %i context exceeds limits [1, %i]", &args);
			return stCORE_ERROR_INPUT;
		}
		
		create->index = pallet;
		create->context = pallet;
	}
	
	return 0;
	
} /* End function */

/* Add command to pallet buffer */
long coreCommandRequest(unsigned char index, SuperTrakCommand_t command, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare Local Variables
	***********************/
	coreCommandBufferType *pBuffer;
	coreCommandType *pCommand;
	coreFormatArgumentType args;
	
	/***************
	 Check Reference
	***************/
	if(core.pCommandBuffer == NULL) {
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "%s command request cannot reference command buffer", &args);
		return stCORE_ERROR_ALLOC;
	}
	
	/**********************
	 Access command manager
	**********************/
	pBuffer = core.pCommandBuffer + index - 1;
	pCommand = &pBuffer->buffer[pBuffer->write];
	
	/* Prepare message */
	getContext(args.s[0], sizeof(args.s[0]), command);
	args.i[0] = command.u1[1];
	getCommand(args.s[1], sizeof(args.s[1]), command);
	
	/* Check if pending or busy */
	if(GET_BIT(pCommand->status, CORE_COMMAND_PENDING) || GET_BIT(pCommand->status, CORE_COMMAND_BUSY)) {
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_BUFFER), "%s %i %s command rejected because buffer is full", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	/* Write command */
	memcpy(&pCommand->command, &command, sizeof(pCommand->command));
	
	/* Update status, tag instance, and share entry */
	CLEAR_BIT(pCommand->status, CORE_COMMAND_DONE);
	CLEAR_BIT(pCommand->status, CORE_COMMAND_ERROR);
	SET_BIT(pCommand->status, CORE_COMMAND_PENDING);
	pCommand->pInstance = pInstance;
	if(ppCommand != NULL) *ppCommand = pCommand;
	
	/* Debug comfirmation message */
	getParameter(args.s[2], sizeof(args.s[2]), command);
	logMessage(CORE_LOG_SEVERITY_DEBUG, 5200, "%s %i %s command request (%s)", &args);
	
	/* Increment write index and check if full */
	pBuffer->write = (pBuffer->write + 1) % CORE_COMMAND_BUFFER_SIZE;
	pCommand = &pBuffer->buffer[pBuffer->write];
	if(GET_BIT(pCommand->status, CORE_COMMAND_PENDING) || GET_BIT(pCommand->status, CORE_COMMAND_BUSY)) {
		args.i[0] = index;
		args.i[1] = CORE_COMMAND_BUFFER_SIZE;
		logMessage(CORE_LOG_SEVERITY_WARNING, coreLogCode(stCORE_WARNING_BUFFER), "Pallet %i command buffer is now full (size = %i)", &args);
	}
	
	return 0;
	
} /* End function */

/* Process requested user commands */
void coreCommandManager(void) {
	
	/***********************
	 Declare Local Variables
	***********************/
	static unsigned char logAlloc = true, logPause = true;
	coreCommandBufferType *pBuffer;
	coreCommandType *pCommand;
	SuperTrakCommand_t *pChannel;
	long i, j;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success, pause;
	static unsigned char used[CORE_COMMAND_BYTE_MAX], reset[CORE_COMMAND_BYTE_MAX], channel, start;
	static unsigned long timer[CORE_COMMAND_COUNT];
	coreCommandType *pSimpleCommand; /* Simple target release command storage */
	unsigned char *pTargetRelease, *pTargetStatus, lowerBit, upperBit; /* Simple target release and target status cyclic bits */
	unsigned long *pSimpleReleaseTimer;
	coreFormatArgumentType args;
	
	/****************
	 Check References
	****************/
	if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL || core.pCommandBuffer == NULL || core.pSimpleRelease == NULL) {
		if(logAlloc) {
			logAlloc = false;
			logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "StCoreCyclic (coreCommandManager) is unable to reference cyclic data or command buffer", NULL);
		}
		return;
	}
	else
		logAlloc = true;
	
	/***********************
	 Process Command Buffers
	***********************/
	pause = false; /* Reset pause for channel availability */
	i = start; j = 0;
	while(j++ < core.palletCount) {
		/* Access data */
		pBuffer = core.pCommandBuffer + i; /* Pallet command buffer */
		pCommand = &pBuffer->buffer[pBuffer->read]; /* Next command in pallet buffer */
		
		/* Pallet buffer busy sending command */
		if(GET_BIT(pCommand->status, CORE_COMMAND_BUSY)) {
			/* Access data */
			pTrigger = core.pCyclicControl + core.interface.commandTriggerOffset + pBuffer->channel / CORE_COMMAND_TRIGGER_PER_BYTE; /* Trigger group */
			pChannel = (SuperTrakCommand_t*)(core.pCyclicControl + core.interface.commandDataOffset) + pBuffer->channel;
			pComplete = core.pCyclicStatus + core.interface.commandCompleteOffset + pBuffer->channel / CORE_COMMAND_COMPLETE_PER_BYTE;
			pSuccess = core.pCyclicStatus + core.interface.commandSuccessOffset + pBuffer->channel / CORE_COMMAND_SUCCESS_PER_BYTE;
			complete = GET_BIT(*pComplete, pBuffer->channel % CORE_COMMAND_COMPLETE_PER_BYTE);
			success = GET_BIT(*pSuccess, pBuffer->channel % CORE_COMMAND_SUCCESS_PER_BYTE);
			
			/* Track time */
			timer[pBuffer->channel] += CORE_CYCLE_TIME;
			
			/* Build message */
			getContext(args.s[0], sizeof(args.s[0]), *pChannel);
			args.i[0] = pChannel->u1[1];
			getCommand(args.s[1], sizeof(args.s[1]), *pChannel);
			
			/* Wait for complete or timeout */
			if(complete) {
				/* Confirm success or failure */
				if(success)
					logMessage(CORE_LOG_SEVERITY_DEBUG, 6000, "%s %i %s command acknowledged", &args);
				else {
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_COMMAND), "%s %i %s command execution failed", &args);
					SET_BIT(pCommand->status, CORE_COMMAND_ERROR);
				}
				
				/* Clear cyclic data and timer */
				memset(pChannel, 0, sizeof(*pChannel));
				CLEAR_BIT(*pTrigger, pBuffer->channel % CORE_COMMAND_TRIGGER_PER_BYTE);
				timer[pBuffer->channel] = 0;
				
				/* Reopen channels only after all pallet buffers have all been polled */
				SET_BIT(reset[channel / CORE_COMMAND_TRIGGER_PER_BYTE], channel % CORE_COMMAND_TRIGGER_PER_BYTE);
				
				/* Update command status */
				CLEAR_BIT(pCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pCommand->status, CORE_COMMAND_DONE);
				
				/* Move to next command in buffer */
				pBuffer->read = (pBuffer->read + 1) % CORE_COMMAND_BUFFER_SIZE;
			}
			else if(timer[pBuffer->channel] >= CORE_COMMAND_TIMEOUT) {
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_TIMEOUT), "%s %i %s command execution timed out", &args);
				
				/* Clear cyclic data and timer */
				memset(pCommand, 0, sizeof(*pCommand));
				CLEAR_BIT(*pTrigger, pBuffer->channel % CORE_COMMAND_TRIGGER_PER_BYTE);
				timer[pBuffer->channel] = 0;
				
				/* Update command status */
				CLEAR_BIT(pCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pCommand->status, CORE_COMMAND_DONE);
				SET_BIT(pCommand->status, CORE_COMMAND_ERROR);
				
				/* Move to next command in buffer */
				pBuffer->read = (pBuffer->read + 1) % CORE_COMMAND_BUFFER_SIZE;
			}
		}
		
		/* Pallet buffer has command pending */
		else if(!pause && GET_BIT(pCommand->status, CORE_COMMAND_PENDING)) {
			/* Is the next channel available? */
			if(GET_BIT(used[channel / CORE_COMMAND_TRIGGER_PER_BYTE], channel % CORE_COMMAND_TRIGGER_PER_BYTE)) {
				pause = true; /* Pause for the rest of this loop */
				start = i; /* Remember spot where the pause started */
				if(logPause) {
					logPause = false;
					logMessage(CORE_LOG_SEVERITY_ERROR, 6001, "Pallet command buffers are paused because all channels are in use", NULL);
				}
			}
			else {
				/* Assign the command to the channel */
				pChannel = (SuperTrakCommand_t*)(core.pCyclicControl + core.interface.commandDataOffset) + channel;
				memcpy(pChannel, &pCommand->command, sizeof(SuperTrakCommand_t));
				
				/* Set trigger */
				pTrigger = core.pCyclicControl + core.interface.commandTriggerOffset + channel / CORE_COMMAND_TRIGGER_PER_BYTE;
				SET_BIT(*pTrigger, channel % CORE_COMMAND_TRIGGER_PER_BYTE);
				
				/* Update command channel and status */
				pBuffer->channel = channel;
				CLEAR_BIT(pCommand->status, CORE_COMMAND_PENDING);
				SET_BIT(pCommand->status, CORE_COMMAND_BUSY);
				
				/* Mark the channel as used. Increment channel index */
				SET_BIT(used[channel / CORE_COMMAND_TRIGGER_PER_BYTE], channel % CORE_COMMAND_TRIGGER_PER_BYTE);
				channel = (channel + 1) % CORE_COMMAND_COUNT;
				
				/* Re-enable pause warning log message */
				logPause = true;
			} /* Used? */
		} /* Busy/Pending? */
		
		i = (i + 1) % core.palletCount; /* Proceed to next pallet buffer */
	} /* Loop pallets */
	
	if(!pause)
		start = 0; /* Reset start */
		
	/* Re-open channels */
	for(i = 0; i < CORE_COMMAND_COUNT; i++) {
		if(GET_BIT(reset[i / CORE_COMMAND_TRIGGER_PER_BYTE], i % CORE_COMMAND_TRIGGER_PER_BYTE)) {
			CLEAR_BIT(reset[i / CORE_COMMAND_TRIGGER_PER_BYTE], i % CORE_COMMAND_TRIGGER_PER_BYTE);
			CLEAR_BIT(used[i / CORE_COMMAND_TRIGGER_PER_BYTE], i % CORE_COMMAND_TRIGGER_PER_BYTE);
		}
	}
	
	/*********************
	 Simple Target Release
	*********************/
	/* 
	   1. Loop through core.targetCount targets
	   2. Access target release commands in cyclic control data
	   3. Write new command 1-3 when pending, switch to busy
	   4. Ackowledge commands when done (!present or error), pass error  
	*/
	for(i = 0; i < core.targetCount; i++) { /* i = 0 is Target 1 */
		pSimpleCommand = core.pSimpleRelease + i;
		pTargetRelease = core.pCyclicControl + core.interface.targetControlOffset + (i + 1) / CORE_TARGET_RELEASE_PER_BYTE; /* Cyclic data starts with Target 0 */
		pTargetStatus = core.pCyclicStatus + core.interface.targetStatusOffset + CORE_TARGET_STATUS_BYTE_COUNT * (i + 1);
		pSimpleReleaseTimer = (unsigned long*)&pSimpleCommand->command.u1[4]; /* Use upper four bytes for timeout monitoring */
		lowerBit = ((i + 1) % CORE_TARGET_RELEASE_PER_BYTE) * CORE_TARGET_RELEASE_BIT_COUNT;
		upperBit = lowerBit + 1;
		
		/* This target's simple release command is in progress */
		if(GET_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY)) {
			*pSimpleReleaseTimer += CORE_CYCLE_TIME;
			
			/* 1. First check for an error */
			if(GET_BIT(*pTargetStatus, stTARGET_RELEASE_ERROR)) {
				args.i[0] = i + 1;
				args.i[1] = pSimpleCommand->command.u1[0];
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_COMMAND), "Target %i release from local move configuration %i has failed", &args);
				
				/* Clear release bits */
				CLEAR_BIT(*pTargetRelease, lowerBit);
				CLEAR_BIT(*pTargetRelease, upperBit);
				
				/* Update status */
				CLEAR_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pSimpleCommand->status, CORE_COMMAND_DONE);
				SET_BIT(pSimpleCommand->status, CORE_COMMAND_ERROR);
			}
			
			/* 2. Next check for not present */
			/* Need to check for error first in case pallet was never present */
			else if(!GET_BIT(*pTargetStatus, stTARGET_PALLET_PRESENT)) {
				args.i[0] = i + 1;
				args.i[1] = pSimpleCommand->command.u1[0];
				logMessage(CORE_LOG_SEVERITY_DEBUG, 6100, "Target %i release from local move configuration %i acknowledged", &args);
				
				/* Clear release bits */
				CLEAR_BIT(*pTargetRelease, lowerBit);
				CLEAR_BIT(*pTargetRelease, upperBit);
				
				/* Update status */
				CLEAR_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pSimpleCommand->status, CORE_COMMAND_DONE);
			}
			
			/* 3. Check for timeout */
			else if(*pSimpleReleaseTimer > CORE_COMMAND_TIMEOUT) {
				args.i[0] = i + 1;
				args.i[1] = pSimpleCommand->command.u1[0];
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_TIMEOUT), "Target %i release from local move configuration %i has timed out", &args);
				
				/* Clear release bits */
				CLEAR_BIT(*pTargetRelease, lowerBit);
				CLEAR_BIT(*pTargetRelease, upperBit);
				
				/* Update status */
				CLEAR_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pSimpleCommand->status, CORE_COMMAND_DONE);
				SET_BIT(pSimpleCommand->status, CORE_COMMAND_ERROR);
			}
		}
		/* This target has a simple release request pending */
		else if(GET_BIT(pSimpleCommand->status, CORE_COMMAND_PENDING)) {
			/* Write command in cyclic control */
			switch(pSimpleCommand->command.u1[0]) {
				case 1:
					SET_BIT(*pTargetRelease, lowerBit);
					break;
				case 2:
					SET_BIT(*pTargetRelease, upperBit);
					break;
				default:
					/* Assume that coreSimpleTargetReleaseType structure is only populated with values 1, 2, or 3 */
					SET_BIT(*pTargetRelease, lowerBit);
					SET_BIT(*pTargetRelease, upperBit);
			}
			
			/* Update status */
			CLEAR_BIT(pSimpleCommand->status, CORE_COMMAND_PENDING);
			SET_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY);
			
			/* Reset request timer */
			*pSimpleReleaseTimer = 0;
			
		} /* Busy?, pending? */
	} /* Loop targets */
	
} /* End function */

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}

/* Write lowercase command name to str */
void getCommand(char *str, unsigned long size, SuperTrakCommand_t data) {
	if(CORE_COMMAND_ID_RELEASE <= data.u1[0] && data.u1[0] <= CORE_COMMAND_ID_RELEASE + 3)
		coreStringCopy(str, "release pallet", size);
	else if(CORE_COMMAND_ID_OFFSET <= data.u1[0] && data.u1[0] <= CORE_COMMAND_ID_OFFSET + 3)
		coreStringCopy(str, "release target offset", size);
	else if(CORE_COMMAND_ID_INCREMENT <= data.u1[0] && data.u1[0] <= CORE_COMMAND_ID_INCREMENT + 3)
		coreStringCopy(str, "release incremental offset", size);
	else if(data.u1[0] == CORE_COMMAND_ID_CONTINUE || data.u1[0] == CORE_COMMAND_ID_CONTINUE + 2)
		coreStringCopy(str, "continue move", size);
	else if(data.u1[0] == CORE_COMMAND_ID_PALLET_ID)
		coreStringCopy(str, "set pallet ID", size);
	else if(data.u1[0] == CORE_COMMAND_ID_MOTION || data.u1[0] == CORE_COMMAND_ID_MOTION + 2)
		coreStringCopy(str, "set motion parameters", size);
	else if(data.u1[0] == CORE_COMMAND_ID_MECHANICAL || data.u1[0] == CORE_COMMAND_ID_MECHANICAL + 2)
		coreStringCopy(str, "set mechanical parameters", size);
	else if(data.u1[0] == CORE_COMMAND_ID_CONTROL || data.u1[0] == CORE_COMMAND_ID_CONTROL + 2)
		coreStringCopy(str, "set control parameters", size);
	else
		coreStringCopy(str, "unknown", size);
}

/* Write uppercase context name to str based on command */
void getContext(char *str, unsigned long size, SuperTrakCommand_t data) {
	if(data.u1[0] == CORE_COMMAND_ID_RELEASE || data.u1[0] == CORE_COMMAND_ID_RELEASE + 1 ||
	  data.u1[0] == CORE_COMMAND_ID_OFFSET || data.u1[0] == CORE_COMMAND_ID_OFFSET + 1 ||
	  data.u1[0] == CORE_COMMAND_ID_INCREMENT || data.u1[0] == CORE_COMMAND_ID_INCREMENT + 1 ||
	  data.u1[0] == CORE_COMMAND_ID_CONTINUE || data.u1[0] == CORE_COMMAND_ID_PALLET_ID || data.u1[0] == CORE_COMMAND_ID_MOTION || data.u1[0] == CORE_COMMAND_ID_MECHANICAL || data.u1[0] == CORE_COMMAND_ID_CONTROL)
		coreStringCopy(str, "Target", size);
	else if(data.u1[0] == CORE_COMMAND_ID_RELEASE + 2 || data.u1[0] == CORE_COMMAND_ID_RELEASE + 3 ||
	  data.u1[0] == CORE_COMMAND_ID_OFFSET + 2 || data.u1[0] == CORE_COMMAND_ID_OFFSET + 3 ||
	  data.u1[0] == CORE_COMMAND_ID_INCREMENT + 2 || data.u1[0] == CORE_COMMAND_ID_INCREMENT + 3 ||
	  data.u1[0] == CORE_COMMAND_ID_CONTINUE + 2 || data.u1[0] == CORE_COMMAND_ID_MOTION + 2 || data.u1[0] == CORE_COMMAND_ID_MECHANICAL + 2 || data.u1[0] == CORE_COMMAND_ID_CONTROL + 2)
		coreStringCopy(str, "Pallet", size);
	else
		coreStringCopy(str, "Unknown", size);
}

/* Write lowercase direction name to str based on command */
void getDirection(char *str, unsigned long size, SuperTrakCommand_t data) {
	if(data.u1[0] == CORE_COMMAND_ID_RELEASE || data.u1[0] == CORE_COMMAND_ID_RELEASE + 2 || 
	  data.u1[0] == CORE_COMMAND_ID_OFFSET || data.u1[0] == CORE_COMMAND_ID_OFFSET + 2 || 
	  data.u1[0] == CORE_COMMAND_ID_INCREMENT || data.u1[0] == CORE_COMMAND_ID_INCREMENT + 2) 
		coreStringCopy(str, "left", size);
	else if(data.u1[0] == CORE_COMMAND_ID_RELEASE + 1 || data.u1[0] == CORE_COMMAND_ID_RELEASE + 3 || 
	  data.u1[0] == CORE_COMMAND_ID_OFFSET + 1 || data.u1[0] == CORE_COMMAND_ID_OFFSET + 3 || 
	  data.u1[0] == CORE_COMMAND_ID_INCREMENT + 1 || data.u1[0] == CORE_COMMAND_ID_INCREMENT + 3)
		coreStringCopy(str, "right", size);
	else
		coreStringCopy(str, "n/a", size);
}

/* Write command parameters */
void getParameter(char *str, unsigned long size, SuperTrakCommand_t data) {
	/* Declare local variables */
	coreFormatArgumentType args;
	unsigned short dataUInt16;
	
	if(CORE_COMMAND_ID_RELEASE <= data.u1[0] && data.u1[0] <= CORE_COMMAND_ID_RELEASE + 3) {
		getDirection(args.s[0], sizeof(args.s[0]), data);
		args.i[0] = data.u1[2];
		coreFormat(str, size, "direction: %s, destination: T%i", &args);
	}
	else if(CORE_COMMAND_ID_OFFSET <= data.u1[0] && data.u1[0] <= CORE_COMMAND_ID_OFFSET + 3) {
		getDirection(args.s[0], sizeof(args.s[0]), data);
		args.i[0] = data.u1[2];
		memcpy(&args.i[1], &data.u1[4], sizeof(args.i[1]));
		coreFormat(str, size, "direction: %s, destination: T%i, offset: %i um", &args);
	}
	else if(CORE_COMMAND_ID_INCREMENT <= data.u1[0] && data.u1[0] <= CORE_COMMAND_ID_INCREMENT + 3) {
		memcpy(&args.i[0], &data.u1[4], sizeof(args.i[0]));
		coreFormat(str, size, "offset: %i um", &args);
	}
	else if(data.u1[0] == CORE_COMMAND_ID_PALLET_ID) {
		args.i[0] = data.u1[2];
		coreFormat(str, size, "ID: %i", &args);
	}
	else if(data.u1[0] == CORE_COMMAND_ID_MOTION || data.u1[0] == CORE_COMMAND_ID_MOTION + 2) {
		memcpy(&dataUInt16, &data.u1[2], sizeof(dataUInt16));
		args.i[0] = dataUInt16;
		memcpy(&dataUInt16, &data.u1[4], sizeof(dataUInt16));
		args.i[1] = dataUInt16;
		coreFormat(str, size, "velocity: %i mm/s, acceleration: %i m/s", &args);
	}
	else if(data.u1[0] == CORE_COMMAND_ID_MECHANICAL || data.u1[0] == CORE_COMMAND_ID_MECHANICAL + 2) {
		memcpy(&dataUInt16, &data.u1[2], sizeof(dataUInt16));
		args.f[0] = dataUInt16 / 10.0;
		memcpy(&dataUInt16, &data.u1[4], sizeof(dataUInt16));
		args.f[1] = dataUInt16 / 10.0;
		coreFormat(str, size, "length: %f mm, offset: %f mm", &args);
	}
	else if(data.u1[0] == CORE_COMMAND_ID_CONTROL || data.u1[0] == CORE_COMMAND_ID_CONTROL + 2) {
		args.i[0] = data.u1[2];
		args.i[1] = data.u1[3];
		args.i[2] = data.u1[4];
		coreFormat(str, size, "set: %i, moving: %i%%, stationary: %i%%", &args);
	}
	else
		coreStringCopy(str, "n/a", size);
}
