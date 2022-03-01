/*******************************************************************************
 * File: StCoreCommand.c
 * Author: Tyler Matijevich
 * Date: 2022-02-28
*******************************************************************************/

#include "StCoreMain.h"

/* Release pallet to target */
long StCoreReleaseToTarget(unsigned char currentTarget, unsigned char palletID, unsigned short direction, unsigned char destinationTarget) {
	
	/***********************
	 Declare local variables
	***********************/
	unsigned char targetStatus, presentPalletID, commandBufferIndex, commandID, *pWriteIndex, *pBufferFull;
	SuperTrakCommand_t *pCommand;
	
	/************
	 Check inputs
	************/
	/* Check if the target number exceeds the initial target count from StCoreInit() */
	if(currentTarget > configInitialTargetCount)
		return -1;
		
	/* Check Pallet ID under target status is within [1, configUserPalletCount] */
	if(currentTarget > 0) {
		targetStatus = *(statusData + configPLCInterface.targetStatusOffset + 3 * currentTarget);
		presentPalletID = *(statusData + configPLCInterface.targetStatusOffset + 3 * currentTarget + 1);
		/* Check if present and ID in range */
		if(!GET_BIT(targetStatus, 0) || presentPalletID < 1 || presentPalletID > configUserPalletCount) {
			commandBufferIndex = configUserPalletCount - 1; /* Make an additional command buffer for non-present or out-of-range pallet IDs */
			commandID = 
		}
		else {
		
		}
	}
	/* Check palletID if currentTarget is 0 */
	/* Check palletID within [1, configUserPalletCount] */
	
	
	return 0;
	
} /* Function definition */
