/*******************************************************************************
 * File: StCore\Command.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "Main.h"

#define CHANNEL_BYTE_MAX 8 /* 8 bytes (64 bits) for a maximum of 64 command channels */

static void setCommandName(char *str, unsigned char ID, unsigned long size);
static void setContextName(char *str, unsigned char ID, unsigned long size);
static void setDirectionName(char *str, unsigned char ID, unsigned long size);
static void setDestinationName(char *str, unsigned char ID, unsigned char destination, unsigned long size);
void getParameterString(char *str, unsigned long size, SuperTrakCommand_t data);

/* Create command ID, context, and buffer assignment */
long coreCommandCreate(unsigned char start, unsigned char target, unsigned char pallet, unsigned short direction, coreCommandCreateType *create) {
	
	/***********************
	 Declare local variables
	***********************/
	unsigned char *pPalletPresent;
	FormatStringArgumentsType args;
	
	/*****************
	 Derive command ID
	*****************/
	/* Add 1 if direction is right, add 2 if pallet context */
	/* Exception: There is no direction choice for configuration (start < 64) */
	/* Exception: The set pallet ID command can only be in context of target (start != 64) */
	create->commandID = start + (unsigned char)(direction > 0 && start < 64) + 2 * (unsigned char)(target == 0 && start != 64);
	
	/***********************
	 Check global references
	***********************/
	setCommandName(args.s[0], create->commandID, sizeof(args.s[0]));
	if(core.pCyclicStatus == NULL) {
		coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "%s command assignment cannot reference cyclic data", &args);
		return stCORE_ERROR_ALLOC;
	}
	
	/********************************
	 Assign context and manager index
	********************************/
	if(target != 0) {
		if(target > core.targetCount) {
			args.i[0] = target;
			args.i[1] = core.targetCount;
			coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INPUT), "Target %i context exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
					
		pPalletPresent = core.pCyclicStatus + core.interface.targetStatusOffset + 3 * target + 1;
		if(*pPalletPresent < 1 || core.palletCount < *pPalletPresent) {
			args.i[0] = *pPalletPresent;
			args.i[1] = target;
			args.i[2] = core.palletCount;
			coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INPUT), "Pallet %i present at target %i exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		create->index = *pPalletPresent;
		create->context = target;
	}
	else {
		if(pallet < 1 || core.palletCount < pallet) {
			args.i[0] = pallet;
			args.i[1] = core.palletCount;
			coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INPUT), "Pallet %i context exceeds count [1, %i] of %s command", &args);
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
	 Declare local variables
	***********************/
	coreCommandBufferType *pBuffer;
	coreCommandType *pCommand;
	FormatStringArgumentsType args;
	
	/****************
	 Check references
	****************/
	if(core.pCommandBuffer == NULL) {
		coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "%s command request cannot reference command manager", &args);
		return stCORE_ERROR_ALLOC;
	}
	
	/**********************
	 Access command manager
	**********************/
	pBuffer = core.pCommandBuffer + index - 1;
	pCommand = &pBuffer->buffer[pBuffer->write];
	
	/* Prepare message */
	setContextName(args.s[0], command.u1[0], sizeof(args.s[0]));
	args.i[0] = command.u1[1];
	setCommandName(args.s[1], command.u1[0], sizeof(args.s[1]));
	
	/* Check if pending or busy */
	if(GET_BIT(pCommand->status, CORE_COMMAND_PENDING) || GET_BIT(pCommand->status, CORE_COMMAND_BUSY)) {
		coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_BUFFER), "%s %i %s command rejected because buffer is full", &args);
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
	getParameterString(args.s[2], sizeof(args.s[2]), command);
	coreLogFormat(USERLOG_SEVERITY_DEBUG, 5200, "%s %i %s command request (%s)", &args);
	
	/* Increment write index and check if full */
	pBuffer->write = (pBuffer->write + 1) % CORE_COMMAND_BUFFER_SIZE;
	pCommand = &pBuffer->buffer[pBuffer->write];
	if(GET_BIT(pCommand->status, CORE_COMMAND_PENDING) || GET_BIT(pCommand->status, CORE_COMMAND_BUSY)) {
		args.i[0] = index;
		args.i[1] = CORE_COMMAND_BUFFER_SIZE;
		coreLogFormat(USERLOG_SEVERITY_WARNING, coreLogCode(stCORE_WARNING_BUFFER), "Pallet %i command buffer is now full (size = %i)", &args);
	}
	
	return 0;
	
} /* End function */

/* Process requested user commands */
void coreCommandManager(void) {
	
	/***********************
	 Declare local variables
	***********************/
	static unsigned char logAlloc = true, logPause = true;
	coreCommandBufferType *pBuffer;
	coreCommandType *pCommand;
	SuperTrakCommand_t *pChannel;
	long i, j;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success, pause;
	static unsigned char used[CHANNEL_BYTE_MAX], reset[CHANNEL_BYTE_MAX], channel, start;
	static unsigned long timer[CORE_COMMAND_COUNT];
	coreSimpleTargetReleaseType *pSimpleCommand; /* Simple target release command storage */
	unsigned char *pTargetRelease, *pTargetStatus, lowerBit, upperBit; /* Simple target release and target status cyclic bits */
	FormatStringArgumentsType args;
	
	/****************
	 Check references
	****************/
	if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL || core.pCommandBuffer == NULL || core.pSimpleRelease == NULL) {
		if(logAlloc) {
			logAlloc = false;
			coreLogMessage(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "StCoreCyclic (coreCommandManager) is unable to reference cyclic data or command buffers");
		}
		return;
	}
	else
		logAlloc = true;
	
	/***********************
	 Process command buffers
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
			pTrigger = core.pCyclicControl + core.interface.commandTriggerOffset + pBuffer->channel / 8; /* Trigger group */
			pChannel = (SuperTrakCommand_t*)(core.pCyclicControl + core.interface.commandDataOffset) + pBuffer->channel;
			pComplete = core.pCyclicStatus + core.interface.commandCompleteOffset + pBuffer->channel / 8;
			pSuccess = core.pCyclicStatus + core.interface.commandSuccessOffset + pBuffer->channel / 8;
			complete = GET_BIT(*pComplete, pBuffer->channel % 8);
			success = GET_BIT(*pSuccess, pBuffer->channel % 8);
			
			/* Track time */
			timer[pBuffer->channel] += 800;
			
			/* Build message */
			setContextName(args.s[0], pChannel->u1[0], sizeof(args.s[0]));
			args.i[0] = pChannel->u1[1];
			setCommandName(args.s[1], pChannel->u1[0], sizeof(args.s[1]));
			
			/* Wait for complete or timeout */
			if(complete) {
				/* Confirm success or failure */
				if(success)
					coreLogFormat(USERLOG_SEVERITY_DEBUG, 6000, "%s %i %s command acknowledged", &args);
				else {
					coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_CMDFAILURE), "%s %i %s command execution failed", &args);
					SET_BIT(pCommand->status, CORE_COMMAND_ERROR);
				}
				
				/* Clear cyclic data and timer */
				memset(pChannel, 0, sizeof(*pChannel));
				CLEAR_BIT(*pTrigger, pBuffer->channel % 8);
				timer[pBuffer->channel] = 0;
				
				/* Reopen channels only after all pallet buffers have all been polled */
				SET_BIT(reset[channel / 8], channel % 8);
				
				/* Update command status */
				CLEAR_BIT(pCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pCommand->status, CORE_COMMAND_DONE);
				
				/* Move to next command in buffer */
				pBuffer->read = (pBuffer->read + 1) % CORE_COMMAND_BUFFER_SIZE;
			}
			else if(timer[pBuffer->channel] >= 500000) {
				coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_CMDTIMEOUT), "%s %i %s command execution timed out", &args);
				
				/* Clear cyclic data and timer */
				memset(pCommand, 0, sizeof(*pCommand));
				CLEAR_BIT(*pTrigger, pBuffer->channel % 8);
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
			if(GET_BIT(used[channel / 8], channel % 8)) {
				pause = true; /* Pause for the rest of this loop */
				start = i; /* Remember spot where the pause started */
				if(logPause) {
					logPause = false;
					coreLogMessage(USERLOG_SEVERITY_WARNING, 6001, "Pallet command buffers are paused because all channels are in use");
				}
			}
			else {
				/* Assign the command to the channel */
				pChannel = (SuperTrakCommand_t*)(core.pCyclicControl + core.interface.commandDataOffset) + channel;
				memcpy(pChannel, &pCommand->command, sizeof(SuperTrakCommand_t));
				
				/* Set trigger */
				pTrigger = core.pCyclicControl + core.interface.commandTriggerOffset + channel / 8;
				SET_BIT(*pTrigger, channel % 8);
				
				/* Update command channel and status */
				pBuffer->channel = channel;
				CLEAR_BIT(pCommand->status, CORE_COMMAND_PENDING);
				SET_BIT(pCommand->status, CORE_COMMAND_BUSY);
				
				/* Mark the channel as used. Increment channel index */
				SET_BIT(used[channel / 8], channel % 8);
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
		if(GET_BIT(reset[i / 8], i % 8)) {
			CLEAR_BIT(reset[i / 8], i % 8);
			CLEAR_BIT(used[i / 8], i % 8);
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
		lowerBit = ((i + 1) % CORE_TARGET_RELEASE_PER_BYTE) * CORE_TARGET_RELEASE_BIT_COUNT;
		upperBit = lowerBit + 1;
		
		/* This target's simple release command is in progress */
		if(GET_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY)) {
			pSimpleCommand->timer += CORE_CYCLE_TIME;
			
			/* 1. First check for an error */
			if(GET_BIT(*pTargetStatus, stTARGET_RELEASE_ERROR)) {
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
				/* Clear release bits */
				CLEAR_BIT(*pTargetRelease, lowerBit);
				CLEAR_BIT(*pTargetRelease, upperBit);
				
				/* Update status */
				CLEAR_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pSimpleCommand->status, CORE_COMMAND_DONE);
			}
			
			/* 3. Check for timeout */
			else if(pSimpleCommand->timer > CORE_COMMAND_TIMEOUT) {
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
			switch(pSimpleCommand->move) {
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
			pSimpleCommand->timer = 0;
			
		} /* Busy?, pending? */
	} /* Loop targets */
	
} /* End function */

/* Write lowercase command name to str */
void setCommandName(char *str, unsigned char ID, unsigned long size) {
	if(ID == 16 || ID == 17 || ID == 18 || ID == 19)
		coreStringCopy(str, "release pallet", size);
	else if(ID == 24 || ID == 25 || ID == 26 || ID == 27)
		coreStringCopy(str, "release target offset", size);
	else if(ID == 28 || ID == 29 || ID == 30 || ID == 31)
		coreStringCopy(str, "release incremental offset", size);
	else if(ID == 60 || ID == 62)
		coreStringCopy(str, "continue move", size);
	else if(ID == 64)
		coreStringCopy(str, "set pallet ID", size);
	else if(ID == 68 || ID == 70)
		coreStringCopy(str, "set motion parameters", size);
	else if(ID == 72 || ID == 74)
		coreStringCopy(str, "set mechanical parameters", size);
	else if(ID == 76 || ID == 78)
		coreStringCopy(str, "set control parameters", size);
	else
		coreStringCopy(str, "unknown", size);
}

/* Write uppercase context name to str based on command */
void setContextName(char *str, unsigned char ID, unsigned long size) {
	if(ID == 16 || ID == 17 || ID == 24 || ID == 25 || ID == 28 || ID == 29 || ID == 60 || ID == 64 || ID == 68 || ID == 72 || ID == 76)
		coreStringCopy(str, "Target", size);
	else if(ID == 18 || ID == 19 || ID == 26 || ID == 27 || ID == 30 || ID == 31 || ID == 62 || ID == 70 || ID == 74 || ID == 78)
		coreStringCopy(str, "Pallet", size);
	else
		coreStringCopy(str, "Unknown", size);
}

/* Write lowercase direction name to str based on command */
void setDirectionName(char *str, unsigned char ID, unsigned long size) {
	if(ID == 16 || ID == 18 || ID == 24 || ID == 26 || ID == 28 || ID == 30) 
		coreStringCopy(str, "left", size);
	else if(ID == 17 || ID == 19 || ID == 25 || ID == 27 || ID == 29 || ID == 31)
		coreStringCopy(str, "right", size);
	else
		coreStringCopy(str, "n/a", size);
}

/* Write destination target in text */
void setDestinationName(char *str, unsigned char ID, unsigned char destination, unsigned long size) {
	FormatStringArgumentsType args;
	if(ID == 16 || ID == 17 || ID == 18 || ID == 19 || ID == 24 || ID == 25 || ID == 26 || ID == 27) {
		args.i[0] = destination;
		IecFormatString(str, size, "T%i", &args);
	}
	else
		coreStringCopy(str, "n/a", size);
}

/* Write command parameters */
void getParameterString(char *str, unsigned long size, SuperTrakCommand_t data) {
	/* Declare local variables */
	FormatStringArgumentsType args;
	
	if(data.u1[0] == 16 || data.u1[0] == 17 || data.u1[0] == 18 || data.u1[0] == 19) {
		setDirectionName(args.s[0], data.u1[0], sizeof(args.s[0]));
		args.i[0] = data.u1[2];
		IecFormatString(str, size, "direction: %s, destination: T%i", &args);
	}
	else
		coreStringCopy(str, "n/a", size);
}
