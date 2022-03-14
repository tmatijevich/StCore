/*******************************************************************************
 * File: StCoreSection.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

void StCoreSection(StCoreSection_typ *inst) {
	
	/***********************
	 Declare local variables
	***********************/
	static StCoreSection_typ *firstInst[stCORE_SECTION_MAX];
	unsigned char *pSectionControl, *pSectionStatus;
	
	/*************
	 Clear outputs
	*************/
	inst->Valid = false;
	inst->Error = false;
	inst->StatusID = 0;
	inst->Enabled = false;
	inst->UnrecognizedPalletsPresent = false;
	inst->MotorPowerOn = false;
	inst->PalletsRecovering = false;
	inst->LocatingPallets = false;
	inst->DisabledExternally = false;
	inst->WarningPresent = false;
	inst->FaultPresent = false;
	
	/*******
	 Disable
	*******/
	if(!inst->Enable) {
		inst->StatusID = ERR_FUB_ENABLE_FALSE;
		return;
	}
	
	/************
	 Verify index
	************/
	if(inst->Section < 1 || coreControlInterfaceConfig.sectionCount < inst->Section) { 
		inst->Error = true;
		inst->StatusID = -1;
		return;
	}
	
	/***************
	 Verify instance
	***************/
	if(firstInst[inst->Section] == NULL)
		firstInst[inst->Section] = inst;
	else if(firstInst[inst->Section] != inst) {
		inst->Error = true;
		inst->StatusID = -1;
		return;
	}
	
	inst->Valid = true;
	
	/*******
	 Control
	*******/
	pSectionControl = pCyclicControlData + coreControlInterfaceConfig.sectionControlOffset + inst->Section - 1;
	
	if(inst->EnableSection) SET_BIT(*pSectionControl, 0);
	else CLEAR_BIT(*pSectionControl, 0);
	
	if(inst->AcknowledgeFaults && !GET_BIT(*pSectionControl, 7)) SET_BIT(*pSectionControl, 7);
	else CLEAR_BIT(*pSectionControl, 7);
	
	/******
	 Status
	******/
	pSectionStatus = pCyclicStatusData + coreControlInterfaceConfig.sectionStatusOffset + inst->Section - 1;
	
	inst->Enabled = GET_BIT(*pSectionStatus, 0);
	inst->UnrecognizedPalletsPresent = GET_BIT(*pSectionStatus, 1);
	inst->MotorPowerOn = GET_BIT(*pSectionStatus, 2);
	inst->PalletsRecovering = GET_BIT(*pSectionStatus, 3);
	inst->LocatingPallets = GET_BIT(*pSectionStatus, 4);
	inst->DisabledExternally = GET_BIT(*pSectionStatus, 5);
	inst->WarningPresent = GET_BIT(*pSectionStatus, 6);
	inst->FaultPresent = GET_BIT(*pSectionStatus, 7);
	
} /* Function definition */
