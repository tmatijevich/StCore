/*******************************************************************************
 * File: StCoreSystem.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

void clearOutputs(StCoreSystem_typ *inst);

/* System control interface */
void StCoreSystem(StCoreSystem_typ *inst) {
	
	/***********************
	 Declare local variables
	***********************/
	static StCoreSystem_typ *firstInst;
	unsigned short *pSystemControl, *pSystemStatus;
	unsigned char i, *pSectionStatus;
	unsigned long dataUInt32;
	
	/***********
	 Verify call
	***********/
	/* Disable */
	if(!inst->Enable) {
		clearOutputs(inst);
		return;
	}
	
	/* Instance */
	if(firstInst == NULL)
		firstInst = inst;
	else if(firstInst != inst) {
		if(inst->Error == false)
			coreLogMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_INST), "Multiple instances of StCoreSystem()");
		clearOutputs(inst);
		inst->Error = true;
		inst->StatusID = stCORE_ERROR_INST;
		return;
	}
	
	/* Global error */
	if(coreError) {
		clearOutputs(inst);
		inst->Error = true;
		inst->StatusID = coreStatusID;
		return;
	}
	
	/* Guard references */
	if(pCoreCyclicControl == NULL || pCoreCyclicStatus == NULL) {
		clearOutputs(inst);
		inst->Error = true;
		inst->StatusID = stCORE_ERROR_ALLOC;
		return;
	}
	
	/* Set outputs */
	inst->Valid = true;
	inst->Error = false;
	inst->StatusID = 0;
	
	/*******
	 Control
	*******/
	pSystemControl = (unsigned short*)(pCoreCyclicControl + coreInterfaceConfig.systemControlOffset);
	
	/* Enable */
	if(inst->EnableAllSections) SET_BIT(*pSystemControl, 0);
	else CLEAR_BIT(*pSystemControl, 0);
	
	/* Fault reset */
	if(inst->AcknowledgeFaults && !GET_BIT(*pSystemControl, 7)) SET_BIT(*pSystemControl, 7);
	else CLEAR_BIT(*pSystemControl, 7);
	
	/******
	 Status
	******/
	pSystemStatus = (unsigned short*)(pCoreCyclicStatus + coreInterfaceConfig.systemStatusOffset);
	
	inst->PalletsStopped = GET_BIT(*pSystemStatus, 4);
	inst->WarningPresent = GET_BIT(*pSystemStatus, 6);
	inst->FaultPresent = GET_BIT(*pSystemStatus, 7);
	inst->PalletCount = (unsigned char)(*(pSystemStatus + 1)); /* Access the next 16 bits */
	
	/* Extended information */
	SuperTrakServChanRead(0, stPAR_SYSTEM_FAULTS_ACTIVE, 1, 1, (unsigned long)&dataUInt32, sizeof(dataUInt32));
	for(i = 0; i < 32; i++) {
		if(GET_BIT(dataUInt32, i) && !GET_BIT(inst->Info.Warnings, i))
			coreLogFaultWarning(32 + i, 0);
	}
	memcpy(&inst->Info.Warnings, &dataUInt32, sizeof(inst->Info.Warnings));
	
	SuperTrakServChanRead(0, stPAR_SYSTEM_FAULTS_ACTIVE, 0, 1, (unsigned long)&dataUInt32, sizeof(dataUInt32));
	for(i = 0; i < 32; i++) {
		if(GET_BIT(dataUInt32, i) && !GET_BIT(inst->Info.Faults, i))
			coreLogFaultWarning(i, 0);
	}
	memcpy(&inst->Info.Faults, &dataUInt32, sizeof(inst->Info.Faults));
	
	/* Section information */
	inst->Info.SectionCount = coreInterfaceConfig.sectionCount;
	
	inst->Info.Enabled = true;
	inst->Info.Disabled = true;
	inst->Info.MotorPower = true;
	inst->Info.DisabledExternally = false;
	inst->Info.SectionWarningPresent = false;
	inst->Info.SectionFaultPresent = false;
	for(i = 0; i < coreInterfaceConfig.sectionCount; i++) {
		/* Reference the section status byte */
		pSectionStatus = pCoreCyclicStatus + coreInterfaceConfig.sectionStatusOffset + i;
		
		if(GET_BIT(*pSectionStatus, 0)) inst->Info.Disabled = false;
		else inst->Info.Enabled = false;
		if(!GET_BIT(*pSectionStatus, 2)) inst->Info.MotorPower = false;
		if(GET_BIT(*pSectionStatus, 5)) inst->Info.DisabledExternally = true;
		if(GET_BIT(*pSectionStatus, 6)) inst->Info.SectionWarningPresent = true;
		if(GET_BIT(*pSectionStatus, 7)) inst->Info.SectionFaultPresent = true;
	}
	
	/* Pallet information */
	
} /* Function defintion */

/* Clean instance outputs */
void clearOutputs(StCoreSystem_typ *inst) {
	inst->Valid = false;
	inst->Error = false;
	inst->StatusID = 0;
	inst->PalletsStopped = false;
	inst->WarningPresent = false;
	inst->FaultPresent = false;
	inst->PalletCount = 0;
	memset(&inst->Info, 0, sizeof(inst->Info));
}
