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

/* Release pallet to target */
long StCoreReleaseToTarget(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandAssignmentType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreGetCommandAssignment(16, Target, Pallet, Direction, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	command.u1[2] = DestinationTarget;
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL);
	if(status)
		return status;
		
	return 0;
	
} /* Function definition */

/* Release pallet to target + offset */
long StCoreReleaseToOffset(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget, double TargetOffset) {
	
	/***********************
	 Declare local variables
	***********************/
	long status, offset;
	coreCommandAssignmentType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreGetCommandAssignment(24, Target, Pallet, Direction, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	command.u1[2] = DestinationTarget;
	offset = (long)(TargetOffset * 1000.0);
	memcpy(&command.u1[4], &offset, 4);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL);
	if(status)
		return status;
	
	return 0;

} /* Function definition */

/* Increment pallet offset */
long StCoreIncrementOffset(unsigned char Target, unsigned char Pallet, double IncrementalOffset) {
	
	/***********************
	 Declare local variables
	***********************/
	long status, offset;
	coreCommandAssignmentType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreGetCommandAssignment(28, Target, Pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	offset = (long)(IncrementalOffset * 1000.0);
	memcpy(&command.u1[4], &offset, 4);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL);
	if(status)
		return status;
	
	return 0;

} /* Function definition */

/* Resume pallet movement when at mandatory stop */
long StCoreResumeMove(unsigned char Target, unsigned char Pallet) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandAssignmentType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreGetCommandAssignment(60, Target, Pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* Function definition */

/* Set ID of pallet at target */
long StCoreSetPalletID(unsigned char Target, unsigned char PalletID) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandAssignmentType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreGetCommandAssignment(64, Target, 0, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	command.u1[2] = PalletID;
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* Function definition */

/*******************************************************************************
********************************************************************************
 Command assignment
********************************************************************************
*******************************************************************************/

/* Assign command's ID, context, and manager index */
long coreGetCommandAssignment(unsigned char start, unsigned char target, unsigned char pallet, unsigned short direction, coreCommandAssignmentType *assign) {
	
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
	assign->commandID = start + (unsigned char)(direction > 0 && start < 64) + 2 * (unsigned char)(target == 0 && start != 64);
	
	/***********************
	 Check global references
	***********************/
	setCommandName(args.s[0], assign->commandID, sizeof(args.s[0]));
	if(pCoreCyclicStatus == NULL) {
		coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_ALLOC), "%s command assignment cannot reference cyclic data", &args);
		return stCORE_ERROR_ALLOC;
	}
	
	/********************************
	 Assign context and manager index
	********************************/
	if(target != 0) {
		if(target > coreTargetCount) {
			args.i[0] = target;
			args.i[1] = coreTargetCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Target %i context exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
					
		pPalletPresent = pCoreCyclicStatus + coreInterfaceConfig.targetStatusOffset + 3 * target + 1;
		if(*pPalletPresent < 1 || corePalletCount < *pPalletPresent) {
			args.i[0] = *pPalletPresent;
			args.i[1] = target;
			args.i[2] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i present at target %i exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		assign->index = *pPalletPresent;
		assign->context = target;
	}
	else {
		if(pallet < 1 || corePalletCount < pallet) {
			args.i[0] = pallet;
			args.i[1] = corePalletCount;
			coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INPUT), "Pallet %i context exceeds count [1, %i] of %s command", &args);
			return stCORE_ERROR_INPUT;
		}
		
		assign->index = pallet;
		assign->context = pallet;
	}
	
	return 0;
	
} /* Function definition */

/*******************************************************************************
********************************************************************************
 Command request
********************************************************************************
*******************************************************************************/

/* Request command for allocated pallet command buffer */
long coreCommandRequest(unsigned char index, SuperTrakCommand_t command, void *inst) {
	
	/***********************
	 Declare local variables
	***********************/
	coreCommandManagerType *pManager;
	coreCommandEntryType *pEntry;
	FormatStringArgumentsType args;
	
	/****************
	 Check references
	****************/
	if(pCoreCommandManager == NULL) {
		coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_ALLOC), "%s command request cannot reference command manager", &args);
		return stCORE_ERROR_ALLOC;
	}
	
	/**********************
	 Access command manager
	**********************/
	pManager = pCoreCommandManager + index - 1;
	pEntry = &pManager->buffer[pManager->write];
	
	/* Prepare message */
	setContextName(args.s[0], command.u1[0], sizeof(args.s[0]));
	args.i[0] = command.u1[1];
	setCommandName(args.s[1], command.u1[0], sizeof(args.s[1]));
	setDirectionName(args.s[2], command.u1[0], sizeof(args.s[2]));
	setDestinationTargetName(args.s[3], command.u1[0], command.u1[2], sizeof(args.s[3]));
	
	/* Check if pending or busy */
	if(GET_BIT(pEntry->status, CORE_COMMAND_PENDING) || GET_BIT(pEntry->status, CORE_COMMAND_BUSY)) {
		coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_BUFFER), "%s %i %s command rejected because buffer is full", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	/* Write command */
	memcpy(&pEntry->command, &command, sizeof(pEntry->command));
	
	/* Update status, tag instance, and share entry */
	CLEAR_BIT(pEntry->status, CORE_COMMAND_DONE);
	CLEAR_BIT(pEntry->status, CORE_COMMAND_ERROR);
	SET_BIT(pEntry->status, CORE_COMMAND_PENDING);
	pEntry->inst = inst;
	/**pEntryRef = &pEntry*/
	
	/* Debug comfirmation message */
	coreLogFormatMessage(USERLOG_SEVERITY_DEBUG, 5200, "%s %i %s command request (direction = %s, destination = %s)", &args);
	
	/* Increment write index and check if full */
	pManager->write = (pManager->write + 1) % CORE_COMMANDBUFFER_SIZE;
	pEntry = &pManager->buffer[pManager->write];
	if(GET_BIT(pEntry->status, CORE_COMMAND_PENDING) || GET_BIT(pEntry->status, CORE_COMMAND_BUSY)) {
		args.i[0] = index;
		args.i[1] = CORE_COMMANDBUFFER_SIZE;
		coreLogFormatMessage(USERLOG_SEVERITY_WARNING, coreEventCode(stCORE_WARNING_BUFFER), "Pallet %i command buffer is now full (size = %i)", &args);
	}
	
	return 0;
	
} /* Function definition */

/*******************************************************************************
********************************************************************************
 Command manager
********************************************************************************
*******************************************************************************/

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

/*******************************************************************************
********************************************************************************
 String handling
********************************************************************************
*******************************************************************************/

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
	else if(command == 64)
		coreStringCopy(str, "set pallet ID", size);
	else
		coreStringCopy(str, "unknown", size);
}

/* Write uppercase context name to str based on command */
void setContextName(char *str, unsigned char ID, unsigned long size) {
	if(ID == 16 || ID == 17 || ID == 24 || ID == 25 || ID == 28 || ID == 29 || ID == 60 || ID == 64)
		coreStringCopy(str, "Target", size);
	else if(ID == 18 || ID == 19 || ID == 26 || ID == 27 || ID == 30 || ID == 31 || ID == 62)
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
