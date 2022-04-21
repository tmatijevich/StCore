/*******************************************************************************
 * File: StCore\Configuration.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "Main.h"

/* Set ID of pallet at target */
long StCoreSetPalletID(unsigned char Target, unsigned char PalletID) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(64, Target, 0, 0, &assign);
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
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* Function definition */

/* Set pallet velocity and/or acceleration */
long StCoreSetMotionParameters(unsigned char Target, unsigned char Pallet, double Velocity, double Acceleration) {
	
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
	status = coreCommandCreate(68, Target, Pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	value = (unsigned short)Velocity;
	memcpy(&command.u1[2], &value, 2);
	value = (unsigned short)(Acceleration / 1000.0);
	memcpy(&command.u1[4], &value, 2);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* Function definition */

/* Set pallet shelf width and offset */
long StCoreSetMechanicalParameters(unsigned char Target, unsigned char Pallet, double ShelfWidth, double CenterOffset) {
	
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
	status = coreCommandCreate(72, Target, Pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	value = (unsigned short)(ShelfWidth * 10.0); /* Units of 0.1 mm */
	memcpy(&command.u1[2], &value, 2);
	value = (unsigned short)(CenterOffset * 10.0); /* Units of 0.1 mm */
	memcpy(&command.u1[4], &value, 2);
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* Function definition */

/* Set pallet control parameters */
long StCoreSetControlParameters(unsigned char Target, unsigned char Pallet, unsigned char ControlGainSet, double MovingFilter, double StationaryFilter) {
	
	/***********************
	 Declare local variables
	***********************/
	long status;
	coreCommandCreateType assign;
	SuperTrakCommand_t command;
	
	/**********************
	 Get command assignment
	**********************/
	status = coreCommandCreate(76, Target, Pallet, 0, &assign);
	if(status)
		return status;
	
	/**************
	 Create command
	**************/
	memset(&command, 0, sizeof(command));
	command.u1[0] = assign.commandID;
	command.u1[1] = assign.context;
	command.u1[2] = ControlGainSet;
	command.u1[3] = (unsigned char)(fmax(0.0, fmin(99.0, MovingFilter * 100.0)));
	command.u1[4] = (unsigned char)(fmax(0.0, fmin(99.0, StationaryFilter * 100.0)));
	
	/***************
	 Request command
	***************/
	status = coreCommandRequest(assign.index, command, NULL, NULL);
	if(status)
		return status;
	
	return 0;
	
} /* Function definition */
