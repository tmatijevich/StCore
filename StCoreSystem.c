/*******************************************************************************
 * File: StCoreSystem.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

void StCoreSystem(StCoreSystem_typ *inst) {
	
	/***********************
	 Declare local variables
	***********************/
	static StCoreSystem_typ *firstInst;
	unsigned short *pSystemControl, *pSystemStatus;
	
	/*******
	 Disable
	*******/
	if(!(inst->Enable)) {
		inst->Valid = false;
		inst->Error = false;
		inst->StatusID = ERR_FUB_ENABLE_FALSE;
		inst->PalletsStopped = false;
		inst->WarningPresent = false;
		inst->FaultPresent = false;
		inst->PalletCount = 0;
		memset(&(inst->Info), 0, sizeof(inst->Info));
		return;
	}
	
	/***************
	 Verify instance
	***************/
	if(firstInst == NULL)
		firstInst = inst;
	else if(firstInst != inst) {
		inst->Valid = false;
		inst->Error = true;
		inst->StatusID = -1;
		inst->PalletsStopped = false;
		inst->WarningPresent = false;
		inst->FaultPresent = false;
		inst->PalletCount = 0;
		memset(&(inst->Info), 0, sizeof(inst->Info));
		return;
	}
	
	/*******
	 Control
	*******/
	pSystemControl = pCyclicControlData + coreControlInterfaceConfig.systemControlOffset;
	
	/* Enable */
	if(inst->EnableAllSections) SET_BIT(*pSystemControl, 0);
	else CLEAR_BIT(*pSystemControl, 0);
	
	/* Fault reset */
	if(inst->AcknowledgeFaults && !GET_BIT(*pSystemControl, 7)) SET_BIT(*pSystemControl, 7);
	else CLEAR_BIT(*pSystemControl, 7);
	
	/******
	 Status
	******/
	pSystemStatus = pCyclicStatusData + coreControlInterfaceConfig.systemStatusOffset;
	
	inst->PalletsStopped = GET_BIT(*pSystemStatus, 4);
	inst->WarningPresent = GET_BIT(*pSystemStatus, 6);
	inst->FaultPresent = GET_BIT(*pSystemStatus, 7);
	inst->PalletCount = (unsigned char)(*(pSystemStatus + 1)); /* Access the next uint */
	memset(&(inst->Info), 0, sizeof(inst->Info));
	
} /* Function defintion */
