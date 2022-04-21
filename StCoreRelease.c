/*******************************************************************************
 * File: StCoreRelease.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

/* Release pallet to target */
long StCoreReleaseToTarget(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget) {
	return coreReleasePallet(Target, Pallet, Direction, DestinationTarget, NULL, NULL);
}

/* Release pallet to a target */
long coreReleasePallet(unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(16, target, pallet, direction, &create);
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
long StCoreReleaseToOffset(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget, double TargetOffset) {
	
	/***********************
	 Declare local variables
	***********************/
	long status, offset;
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(24, Target, Pallet, Direction, &assign);
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
	status = coreCommandRequest(assign.index, command, NULL, NULL);
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
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(28, Target, Pallet, 0, &assign);
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
	status = coreCommandRequest(assign.index, command, NULL, NULL);
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
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(60, Target, Pallet, 0, &assign);
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
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* Function definition */
