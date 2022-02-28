/*******************************************************************************
 * File: StCoreSection.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

/* Register user section command structure */
long StCoreSectionControl(unsigned char section, struct StCoreSectionCommandType* command) {	
	
	/* Guard null pointer and check range */
	if(command == NULL || section < 1 || section > configPLCInterface.sectionCount)
		return -1;
	
	/* Register user section command reference to array */
	*(user.sectionCommand + section - 1) = command;
	
	return 0;
	
} /* Function definition */


/* Write system commands to SuperTrak cyclic control data */
void StCoreRunSectionControl(void) {
	
	/***********************
	 Declare local variables
	***********************/
	StCoreSectionCommandType *command;
	/* See TrakMaster help for description of section control word */
	unsigned char *sectionControl, i;
	
	/* Check if cyclic control data was initialized */
	if(control == NULL) 
		return -1;
	
	/****************
	 Run all sections
	****************/
	for(i = 0; i < configPLCInterface.sectionCount; i++) {
		/******************
		 Access cyclic data
		******************/
		sectionControl = control + configPLCInterface.sectionControlOffset + i;
		
		/* Get the pointer for this section command */
		command = *(user.sectionCommand + i); 
		
		/**********************
		 Enable and acknowledge
		**********************/
		/* Enable and acknowledge only if user command has registered */
		if(command) {
			/* Enable/disable section (bit 0) */
			/* Check: 1. User command 2. Enable signal source 3. Error 4. Saved */
			command->Enable && !configError && saveParameters ? SET_BIT(*sectionControl, 0) : CLEAR_BIT(*sectionControl, 0);
			/* Acknowledge faults and warnings (bit 7) */
			command->ErrorReset && !GET_BIT(*sectionControl, 7) ? SET_BIT(*sectionControl, 7) : CLEAR_BIT(*sectionControl, 7);
		}
	}
	
} /* Function definition */
