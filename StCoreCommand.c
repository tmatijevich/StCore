/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

static void getCommand(char *str, unsigned char command, unsigned long size);
static void getContext(char *str, unsigned char command, unsigned long size);
static void getDirection(char *str, unsigned char command, unsigned long size);

/* Command target or pallet to release to target */
long StCoreReleaseToTarget(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget) {
	
	/***********************
	 Declare local variables
	***********************/
	unsigned char index, commandID, context, *pPalletPresent;
	coreCommandBufferType *pBuffer;
	FormatStringArgumentsType args;
	
	/* Check references */
	if(pCoreCyclicStatus == NULL || pCoreCommandBuffer == NULL) {
		coreLogMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_ALLOC), "StCoreReleaseToTarget() is unable to reference cyclic data or command buffers");
		return stCORE_ERROR_ALLOC;
	}
	
	/**************
	 Create command
	**************/
	if(Target != 0) {
		if(Target > coreTargetCount) {
			args.i[0] = Target;
			args.i[1] = coreTargetCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Target %i context exceeds count [1, %i] of release to target command", &args);
			return stCORE_ERROR_INPUT;
		}
					
		pPalletPresent = pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * Target + 1;
		if(*pPalletPresent < 1 || corePalletCount < *pPalletPresent) {
			args.i[0] = *pPalletPresent;
			args.i[1] = Target;
			args.i[2] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i present at target %i exceeds count [1, %i] of release to target command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = *pPalletPresent;
		context = Target;
		if(Direction == stDIRECTION_RIGHT)
			commandID = 17;
		else
			commandID = 16;
	}
	else {
		if(Pallet < 1 || corePalletCount < Pallet) {
			args.i[0] = Pallet;
			args.i[1] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i context exceeds count [1, %i] of release to target command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = Pallet;
		context = Pallet;
		if(Direction == stDIRECTION_RIGHT)
			commandID = 19;
		else
			commandID = 18;
	}
	
	/*********************
	 Access command buffer
	*********************/
	pBuffer = pCoreCommandBuffer + index - 1;
	
	/* Prepare message */
	getContext(args.s[0], commandID, sizeof(args.s[0]));
	if(Target) args.i[0] = Target;
	else args.i[0] = Pallet;
	getDirection(args.s[1], commandID, sizeof(args.s[1]));
	args.i[1] = DestinationTarget;
	
	if(pBuffer->full) {
		coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_BUFFER), "Release to target command of context %s %i rejected by full buffer", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	pBuffer->command[pBuffer->write].u1[0] = commandID;
	pBuffer->command[pBuffer->write].u1[1] = context;
	pBuffer->command[pBuffer->write].u1[2] = DestinationTarget;
	
	coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 5200, "%s %i release to target direction=%s destination=T%i", &args);
	
	pBuffer->write = (pBuffer->write + 1) % CORE_COMMANDBUFFER_SIZE;
	if(pBuffer->write == pBuffer->read) {
		pBuffer->full = true;
		args.i[0] = index;
		args.i[1] = CORE_COMMANDBUFFER_SIZE;
		coreLogFormatMessage(USERLOG_SEVERITY_WARNING, coreEventCode(stCORE_WARNING_BUFFER), "Pallet %i commad buffer is full (size=%i)", &args);
	}
	
	return 0;
	
}

/* Command target or pallet to release to offset */
long StCoreReleaseToOffset(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget, long TargetOffset) {
	
	/***********************
	 Declare local variables
	***********************/
	unsigned char index, commandID, context, *pPalletPresent;
	coreCommandBufferType *pBuffer;
	FormatStringArgumentsType args;
	
	/* Check references */
	if(pCoreCyclicStatus == NULL || pCoreCommandBuffer == NULL) {
		coreLogMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_ALLOC), "StCoreReleaseToOffset() is unable to reference cyclic data or command buffers");
		return stCORE_ERROR_ALLOC;
	}
	
	/**************
	 Create command
	**************/
	if(Target != 0) {
		if(Target > coreTargetCount) {
			args.i[0] = Target;
			args.i[1] = coreTargetCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Target %i context exceeds count [1, %i] of release to offset command", &args);
			return stCORE_ERROR_INPUT;
		}
					
		pPalletPresent = pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * Target + 1;
		if(*pPalletPresent < 1 || corePalletCount < *pPalletPresent) {
			args.i[0] = *pPalletPresent;
			args.i[1] = Target;
			args.i[2] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i present at target %i exceeds count [1, %i] of release to offset command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = *pPalletPresent;
		context = Target;
		if(Direction == stDIRECTION_RIGHT)
			commandID = 25;
		else
			commandID = 24;
	}
	else {
		if(Pallet < 1 || corePalletCount < Pallet) {
			args.i[0] = Pallet;
			args.i[1] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i context exceeds count [1, %i] of release to offset command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = Pallet;
		context = Pallet;
		if(Direction == stDIRECTION_RIGHT)
			commandID = 27;
		else
			commandID = 26;
	}
	
	/*********************
	 Access command buffer
	*********************/
	pBuffer = pCoreCommandBuffer + index - 1;
	
	/* Prepare message */
	getContext(args.s[0], commandID, sizeof(args.s[0]));
	if(Target) args.i[0] = Target;
	else args.i[0] = Pallet;
	getDirection(args.s[1], commandID, sizeof(args.s[1]));
	args.i[1] = DestinationTarget;
	args.i[2] = TargetOffset;
	
	if(pBuffer->full) {
		coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_BUFFER), "Release to offset command of context %s %i rejected by full buffer", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	pBuffer->command[pBuffer->write].u1[0] = commandID;
	pBuffer->command[pBuffer->write].u1[1] = context;
	pBuffer->command[pBuffer->write].u1[2] = DestinationTarget;
	memcpy(&(pBuffer->command[pBuffer->write].u1[4]), &TargetOffset, 4);
	
	coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 5200, "%s %i release to offset direction=%s destination=T%i offset=%i um", &args);
	
	pBuffer->write = (pBuffer->write + 1) % CORE_COMMANDBUFFER_SIZE;
	if(pBuffer->write == pBuffer->read) {
		pBuffer->full = true;
		args.i[0] = index;
		args.i[1] = CORE_COMMANDBUFFER_SIZE;
		coreLogFormatMessage(USERLOG_SEVERITY_WARNING, coreEventCode(stCORE_WARNING_BUFFER), "Pallet %i commad buffer is full (size=%i)", &args);
	}
	
	return 0;
	
} /* Function definition */

/* Execute requested user commands */
void coreProcessCommand(void) {
	
	/***********************
	 Declare local variables
	***********************/
	static unsigned char logAlloc = true;
	coreCommandBufferType *pBuffer;
	SuperTrakCommand_t *pCommand;
	long i;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success;
	FormatStringArgumentsType args;
	
	/****************
	 Check references
	****************/
	if(pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL || pCoreCommandBuffer == NULL) {
		if(logAlloc) {
			logAlloc = false;
			coreLogMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_ALLOC), "StCoreCyclic() (coreProcessCommand) is unable to reference cyclic data or command buffers");
		}
		return;
	}
	else
		logAlloc = true;
	
	/***********************
	 Process command buffers
	***********************/
	for(i = 0; i < corePalletCount; i++) {
		/* Access data */
		pBuffer = pCoreCommandBuffer + i;
		pTrigger = pCoreCyclicControl + coreInterfaceConfig.commandTriggerOffset + i / 8; /* Trigger group */
		pCommand = (SuperTrakCommand_t*)(pCoreCyclicControl + coreInterfaceConfig.commandDataOffset) + i;
		pComplete = pCoreCyclicStatus + coreInterfaceConfig.commandCompleteOffset + i / 8;
		pSuccess = pCoreCyclicStatus + coreInterfaceConfig.commandSuccessOffset + i / 8;
		complete = GET_BIT(*pComplete, i % 8);
		success = GET_BIT(*pSuccess, i % 8);
		
		if(pBuffer->active) {
			/* Track time */
			pBuffer->timer += 800;
			
			/* Build message */
			getContext(args.s[0], pCommand->u1[0], sizeof(args.s[0]));
			args.i[0] = pCommand->u1[1];
			getCommand(args.s[1], pCommand->u1[0], sizeof(args.s[1]));
			
			/* Wait for complete or timeout */
			if(complete) {
				if(success)
					coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 6000, "%s %i %s command executed successfully", &args);
				else
					coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_CMDFAILURE), "%s %i %s command execution failed", &args);
				memset(pCommand, 0, sizeof(*pCommand));
				CLEAR_BIT(*pTrigger, i % 8);
				pBuffer->active = false;
			}
			else if(pBuffer->timer >= 500000) {
				coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_CMDTIMEOUT), "%s %i %s command execution timed out", &args);
				memset(pCommand, 0, sizeof(*pCommand));
				CLEAR_BIT(*pTrigger, i % 8);
				pBuffer->active = false;
			}
		}
		else if(pBuffer->read != pBuffer->write || pBuffer->full) {
			/* Read next command, iterate, and reset full */
			memcpy(pCommand, &(pBuffer->command[pBuffer->read]), sizeof(SuperTrakCommand_t));
			pBuffer->read = (pBuffer->read + 1) % CORE_COMMANDBUFFER_SIZE;
			pBuffer->full = false;
			
			/* Set the trigger and activate */
			SET_BIT(*pTrigger, i % 8);
			pBuffer->active = true;
		} /* Active? */
	} /* Loop pallets */
	
}

/* Get command */
static void getCommand(char *str, unsigned char command, unsigned long size) {
	if(command == 16 || command == 17 || command == 18 || command == 19)
		coreStringCopy(str, "release to target", size);
	else if(command == 24 || command == 25 || command == 26 || command == 27)
		coreStringCopy(str, "release to offset", size);
	else
		coreStringCopy(str, "unknown", size);
}

/* Get context */
static void getContext(char *str, unsigned char command, unsigned long size) {
	if(command == 16 || command == 17 || command == 24 || command == 25)
		coreStringCopy(str, "Target", size);
	else if(command == 18 || command == 19 || command == 26 || command == 27)
		coreStringCopy(str, "Pallet", size);
	else
		coreStringCopy(str, "Unknown", size);
}

/* Get direction */
static void getDirection(char *str, unsigned char command, unsigned long size) {
	if(command == 16 || command == 18 || command == 24 || command == 26) 
		coreStringCopy(str, "left", size);
	else
		coreStringCopy(str, "right", size);
}
