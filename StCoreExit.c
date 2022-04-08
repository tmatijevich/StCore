/*******************************************************************************
 * File: StCoreExit.c
 * Author: Tyler Matijevich
 * Date: 2022-02-25
*******************************************************************************/

#include "StCoreMain.h"

/* Free internal memory */
long StCoreExit(void) {
	
	/* Declare local variables */
	unsigned long allocationSize;

	SuperTrakExit();
	
	/* Free global pointers */
	if(pCoreCyclicControl) {
		allocationSize = coreInterfaceConfig.controlSize;
		TMP_free(allocationSize, (void**)pCoreCyclicControl);
	}
	
	if(pCoreCyclicStatus) {
		allocationSize = coreInterfaceConfig.statusSize;
		TMP_free(allocationSize, (void**)pCoreCyclicStatus);
	}
	
	if(pCoreCommandManager) {
		allocationSize = sizeof(SuperTrakCommand_t) * CORE_COMMANDBUFFER_SIZE * corePalletCount;
		TMP_free(allocationSize, (void**)pCoreCommandManager);
	}
	
	return 0;
	
} /* Function definition */
