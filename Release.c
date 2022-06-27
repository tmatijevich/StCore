/*******************************************************************************
 * File: StCore\Release.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "Release"

/* Prototypes */
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);

/* Release with local move configuration */
long StCoreSimpleRelease(unsigned char Target, unsigned char LocalMove) {
	return coreSimpleRelease(Target, LocalMove, NULL, NULL);
}

/* (Internal) Release with local move configuration */
long coreSimpleRelease(unsigned char target, unsigned char localMove, void *pInstance, coreCommandType **ppCommand) {
	
	/************************************************
	 Dependencies:
	  Global:
	   core.pSimpleRelease
	   core.targetCount
	   core.error
	   core.statusID
	  Subroutines:
	   logMessage
	************************************************/

	/***********************
	 Declare Local Variables
	***********************/
	coreFormatArgumentType args;
	coreCommandType *pSimpleCommand;
	
	if(core.error) {
		args.i[0] = target;
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(core.statusID), "Target simple release target %i aborted due to critical error in StCore", &args);
		return core.statusID;
	}
	
	if(core.pSimpleRelease == NULL) {
		args.i[0] = target;
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOCATION), "Target simple release target %i unable to reference command buffer", &args);
		return stCORE_ERROR_ALLOCATION;
	}
	
	if(target < 1 || core.targetCount < target) {
		args.i[0] = target;
		args.i[1] = core.targetCount;
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INDEX), "Target simple release target %i exceeds initial target count [1, %i]", &args);
		return stCORE_ERROR_INDEX;
	}
	
	if(localMove < 1 || 3 < localMove) {
		args.i[0] = localMove;
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INDEX), "Target simple release local move index %i exceeds limits [1, 3]", &args);
		return stCORE_ERROR_INDEX;
	}
	
	pSimpleCommand = core.pSimpleRelease + target - 1;
	
	if(GET_BIT(pSimpleCommand->status, CORE_COMMAND_BUSY) || GET_BIT(pSimpleCommand->status, CORE_COMMAND_PENDING)) {
		args.i[0] = target;
		args.i[1] = localMove;
		logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_BUFFER), "Target simple release target %i local move %i rejected due to command in progress", &args);
		return stCORE_ERROR_BUFFER;
	}
	
	/* Share the command and assign the instance */
	pSimpleCommand->pInstance = pInstance;
	if(ppCommand != NULL) *ppCommand = pSimpleCommand;
	
	/* Write local move configuration index and set status */
	pSimpleCommand->command.u1[0] = localMove; /* Store local move in first command byte */
	CLEAR_BIT(pSimpleCommand->status, CORE_COMMAND_DONE);
	CLEAR_BIT(pSimpleCommand->status, CORE_COMMAND_ERROR);
	SET_BIT(pSimpleCommand->status, CORE_COMMAND_PENDING);
	
	return 0;
	
}

/* Release pallet to target */
long StCoreReleasePallet(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget) {
	return coreReleasePallet(Target, Pallet, Direction, DestinationTarget, NULL, NULL);
}

/* (Internal) release pallet to target */
long coreReleasePallet(unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, void *pInstance, coreCommandType **ppCommand) {
	
	/************************************************
	 Dependencies:
	  Subroutines:
	   coreCommandCreate
	   coreCommandRequest
	************************************************/

	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(CORE_COMMAND_ID_RELEASE, target, pallet, direction, &create);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = create.commandID;
	command.u1[1] = create.context;
	command.u1[2] = destinationTarget;
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(create.index, command, pInstance, ppCommand);
	if(status)
		return status;
		
	return 0;
	
} /* End function */

/* Release pallet to target + offset */
long StCoreReleaseTargetOffset(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget, double TargetOffset) {
	return coreReleaseTargetOffset(Target, Pallet, Direction, DestinationTarget, TargetOffset, NULL, NULL);
}

/* (Internal) Release pallet to target + offset */
long coreReleaseTargetOffset(unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, double targetOffset, void *pInstance, coreCommandType **ppCommand) {
	
	/************************************************
	 Dependencies:
	  Subroutines:
	   coreCommandCreate
	   coreCommandRequest
	************************************************/

	/***********************
	 Declare local variables
	***********************/
	long status, offset;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(CORE_COMMAND_ID_OFFSET, target, pallet, direction, &create);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = create.commandID;
	command.u1[1] = create.context;
	command.u1[2] = destinationTarget;
	offset = (long)(targetOffset * 1000.0);
	memcpy(&command.u1[4], &offset, 4);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(create.index, command, pInstance, ppCommand);
	if(status)
		return status;
	
	return 0;

} /* End function */

/* Increment pallet offset */
long StCoreReleaseIncrementalOffset(unsigned char Target, unsigned char Pallet, double IncrementalOffset) {
	return coreReleaseIncrementalOffset(Target, Pallet, IncrementalOffset, NULL, NULL);
}

/* (Internal) Increment pallet offset */
long coreReleaseIncrementalOffset(unsigned char target, unsigned char pallet, double incrementalOffset, void *pInstance, coreCommandType **ppCommand) {
	
	/************************************************
	 Dependencies:
	  Subroutines:
	   coreCommandCreate
	   coreCommandRequest
	************************************************/
	
	/***********************
	 Declare local variables
	***********************/
	long status, offset;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(CORE_COMMAND_ID_INCREMENT, target, pallet, 0, &create);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = create.commandID;
	command.u1[1] = create.context;
	offset = (long)(incrementalOffset * 1000.0);
	memcpy(&command.u1[4], &offset, 4);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(create.index, command, pInstance, ppCommand);
	if(status)
		return status;
	
	return 0;

} /* End function */

/* Resume pallet movement when at mandatory stop */
long StCoreContinueMove(unsigned char Target, unsigned char Pallet) {
	return coreContinueMove(Target, Pallet, NULL, NULL);
}

/* (Internal) Resume pallet movement when at mandatory stop */
long coreContinueMove(unsigned char target, unsigned char pallet, void *pInstance, coreCommandType **ppCommand) {
	
	/************************************************
	 Dependencies:
	  Subroutines:
	   coreCommandCreate
	   coreCommandRequest
	************************************************/
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(CORE_COMMAND_ID_CONTINUE, target, pallet, 0, &create);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = create.commandID;
	command.u1[1] = create.context;
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(create.index, command, pInstance, ppCommand);
	if(status)
		return status;
	
	return 0;
	
} /* End function */

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}
