/*******************************************************************************
 * File: StCoreSection.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

/* Register user section command structure */
long StCoreSectionControl(unsigned char section, struct StCoreSectionCommandType* control) {	
	
	if(control == NULL || section < 1 || section > configPLCInterface.sectionCount)
		return -1;
	
	
	
	return 0;
	
} /* Function definition */
