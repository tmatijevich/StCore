/*******************************************************************************
 * File: StCoreExit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

long StCoreExit(void) {
	
	TMP_free(configPLCInterface.controlSize, (void**)controlData);
	TMP_free(configPLCInterface.statusSize, (void**)statusData);
	TMP_free(sizeof(StCoreSectionCommandType*) * configPLCInterface.sectionCount, (void**)user.section.command);
	
	SuperTrakExit();
	
	return 0;
	
} /* Function definition */
