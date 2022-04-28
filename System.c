/*******************************************************************************
 * File: StCore\System.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "Main.h"

static void clearOutputs(StCoreSystem_typ *inst);

enum systemStateEnum {
	SYSTEM_STATE_DISABLED = 0,
	SYSTEM_STATE_EXECUTING = 1,
	SYSTEM_STATE_ERROR = 255
};

/* System core interface */
void StCoreSystem(StCoreSystem_typ *inst) {
	
	/***********************
	 Declare local variables
	***********************/
	static StCoreSystem_typ *usedInst;
	unsigned short *pSystemControl, *pSystemStatus;
	unsigned long dataUInt32;
	long i;
	unsigned char *pSectionStatus;
	SuperTrakPalletInfo_t palletInfo[255];
		
	/************
	 Switch state
	************/
	/* Interrupt if disabled */
	if(!inst->Enable)
		inst->Internal.State = SYSTEM_STATE_DISABLED;
		
	switch(inst->Internal.State) {
		case SYSTEM_STATE_DISABLED:
			clearOutputs(inst);
			if(inst->Enable) {
				/* Register isntance */
				if(usedInst == NULL) {
					usedInst = inst;
					inst->Internal.State = SYSTEM_STATE_EXECUTING;
				}
				else {
					coreLogMessage(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INST), "Multiple instances of StCoreSystem()");
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INST;
					inst->Internal.State = SYSTEM_STATE_ERROR;
				}
			}
			/* Unregister instance */
			else if(usedInst == inst) {
				usedInst = NULL;
			}
			break;
			
		case SYSTEM_STATE_EXECUTING:
			/* Check core error */
			if(core.error) {
				clearOutputs(inst);
				inst->Error = true;
				inst->StatusID = core.statusID;
				inst->Internal.State = SYSTEM_STATE_ERROR;
				break;
			}
			/* Check references */
			if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL) {
				coreLogMessage(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOC), "StCoreSystem() cannot reference cyclic control or status data due to null pointer");
				clearOutputs(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_ALLOC;
				inst->Internal.State = SYSTEM_STATE_ERROR;
				break;
			}
			/* Report valid */
			inst->Valid = true;
			
			/*******
			 Control
			*******/
			pSystemControl = (unsigned short*)(core.pCyclicControl + core.interface.systemControlOffset);
			
			/* Enable */
			if(inst->EnableAllSections) SET_BIT(*pSystemControl, 0);
			else CLEAR_BIT(*pSystemControl, 0);
			
			/* Clear faults/warnings */
			if(inst->AcknowledgeFaults) SET_BIT(*pSystemControl, 7);
			else CLEAR_BIT(*pSystemControl, 7);
			
			/******
			 Status
			******/
			pSystemStatus = (unsigned short*)(core.pCyclicStatus + core.interface.systemStatusOffset);
			
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
			inst->Info.SectionCount = core.interface.sectionCount;
			
			inst->Info.Enabled = true;
			inst->Info.Disabled = true;
			inst->Info.MotorPower = true;
			inst->Info.DisabledExternally = false;
			inst->Info.SectionWarningPresent = false;
			inst->Info.SectionFaultPresent = false;
			for(i = 0; i < core.interface.sectionCount; i++) {
				/* Reference the section status byte */
				pSectionStatus = core.pCyclicStatus + core.interface.sectionStatusOffset + i;
				
				if(GET_BIT(*pSectionStatus, 0)) inst->Info.Disabled = false;
				else inst->Info.Enabled = false;
				if(!GET_BIT(*pSectionStatus, 2)) inst->Info.MotorPower = false;
				if(GET_BIT(*pSectionStatus, 5)) inst->Info.DisabledExternally = true;
				if(GET_BIT(*pSectionStatus, 6)) inst->Info.SectionWarningPresent = true;
				if(GET_BIT(*pSectionStatus, 7)) inst->Info.SectionFaultPresent = true;
			}
			
			/* Pallet information */
			inst->Info.PalletRecoveringCount = 0;
			inst->Info.PalletInitializingCount = 0;
			SuperTrakGetPalletInfo((unsigned long)&palletInfo, COUNT_OF(palletInfo), false);
			for(i = 0; i < COUNT_OF(palletInfo); i++) {
				if(GET_BIT(palletInfo[i].status, 0)) { /* Pallet present */
					if(GET_BIT(palletInfo[i].status, 1))
						inst->Info.PalletRecoveringCount++;
					if(GET_BIT(palletInfo[i].status, 5))
						inst->Info.PalletInitializingCount++;
				}
			}
			
			break;
			
		default:
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset) {
				clearOutputs(inst);
				if(inst == usedInst)
					inst->Internal.State = SYSTEM_STATE_EXECUTING;
				else
					inst->Internal.State = SYSTEM_STATE_DISABLED;
			}
			
			break;
	}
	
	inst->Internal.PreviousErrorReset = inst->ErrorReset;
	
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