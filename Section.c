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
	static StCoreSection_typ *usedInst[CORE_SECTION_ADDRESS_MAX + 1];
	coreFormatArgumentType args;
	unsigned char *pSectionControl;
	unsigned short *pSectionStatus;
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
				if(inst->Section < 1 || CORE_SECTION_ADDRESS_MAX < inst->Section) {
					args.i[0] = inst->Section;
					args.i[1] = CORE_SECTION_ADDRESS_MAX;
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INDEX), "StCoreSection section %i index exceeds limits [1, %]", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INDEX;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
				else if(core.sectionMap[inst->Section] < 0) {
					args.i[0] = inst->Section;
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INDEX), "StCoreSection section %i index is not defined in system layout", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INDEX;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
				
				/* Register instance */
				else if(usedInst[inst->Section] == NULL) {
					usedInst[inst->Section] = inst;
					inst->Internal.Select = inst->Section;
					inst->Internal.State = CORE_FUNCTION_EXECUTING;
				}
				/* Instance multiple */
				else {
					args.i[0] = inst->Section;
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INSTANCE), "Multiple instances of StCoreSection with section %i", &args);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INSTANCE;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
			}
			/* Unregister instance */
			else if(1 <= inst->Section && inst->Section <= CORE_SECTION_ADDRESS_MAX) {
				if(usedInst[inst->Section] == inst)
					usedInst[inst->Section] = NULL;
			}
			break;
			
		case CORE_FUNCTION_EXECUTING:
			/* Check references */
			if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL) {
				args.i[0] = inst->Internal.Select;
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOCATION), "StCoreSection section %i is unable to reference cyclic data", &args);
				resetOutput(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_ALLOCATION;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			
			/* Check select changes */
			if(inst->Section != inst->Internal.Select && inst->Section != inst->Internal.PreviousSelect) {
				args.i[0] = inst->Internal.PreviousSelect;
				args.i[1] = inst->Section;
				logMessage(CORE_LOG_SEVERITY_WARNING, coreLogCode(stCORE_WARNING_INDEX), "StCoreSection section %i index change to %i is ignored until re-enabled", &args);
				inst->StatusID = stCORE_WARNING_INDEX;
			}
			
			/*******
			 Control
			*******/
			pSectionControl = core.pCyclicControl + core.interface.sectionControlOffset + (unsigned char)core.sectionMap[inst->Internal.Select];
			
			/* Enable */
			if(inst->EnableSection) SET_BIT(*pSectionControl, stSECTION_ENABLE);
			else CLEAR_BIT(*pSectionControl, stSECTION_ENABLE);
			
			/* Clear faults/warnings */
			if(inst->AcknowledgeFaults) SET_BIT(*pSectionControl, stSECTION_ACKNOWLEDGE_FAULTS);
			else CLEAR_BIT(*pSectionControl, stSECTION_ACKNOWLEDGE_FAULTS);
			
			/******
			 Status
			******/
			pSectionStatus = (unsigned short*)(core.pCyclicStatus + core.interface.sectionStatusOffset) + core.sectionMap[inst->Internal.Select];
			
			inst->Enabled = GET_BIT(*pSectionStatus, stSECTION_ENABLED);
			inst->UnrecognizedPallets = GET_BIT(*pSectionStatus, stSECTION_UNRECOGNIZED_PALLET);
			inst->MotorPowerOn = GET_BIT(*pSectionStatus, stSECTION_MOTOR_POWER_ON);
			inst->PalletsRecovering = GET_BIT(*pSectionStatus, stSECTION_PALLETS_RECOVERING);
			inst->LocatingPallets = GET_BIT(*pSectionStatus, stSECTION_LOCATING_PALLETS);
			inst->DisabledExternally = GET_BIT(*pSectionStatus, stSECTION_DISABLED_EXTERNALLY);
			inst->WarningPresent = GET_BIT(*pSectionStatus, stSECTION_WARNING);
			inst->FaultPresent = GET_BIT(*pSectionStatus, stSECTION_FAULT);
			inst->DisabledPallet = GET_BIT(*pSectionStatus, 8);
			inst->PalletsInitializing = GET_BIT(*pSectionStatus, 9);
			
			/********************
			 Extended Information
			********************/
			SuperTrakServChanRead(inst->Internal.Select, stPAR_SECTION_FAULTS_ACTIVE, 1, 1, (unsigned long)&dataUInt32, sizeof(dataUInt32));
			for(i = 0; i < 32; i++) {
				if(GET_BIT(dataUInt32, i) && !GET_BIT(inst->Info.Warnings, i))
					coreLogFaultWarning(32 + i, inst->Internal.Select);
			}
			memcpy(&inst->Info.Warnings, &dataUInt32, sizeof(inst->Info.Warnings));
			
			SuperTrakServChanRead(inst->Internal.Select, stPAR_SECTION_FAULTS_ACTIVE, 0, 1, (unsigned long)&dataUInt32, sizeof(dataUInt32));
			for(i = 0; i < 32; i++) {
				if(GET_BIT(dataUInt32, i) && !GET_BIT(inst->Info.Faults, i))
					coreLogFaultWarning(i, inst->Internal.Select);
			}
			memcpy(&inst->Info.Faults, &dataUInt32, sizeof(inst->Info.Faults));
			
			SuperTrakServChanRead(inst->Internal.Select, stPAR_SECTION_PALLET_COUNT, 0, 1, (unsigned long)&dataUInt16, sizeof(dataUInt16));
			inst->Info.PalletCount = (unsigned char)dataUInt16;
			
			/* Allow warning reset */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset)
				inst->StatusID = 0;
			
			/* Report valid */
			inst->Valid = true;
			
			break;
			
		default:
			/* Wait for rising edge */
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset) {
				resetOutput(inst); /* Clear error */
				/* Check if used instance (valid select) and go to EXECUTING otherwise DISABLED */
				if(1 <= inst->Section && inst->Section <= CORE_SECTION_ADDRESS_MAX) {
					if(usedInst[inst->Section] == inst)
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
	inst->DisabledPallet = false;
	inst->PalletsInitializing = false;
	memset(&inst->Info, 0, sizeof(inst->Info));
}

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}
