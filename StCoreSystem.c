/*******************************************************************************
 * File: StCoreSystem.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

long StCoreSystemControl(struct StCoreSystemCommandType* control) {
	
	user.systemCommand = control;
	
	return 0;
	
} /* Function definition */

void StCoreRunSystemControl(void) {
	
	static unsigned short systemControl, previousControl;
	
	/* See TrakMaster help for description of system control word */
	
	/* Enable and acknowledge faults is system command is registered */
	if(user.systemCommand) {
		/* Enable/disabled (bit 0) 
		   Check user command, system control as enable signal source (1107), no configuration error, configuration has been saved */
		systemControl = user.systemCommand->Enable && configEnableSource == 1 && !configError && saveParameters ? SET_BIT(systemControl, 0) : CLEAR_BIT(systemControl, 0);
		/* Acknowledge faults and warnings (bit 7) */
		systemControl = user.systemCommand->ErrorReset && !GET_BIT(previousControl, 7) ? SET_BIT(systemControl, 7) : CLEAR_BIT(systemControl, 7);
	}
	
	/* Toggle heartbeat (bit 15) */
	TOGGLE_BIT(systemControl, 15);
	
	/* Write system control to cyclic data */
	*(control + configPLCInterface.systemControlOffset) = systemControl;
	previousControl = systemControl;
	
} /* Function definition */
