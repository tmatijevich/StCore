/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

static void setCommandName(char *str, unsigned char command, unsigned long size);
static void setContextName(char *str, unsigned char command, unsigned long size);
static void setDirectionName(char *str, unsigned char command, unsigned long size);

/* Accept all public inputs for common release command */
static long releaseCommand(unsigned char commandStart, unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, long targetOffset, long incrementalOffset) {
	
	/***********************
	 Declare local variables
	***********************/
	unsigned char index, commandID, context, *pPalletPresent;
	coreCommandManagerType *pManager;
	coreCommandEntryType *pEntry;
	FormatStringArgumentsType args;
	
	/*****************
	 Derive command ID
	*****************/
	/* Add 1 if direction is right, add 2 if pallet context */
	commandID = commandStart + (unsigned char)(direction > 0) + 2 * (unsigned char)(target == 0);
	
	/**************
	 Create command
	**************/
	if(target != 0) {
		if(target > coreTargetCount) {
			args.i[0] = target;
			args.i[1] = coreTargetCount;
			setCommandName(args.s[0], commandID, sizeof(args.s[0]));
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Target %i context exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
					
		pPalletPresent = pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * target + 1;
		if(*pPalletPresent < 1 || corePalletCount < *pPalletPresent) {
			args.i[0] = *pPalletPresent;
			args.i[1] = target;
			args.i[2] = corePalletCount;
			setCommandName(args.s[0], commandID, sizeof(args.s[0]));
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i present at target %i exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = *pPalletPresent;
		context = target;
	}
	else {
		if(pallet < 1 || corePalletCount < pallet) {
			args.i[0] = pallet;
			args.i[1] = corePalletCount;
			setCommandName(args.s[0], commandID, sizeof(args.s[0]));
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i context exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		index = pallet;
		context = pallet;
	}
	
	/*********************
	 Access command buffer
	*********************/
	pManager = pCoreCommandManager + index - 1;
	pEntry = &pManager->buffer[pManager->write];
	
	/* Prepare message */
	setContextName(args.s[0], commandID, sizeof(args.s[0]));
	if(target) args.i[0] = target;
	else args.i[0] = pallet;
	setCommandName(args.s[1], commandID, sizeof(args.s[1]));
	setDirectionName(args.s[2], commandID, sizeof(args.s[2]));
	args.i[1] = destinationTarget;
	
	if(GET_BIT(pEntry->status, CORE_COMMAND_PENDING) || GET_BIT(pEntry->status, CORE_COMMAND_BUSY)) {
		coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_BUFFER), "%s %i %s command rejected because buffer is full", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	/* Construct command */
	pEntry->command.u1[0] = commandID;
	pEntry->command.u1[1] = context;
	if(commandStart != 60) /* Resume */
		pEntry->command.u1[2] = destinationTarget;
	if(commandStart == 24) /* Release to offset */
		memcpy(&(pEntry->command.u1[4]), &targetOffset, 4);
	else if(commandStart == 28) /* Increment offset */
		memcpy(&(pEntry->command.u1[4]), &incrementalOffset, 4);
	
	/* Update status, tag instance, and share entry */
	CLEAR_BIT(pEntry->status, CORE_COMMAND_DONE);
	CLEAR_BIT(pEntry->status, CORE_COMMAND_ERROR);
	SET_BIT(pEntry->status, CORE_COMMAND_PENDING);
	/*pEntry->inst = userInst*/
	/**pEntryRef = &pEntry*/
	
	coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 5200, "%s %i %s command request (direction = %s, destination = T%i)", &args);
	
	pManager->write = (pManager->write + 1) % CORE_COMMANDBUFFER_SIZE;
	pEntry = &pManager->buffer[pManager->write];
	if(GET_BIT(pEntry->status, CORE_COMMAND_PENDING) || GET_BIT(pEntry->status, CORE_COMMAND_BUSY)) {
		args.i[0] = index;
		args.i[1] = CORE_COMMANDBUFFER_SIZE;
		coreLogFormatMessage(USERLOG_SEVERITY_WARNING, coreEventCode(stCORE_WARNING_BUFFER), "Pallet %i command buffer is now full (size = %i)", &args);
	}
	
	return 0;
}

/* Command target or pallet to release to target */
long StCoreReleaseToTarget(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget) {
	return releaseCommand(16, Target, Pallet, Direction, DestinationTarget, 0, 0);
}

/* Command target or pallet to release to offset */
long StCoreReleaseToOffset(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget, long TargetOffset) {
	return releaseCommand(24, Target, Pallet, Direction, DestinationTarget, TargetOffset, 0);
}

/* Execute requested user commands */
void coreProcessCommand(void) {
	
	/***********************
	 Declare local variables
	***********************/
	static unsigned char logAlloc = true;
	coreCommandManagerType *pManager;
	coreCommandEntryType *pEntry;
	SuperTrakCommand_t *pCommand;
	long i;
	unsigned char *pTrigger, *pComplete, *pSuccess, complete, success;
	FormatStringArgumentsType args;
	
	/****************
	 Check references
	****************/
	if(pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL || pCoreCommandManager == NULL) {
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
		pManager = pCoreCommandManager + i;
		pEntry = &pManager->buffer[pManager->read];
		pTrigger = pCoreCyclicControl + coreInterfaceConfig.commandTriggerOffset + i / 8; /* Trigger group */
		pCommand = (SuperTrakCommand_t*)(pCoreCyclicControl + coreInterfaceConfig.commandDataOffset) + i;
		pComplete = pCoreCyclicStatus + coreInterfaceConfig.commandCompleteOffset + i / 8;
		pSuccess = pCoreCyclicStatus + coreInterfaceConfig.commandSuccessOffset + i / 8;
		complete = GET_BIT(*pComplete, i % 8);
		success = GET_BIT(*pSuccess, i % 8);
		
		/* Monitor current entry if BUSY */
		if(GET_BIT(pEntry->status, CORE_COMMAND_BUSY)) {
			/* Track time */
			pManager->timer += 800;
			
			/* Build message */
			setContextName(args.s[0], pCommand->u1[0], sizeof(args.s[0]));
			args.i[0] = pCommand->u1[1];
			setCommandName(args.s[1], pCommand->u1[0], sizeof(args.s[1]));
			
			/* Wait for complete or timeout */
			if(complete) {
				/* Confirm success or failure */
				if(success)
					coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 6000, "%s %i %s command executed successfully", &args);
				else {
					coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_CMDFAILURE), "%s %i %s command execution failed", &args);
					SET_BIT(pEntry->status, CORE_COMMAND_ERROR);
				}
				
				/* Clear cyclic data */
				memset(pCommand, 0, sizeof(*pCommand));
				CLEAR_BIT(*pTrigger, i % 8);
				
				/* Update status */
				CLEAR_BIT(pEntry->status, CORE_COMMAND_BUSY);
				SET_BIT(pEntry->status, CORE_COMMAND_DONE);
				
				/* Move to next entry */
				pManager->read = (pManager->read + 1) % CORE_COMMANDBUFFER_SIZE;
			}
			else if(pManager->timer >= 500000) {
				coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_CMDTIMEOUT), "%s %i %s command execution timed out", &args);
				
				/* Clear cyclic data */
				memset(pCommand, 0, sizeof(*pCommand));
				CLEAR_BIT(*pTrigger, i % 8);
				
				/* Update status */
				CLEAR_BIT(pEntry->status, CORE_COMMAND_BUSY);
				SET_BIT(pEntry->status, CORE_COMMAND_DONE);
				SET_BIT(pEntry->status, CORE_COMMAND_ERROR);
				
				/* Move to next entry */
				pManager->read = (pManager->read + 1) % CORE_COMMANDBUFFER_SIZE;
			}
		}
		
		/* Execute current entry if PENDING, set BUSY */
		else if(GET_BIT(pEntry->status, CORE_COMMAND_PENDING)) {
			/* Read next command, iterate, and reset full */
			memcpy(pCommand, &pEntry->command, sizeof(SuperTrakCommand_t));
			
			/* Set the trigger and activate */
			SET_BIT(*pTrigger, i % 8);
			CLEAR_BIT(pEntry->status, CORE_COMMAND_PENDING);
			SET_BIT(pEntry->status, CORE_COMMAND_BUSY);
			
		} /* BUSY or PENDING? */
	} /* Loop pallets */
	
} /* Function definition */

/* Write lowercase command name to str */
void setCommandName(char *str, unsigned char command, unsigned long size) {
	if(command == 16 || command == 17 || command == 18 || command == 19)
		coreStringCopy(str, "release to target", size);
	else if(command == 24 || command == 25 || command == 26 || command == 27)
		coreStringCopy(str, "release to offset", size);
	else if(command == 28 || command == 29 || command == 30 || command == 31)
		coreStringCopy(str, "increment offset", size);
	else if(command == 60 || command == 62)
		coreStringCopy(str, "resume", size);
	else
		coreStringCopy(str, "unknown", size);
}

/* Write uppercase context name to str based on command */
void setContextName(char *str, unsigned char command, unsigned long size) {
	if(command == 16 || command == 17 || command == 24 || command == 25 || command == 28 || command == 29 || command == 60)
		coreStringCopy(str, "Target", size);
	else if(command == 18 || command == 19 || command == 26 || command == 27 || command == 30 || command == 31 || command == 62)
		coreStringCopy(str, "Pallet", size);
	else
		coreStringCopy(str, "Unknown", size);
}

/* Write lowercase direction name to str based on command */
void setDirectionName(char *str, unsigned char command, unsigned long size) {
	if(command == 16 || command == 18 || command == 24 || command == 26 || command == 28 || command == 30) 
		coreStringCopy(str, "left", size);
	else if(command == 17 || command == 19 || command == 25 || command == 27 || command == 29 || command == 31)
		coreStringCopy(str, "right", size);
	else
		coreStringCopy(str, "n/a", size);
}
