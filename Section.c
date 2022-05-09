/*******************************************************************************
 * File: StCore\Section.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "Section"

/* Prototypes */
static void resetOutput(StCoreSection_typ *inst);
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);

/* Section core interface */
void StCoreSection(StCoreSection_typ *inst) {
	
	/***********************
	 Declare Local Variables
	***********************/
	static StCoreSection_typ *usedInst[CORE_SECTION_MAX];
	coreFormatArgumentType args;
	unsigned char *pSectionControl, *pSectionStatus;
	unsigned long dataUInt32;
	long i;
	unsigned short dataUInt16;
	
	/************
	 Switch State
	************/
	/* Interrupt if disabled */
	if(inst->Enable == false)
		inst->Internal.State = CORE_FUNCTION_DISABLED;
	
	switch(inst->Internal.State) {
		case CORE_FUNCTION_DISABLED:
			resetOutput(inst);
			if(inst->Enable) {
				/* Check select */
				if(inst->Section < 1 || core.interface.sectionCount < inst->Section) {
					args.i[0] = inst->Section;
					args.i[1] = core.interface.sectionCount;
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INPUT), "Section %i context of StCoreSection call exceeds limits [1, %]", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INPUT;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
				/* Register instance */
				else if(usedInst[inst->Section - 1] == NULL) {
					usedInst[inst->Section - 1] = inst;
					inst->Internal.Select = inst->Section;
					inst->Internal.State = CORE_FUNCTION_EXECUTING;
				}
				/* Instance multiple */
				else {
					args.i[0] = inst->Section;
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INST), "Multiple instances of StCoreSection with section %i", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INST;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
			}
			/* Unregister instance */
			else if(1 <= inst->Section && inst->Section <= core.interface.sectionCount) {
				if(usedInst[inst->Section - 1] == inst)
					usedInst[inst->Section - 1] = NULL;
			}
			break;
			
		case CORE_FUNCTION_EXECUTING:
			/* Check references */
			if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL) {
				/* Do not spam the logger, StCoreSystem() logs this */
				resetOutput(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_ALLOC;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			
			/* Check select changes */
			if(inst->Section != inst->Internal.Select && inst->Section != inst->Internal.PreviousSelect) {
				args.i[0] = inst->Internal.PreviousSelect;
				args.i[1] = inst->Section;
				logMessage(CORE_LOG_SEVERITY_WARNING, coreLogCode(stCORE_WARNING_SELECT), "StCoreSection select %i change to %i is ignored until re-enabled", &args);
			}
			
			inst->Valid = true;
			
			/*******
			 Control
			*******/
			pSectionControl = core.pCyclicControl + core.interface.sectionControlOffset + inst->Section - 1;
			
			/* Enable */
			if(inst->EnableSection) SET_BIT(*pSectionControl, stSECTION_ENABLE);
			else CLEAR_BIT(*pSectionControl, stSECTION_ENABLE);
			
			/* Clear faults/warnings */
			if(inst->AcknowledgeFaults) SET_BIT(*pSectionControl, stSECTION_ACKNOWLEDGE_FAULTS);
			else CLEAR_BIT(*pSectionControl, stSECTION_ACKNOWLEDGE_FAULTS);
			
			/******
			 Status
			******/
			pSectionStatus = core.pCyclicStatus + core.interface.sectionStatusOffset + inst->Section - 1;
			
			inst->Enabled = GET_BIT(*pSectionStatus, stSECTION_ENABLED);
			inst->UnrecognizedPallets = GET_BIT(*pSectionStatus, stSECTION_UNRECOGNIZED_PALLET);
			inst->MotorPowerOn = GET_BIT(*pSectionStatus, stSECTION_MOTOR_POWER_ON);
			inst->PalletsRecovering = GET_BIT(*pSectionStatus, stSECTION_PALLETS_RECOVERING);
			inst->LocatingPallets = GET_BIT(*pSectionStatus, stSECTION_LOCATING_PALLETS);
			inst->DisabledExternally = GET_BIT(*pSectionStatus, stSECTION_DISABLED_EXTERNALLY);
			inst->WarningPresent = GET_BIT(*pSectionStatus, stSECTION_WARNING);
			inst->FaultPresent = GET_BIT(*pSectionStatus, stSECTION_FAULT);
			
			/********************
			 Extended Information
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
			inst->Info.PalletCount = (unsigned char)dataUInt16;
			
			break;
			
		default:
			/* Wait for rising edge */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset) {
				resetOutput(inst); /* Clear error */
				/* Check if used instance (valid select) and go to EXECUTING otherwise DISABLED */
				if(1 <= inst->Section && inst->Section <= core.interface.sectionCount) {
					if(usedInst[inst->Section - 1] == inst)
						inst->Internal.State = CORE_FUNCTION_EXECUTING;
					else
						inst->Internal.State = CORE_FUNCTION_DISABLED;
				}
				else
					inst->Internal.State = CORE_FUNCTION_DISABLED;
			}
			break;
	}
	
	inst->Internal.PreviousSelect = inst->Section;
	inst->Internal.PreviousErrorReset = inst->ErrorReset;
	
} /* End function */

/* Clear StCoreSection outputs */
void resetOutput(StCoreSection_typ *inst) {
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
	memset(&inst->Info, 0, sizeof(inst->Info));
}

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}
