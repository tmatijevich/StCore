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
	unsigned char i, *pSectionStatus;
	
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
	inst->PalletCount = (unsigned char)(*(pSystemStatus + 1)); /* Access the next 16 bits */
	
	/* Extended information */
	SuperTrakServChanRead(0, stPAR_SYSTEM_FAULTS_ACTIVE, 1, 1, (unsigned long)&inst->Info.Warnings, sizeof(inst->Info.Warnings));
	SuperTrakServChanRead(0, stPAR_SYSTEM_FAULTS_ACTIVE, 0, 1, (unsigned long)&inst->Info.Faults, sizeof(inst->Info.Faults));
	
	inst->Info.SectionCount = coreControlInterfaceConfig.sectionCount;
	
	inst->Info.Enabled = true;
	inst->Info.Disabled = true;
	inst->Info.MotorPower = true;
	inst->Info.DisabledExternally = false;
	inst->Info.SectionWarningPresent = false;
	inst->Info.SectionFaultPresent = false;
	for(i = 0; i < coreControlInterfaceConfig.sectionCount; i++) {
		/* Reference the section status byte */
		pSectionStatus = pCyclicStatusData + coreControlInterfaceConfig.sectionStatusOffset + i;
		
		if(GET_BIT(*pSectionStatus, 0)) inst->Info.Disabled = false;
		else inst->Info.Enabled = false;
		if(!GET_BIT(*pSectionStatus, 2)) inst->Info.MotorPower = false;
		if(GET_BIT(*pSectionStatus, 5)) inst->Info.DisabledExternally = true;
		if(GET_BIT(*pSectionStatus, 6)) inst->Info.SectionWarningPresent = true;
		if(GET_BIT(*pSectionStatus, 7)) inst->Info.SectionFaultPresent = true;
	}
	
} /* Function defintion */
