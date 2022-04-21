/*******************************************************************************
 * File: StCoreSection.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "StCoreMain.h"

static void clearOutputs(StCoreSection_typ *inst);

enum sectionStateEnum {
	SECTION_STATE_DISABLED = 0,
	SECTION_STATE_EXECUTING = 1,
	SECTION_STATE_ERROR = 255
};

/* Section core interface */
void StCoreSection(StCoreSection_typ *inst) {
	
	/***********************
	 Declare local variables
	***********************/
	static StCoreSection_typ *usedInst[CORE_SECTION_MAX];
	FormatStringArgumentsType args;
	unsigned char *pSectionControl, *pSectionStatus;
	unsigned long dataUInt32;
	long i;
	unsigned short dataUInt16;
	
	/************
	 Switch state
	************/
	/* Interrupt if disabled */
	if(inst->Enable == false)
		inst->Internal.State = SECTION_STATE_DISABLED;
	
	switch(inst->Internal.State) {
		case SECTION_STATE_DISABLED:
			clearOutputs(inst);
			if(inst->Enable) {
				/* Check select */
				if(inst->Section < 1 || core.interface.sectionCount < inst->Section) {
					args.i[0] = inst->Section;
					args.i[1] = core.interface.sectionCount;
					coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INST), "StCoreSection() called with section %i exceeds limits [1, %]", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INST;
					inst->Internal.State = SECTION_STATE_ERROR;
				}
				/* Register instance */
				else if(usedInst[inst->Section - 1] == NULL) {
					usedInst[inst->Section - 1] = inst;
					inst->Internal.State = SECTION_STATE_EXECUTING;
				}
				/* Instance multiple */
				else {
					args.i[0] = inst->Section;
					coreLogFormat(USERLOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INST), "Multiple instances of StCoreSection() with section %i", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INST;
					inst->Internal.State = SECTION_STATE_ERROR;
				}
			}
			/* Unregister instance */
			else if(1 <= inst->Section && inst->Section <= core.interface.sectionCount) {
				if(usedInst[inst->Section - 1] == inst)
					usedInst[inst->Section - 1] = NULL;
			}
			break;
			
		case SECTION_STATE_EXECUTING:
			/* Check references */
			if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL) {
				/* Do not spam the logger, StCoreSystem() logs this */
				clearOutputs(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_ALLOC;
				inst->Internal.State = SECTION_STATE_ERROR;
				break;
			}
			
			inst->Valid = true;
			
			/*******
			 Control
			*******/
			pSectionControl = core.pCyclicControl + core.interface.sectionControlOffset + inst->Section - 1;
			
			/* Enable */
			if(inst->EnableSection) SET_BIT(*pSectionControl, 0);
			else CLEAR_BIT(*pSectionControl, 0);
			
			/* Clear faults/warnings */
			if(inst->AcknowledgeFaults) SET_BIT(*pSectionControl, 7);
			else CLEAR_BIT(*pSectionControl, 7);
			
			/******
			 Status
			******/
			pSectionStatus = core.pCyclicStatus + core.interface.sectionStatusOffset + inst->Section - 1;
			
			inst->Enabled = GET_BIT(*pSectionStatus, 0);
			inst->UnrecognizedPallets = GET_BIT(*pSectionStatus, 1);
			inst->MotorPowerOn = GET_BIT(*pSectionStatus, 2);
			inst->PalletsRecovering = GET_BIT(*pSectionStatus, 3);
			inst->LocatingPallets = GET_BIT(*pSectionStatus, 4);
			inst->DisabledExternally = GET_BIT(*pSectionStatus, 5);
			inst->WarningPresent = GET_BIT(*pSectionStatus, 6);
			inst->FaultPresent = GET_BIT(*pSectionStatus, 7);
			
			/********************
			 Extended information
			********************/
			SuperTrakServChanRead(inst->Section, stPAR_SECTION_FAULTS_ACTIVE, 1, 1, (unsigned long)&dataUInt32, sizeof(dataUInt32));
			for(i = 0; i < 32; i++) {
				if(GET_BIT(dataUInt32, i) && !GET_BIT(inst->Info.Warnings, i))
					coreLogFaultWarning(32 + i, inst->Section);
			}
			memcpy(&inst->Info.Warnings, &dataUInt32, sizeof(inst->Info.Warnings));
			
			SuperTrakServChanRead(inst->Section, stPAR_SECTION_FAULTS_ACTIVE, 0, 1, (unsigned long)&dataUInt32, sizeof(dataUInt32));
			for(i = 0; i < 32; i++) {
				if(GET_BIT(dataUInt32, i) && !GET_BIT(inst->Info.Faults, i))
					coreLogFaultWarning(i, inst->Section);
			}
			memcpy(&inst->Info.Faults, &dataUInt32, sizeof(inst->Info.Faults));
			
			SuperTrakServChanRead(inst->Section, stPAR_SECTION_PALLET_COUNT, 0, 1, (unsigned long)&dataUInt16, sizeof(dataUInt16));
			inst->Info.PalletCount = (unsigned short)dataUInt16;
			
			break;
			
		default:
			/* Wait for rising edge */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset) {
				clearOutputs(inst); /* Clear error */
				/* Check if used instance (valid select) and go to EXECUTING otherwise DISABLED */
				if(1 <= inst->Section && inst->Section <= core.interface.sectionCount) {
					if(usedInst[inst->Section - 1] == inst)
						inst->Internal.State = SECTION_STATE_EXECUTING;
					else
						inst->Internal.State = SECTION_STATE_DISABLED;
				}
				else
					inst->Internal.State = SECTION_STATE_DISABLED;
			}
			break;
	}
	
	inst->Internal.PreviousErrorReset = inst->ErrorReset;
	
} /* Function definition */

/* Clear StCoreSection() outputs */
void clearOutputs(StCoreSection_typ *inst) {
	inst->Valid = false;
	inst->Error = false;
	inst->StatusID = 0;
	inst->Enabled = false;
	inst->UnrecognizedPallets = false;
	inst->PalletsRecovering = false;
	inst->LocatingPallets = false;
	inst->DisabledExternally = false;
	inst->WarningPresent = false;
	inst->FaultPresent = false;
}
