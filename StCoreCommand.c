/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

static void setCommandName(char *str, unsigned char ID, unsigned long size);
static void setContextName(char *str, unsigned char ID, unsigned long size);
static void setDirectionName(char *str, unsigned char ID, unsigned long size);
static void setDestinationTargetName(char *str, unsigned char ID, unsigned char destination, unsigned long size);

/*******************************************************************************
********************************************************************************
 Create command
********************************************************************************
*******************************************************************************/

/* Create command ID, context, and buffer index */
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
	
} /* Function definition */

/*******************************************************************************
********************************************************************************
 Command request
********************************************************************************
*******************************************************************************/

/* Request command for allocated pallet command buffer */
long coreCommandRequest(unsigned char index, SuperTrakCommand_t command, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare local variables
	***********************/
	coreCommandBufferType *pManager;
	coreCommandType *pEntry;
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
	pManager = core.pCommandBuffer + index - 1;
	pEntry = &pManager->buffer[pManager->write];
	
	/* Prepare message */
	setContextName(args.s[0], command.u1[0], sizeof(args.s[0]));
	args.i[0] = command.u1[1];
	setCommandName(args.s[1], command.u1[0], sizeof(args.s[1]));
	setDirectionName(args.s[2], command.u1[0], sizeof(args.s[2]));
	setDestinationTargetName(args.s[3], command.u1[0], command.u1[2], sizeof(args.s[3]));
	
	/* Check if pending or busy */
	if(GET_BIT(pEntry->status, CORE_COMMAND_PENDING) || GET_BIT(pEntry->status, CORE_COMMAND_BUSY)) {
		coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_BUFFER), "%s %i %s command rejected because buffer is full", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	/* Write command */
	memcpy(&pEntry->command, &command, sizeof(pEntry->command));
	
	/* Update status, tag instance, and share entry */
	CLEAR_BIT(pEntry->status, CORE_COMMAND_DONE);
	CLEAR_BIT(pEntry->status, CORE_COMMAND_ERROR);
	SET_BIT(pEntry->status, CORE_COMMAND_PENDING);
	pEntry->pInstance = pInstance;
	if(ppCommand != NULL) *ppCommand = pEntry;
	
	/* Debug comfirmation message */
	coreLogFormat(USERLOG_SEVERITY_DEBUG, 5200, "%s %i %s command request (direction = %s, destination = %s)", &args);
	
	/* Increment write index and check if full */
	pManager->write = (pManager->write + 1) % CORE_COMMANDBUFFER_SIZE;
	pEntry = &pManager->buffer[pManager->write];
	if(GET_BIT(pEntry->status, CORE_COMMAND_PENDING) || GET_BIT(pEntry->status, CORE_COMMAND_BUSY)) {
		args.i[0] = index;
		args.i[1] = CORE_COMMANDBUFFER_SIZE;
		coreLogFormat(USERLOG_SEVERITY_WARNING, coreLogCode(stCORE_WARNING_BUFFER), "Pallet %i command buffer is now full (size = %i)", &args);
	}
	
	return 0;
	
} /* Function definition */

/*******************************************************************************
********************************************************************************
 Command manager
********************************************************************************
*******************************************************************************/

/* Execute requested user commands */
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
	static unsigned char used[CORE_COMMAND_COUNT], channel, start;
	static unsigned long timer[CORE_COMMAND_COUNT];
	FormatStringArgumentsType args;
	
	/****************
	 Check references
	****************/
	if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL || core.pCommandBuffer == NULL) {
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
				
				/* Reopen channels only after all pallet buffers have been polled */
				
				/* Update command status */
				CLEAR_BIT(pCommand->status, CORE_COMMAND_BUSY);
				SET_BIT(pCommand->status, CORE_COMMAND_DONE);
				
				/* Move to next command in buffer */
				pBuffer->read = (pBuffer->read + 1) % CORE_COMMANDBUFFER_SIZE;
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
				pBuffer->read = (pBuffer->read + 1) % CORE_COMMANDBUFFER_SIZE;
			}
		}
		
		/* Pallet buffer has command pending */
		else if(!pause && GET_BIT(pCommand->status, CORE_COMMAND_PENDING)) {
			/* Is the next channel available? */
			if(used[channel]) {
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
				used[channel] = true;
				channel = (channel + 1) % CORE_COMMAND_COUNT;
				
				/* Re-enable pause warning log message */
				logPause = true;
			} /* Used? */
		} /* Busy/Pending? */
		
		i = (i + 1) % core.palletCount; /* Proceed to next pallet buffer */
	} /* Loop pallets */
	
	if(!pause)
		start = 0; /* Reset start */
		
	/* Go through all channels and re-open used channels whose triggers are reset */
	for(i = 0; i < CORE_COMMAND_COUNT; i++) {
		if(used[i]) {
			/* Check the trigger */
			pTrigger = core.pCyclicControl + core.interface.commandTriggerOffset + i / 8;
			if(!GET_BIT(*pTrigger, i % 8))
				used[i] = false; /* Open the channel */
		}
	}
	
} /* End function */

/*******************************************************************************
********************************************************************************
 String handling
********************************************************************************
*******************************************************************************/

/* Write lowercase command name to str */
void setCommandName(char *str, unsigned char ID, unsigned long size) {
	if(ID == 16 || ID == 17 || ID == 18 || ID == 19)
		coreStringCopy(str, "release to target", size);
	else if(ID == 24 || ID == 25 || ID == 26 || ID == 27)
		coreStringCopy(str, "release to offset", size);
	else if(ID == 28 || ID == 29 || ID == 30 || ID == 31)
		coreStringCopy(str, "increment offset", size);
	else if(ID == 60 || ID == 62)
		coreStringCopy(str, "resume", size);
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
void setDestinationTargetName(char *str, unsigned char ID, unsigned char destination, unsigned long size) {
	FormatStringArgumentsType args;
	if(ID == 16 || ID == 17 || ID == 18 || ID == 19 || ID == 24 || ID == 25 || ID == 26 || ID == 27) {
		args.i[0] = destination;
		IecFormatString(str, size, "T%i", &args);
	}
	else
		coreStringCopy(str, "n/a", size);
}
