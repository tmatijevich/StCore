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

	/* Free global pointers */
	if(pCoreCyclicControl) {
		allocationSize = coreInterfaceConfig.controlSize;
		TMP_free(allocationSize, (void**)&pCoreCyclicControl);
	}
	
	if(pCoreCyclicStatus) {
		allocationSize = coreInterfaceConfig.statusSize;
		TMP_free(allocationSize, (void**)&pCoreCyclicStatus);
	}
	
	if(pCoreCommandBuffer) {
		allocationSize = sizeof(SuperTrakCommand_t) * CORE_COMMANDBUFFER_SIZE * corePalletCount;
		TMP_free(allocationSize, (void**)&pCoreCommandBuffer);
	}
	
	if(pCoreBufferControl) {
		allocationSize = sizeof(coreBufferControlType) * corePalletCount;
		TMP_free(allocationSize, (void**)&pCoreBufferControl);
	}
	
	return 0;
	
} /* Function definition */
