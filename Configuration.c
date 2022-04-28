/*******************************************************************************
 * File: StCore\Configuration.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "Main.h"

/* Set ID of pallet at target */
long StCoreSetPalletID(unsigned char Target, unsigned char PalletID) {
	return coreSetPalletID(Target, PalletID, NULL, NULL);
}

/* (Internal) Set ID of pallet at target */
long coreSetPalletID(unsigned char target, unsigned char palletID, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(64, target, 0, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	command.u1[2] = palletID;
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* End function */

/* Set pallet velocity and/or acceleration */
long StCoreSetMotionParameters(unsigned char Target, unsigned char Pallet, double Velocity, double Acceleration) {
	return coreSetMotionParameters(Target, Pallet, Velocity, Acceleration, NULL, NULL);
}

/* (Internal) Set pallet velocity and/or acceleration */
long coreSetMotionParameters(unsigned char target, unsigned char pallet, double velocity, double acceleration, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	unsigned short value;
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(68, target, pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	value = (unsigned short)velocity;
	memcpy(&command.u1[2], &value, 2);
	value = (unsigned short)(acceleration / 1000.0);
	memcpy(&command.u1[4], &value, 2);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* End function */

/* Set pallet shelf width and offset */
long StCoreSetMechanicalParameters(unsigned char Target, unsigned char Pallet, double ShelfWidth, double CenterOffset) {
	return coreSetMechanicalParameters(Target, Pallet, ShelfWidth, CenterOffset, NULL, NULL);
}

/* (Internal) Set pallet shelf width and offset */
long coreSetMechanicalParameters(unsigned char target, unsigned char pallet, double shelfWidth, double centerOffset, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	unsigned short value;
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(72, target, pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	value = (unsigned short)(shelfWidth * 10.0); /* Units of 0.1 mm */
	memcpy(&command.u1[2], &value, 2);
	value = (unsigned short)(centerOffset * 10.0); /* Units of 0.1 mm */
	memcpy(&command.u1[4], &value, 2);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* End function */

/* Set pallet control parameters */
long StCoreSetControlParameters(unsigned char Target, unsigned char Pallet, unsigned char ControlGainSet, double MovingFilter, double StationaryFilter) {
	return coreSetControlParameters(Target, Pallet, ControlGainSet, MovingFilter, StationaryFilter, NULL, NULL);
}

/* (Internal) Set pallet control parameters */
long coreSetControlParameters(unsigned char target, unsigned char pallet, unsigned char controlGainSet, double movingFilter, double stationaryFilter, void *pInstance, coreCommandType **ppCommand) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(76, target, pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	command.u1[2] = controlGainSet;
	command.u1[3] = (unsigned char)(fmax(0.0, fmin(99.0, movingFilter * 100.0)));
	command.u1[4] = (unsigned char)(fmax(0.0, fmin(99.0, stationaryFilter * 100.0)));
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* End function */
