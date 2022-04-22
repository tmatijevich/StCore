/*******************************************************************************
 * File: StCore\Release.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "Main.h"

/* Release pallet to target */
long StCoreReleasePallet(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget) {
	return coreReleasePallet(Target, Pallet, Direction, DestinationTarget, NULL, NULL);
}

/* (Internal) release pallet to target */
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
long StCoreReleaseTargetOffset(unsigned char Target, unsigned char Pallet, unsigned short Direction, unsigned char DestinationTarget, double TargetOffset) {
	return coreReleaseTargetOffset(Target, Pallet, Direction, DestinationTarget, TargetOffset, NULL, NULL);
}

/* (Internal) Release pallet to target + offset */
long coreReleaseTargetOffset(unsigned char target, unsigned char pallet, unsigned short direction, unsigned char destinationTarget, double targetOffset, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare local variables
	***********************/
	long status, offset;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(24, target, pallet, direction, &create);
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
	
	/***********************
	 Declare local variables
	***********************/
	long status, offset;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(28, target, pallet, 0, &create);
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
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType create;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(60, target, pallet, 0, &create);
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
