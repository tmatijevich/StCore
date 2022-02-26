/*******************************************************************************
 * File: StCoreCyclic.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

long StCoreExit(void) {
	
	TMP_free(configPLCInterface.controlSize, (void**)control);
	TMP_free(configPLCInterface.statusSize, (void**)status);
	
	return 0;
	
} /* Function definition */
