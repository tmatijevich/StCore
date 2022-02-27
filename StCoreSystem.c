/*******************************************************************************
 * File: StCoreSystem.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

/* Register user's system command structure */
long StCoreSystemControl(struct StCoreSystemCommandType* control) {
	
	/* Guard null pointer */
	if(control == NULL)
		return -1;
	
	user.systemCommand = control;
	return 0;
	
} /* Function definition */


/* Write system commands to SuperTrak cyclic control data */
void StCoreRunSystemControl(void) {
	
	/***********************
	 Declare local variables
	***********************/
	/* See TrakMaster help for description of system control word */
	static unsigned short systemControl, previousControl;
	
	/*******************************************
	 Enable system and acknowledge system faults
	*******************************************/
	/* Enable and acknowledge faults only if user command has registered */
	if(user.systemCommand) {
		/* Enable/disabled (bit 0) 
		   Check user command, system control as enable signal source (1107), no configuration error, configuration has been saved */
		user.systemCommand->Enable && configEnableSource == 1 && !configError && saveParameters ? SET_BIT(systemControl, 0) : CLEAR_BIT(systemControl, 0);
		/* Acknowledge faults and warnings (bit 7) */
		user.systemCommand->ErrorReset && !GET_BIT(previousControl, 7) ? SET_BIT(systemControl, 7) : CLEAR_BIT(systemControl, 7);
	}
	
	/*********
	 Heartbeat
	*********/
	/* Toggle heartbeat (bit 15) */
	TOGGLE_BIT(systemControl, 15);
	
	/*********************************** 
	 Write system control to cyclic data 
	***********************************/
	*(control + configPLCInterface.systemControlOffset) = systemControl;
	
	/* Update previous control word */
	previousControl = systemControl;
	
} /* Function definition */
