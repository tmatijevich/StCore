/*******************************************************************************
 * File: StCoreSystem.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

/* Register user's system command structure */
long StCoreSystemControl(struct StCoreSystemCommandType* command) {
	
	/* Guard null pointer */
	if(command == NULL)
		return -1;
	
	user.system.command = command;
	return 0;
	
} /* Function definition */


/* Register user's system status structure */
long StCoreSystemStatus(StCoreSystemStatusType* status) {
	
	/* Guard null pointer */
	if(status == NULL) 
		return -1;
		
	user.system.status = status;
	return 0;
	
} /* Function definition */


/* Write system commands to SuperTrak cyclic control data */
void StCoreRunSystemControl(void) {
	
	/***********************
	 Declare local variables
	***********************/
	/* See TrakMaster help for description of system control word */
	unsigned short *systemControl;
	
	/******************
	 Access cyclic data
	******************/
	/* Check if cyclic control data was initialized */
	if(controlData == NULL) 
		return -1;
	
	/* Access system control word */
	systemControl = controlData + configPLCInterface.systemControlOffset;
	
	/**********************
	 Enable and acknowledge
	**********************/
	/* Enable and acknowledge faults only if user command has registered */
	if(user.system.command) {
		/* Enable/disabled system (bit 0) */
		/* Check user command, system control as enable signal source (1107), no configuration error, configuration has been saved */
		user.system.command->EnableAllSections && !configError && saveParameters ? SET_BIT(*systemControl, 0) : CLEAR_BIT(*systemControl, 0);
		/* Acknowledge faults and warnings (bit 7) */
		user.system.command->AcknowledgeFaults && !GET_BIT(*systemControl, 7) ? SET_BIT(*systemControl, 7) : CLEAR_BIT(*systemControl, 7);
	}
	
	/*********
	 Heartbeat
	*********/
	/* Toggle heartbeat (bit 15) */
	TOGGLE_BIT(*systemControl, 15);
	
} /* Function definition */
