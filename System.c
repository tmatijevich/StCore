/*******************************************************************************
 * File: StCore\System.c
 * Author: Tyler Matijevich
 * Date: 2022-02-26
*******************************************************************************/

#include "Main.h"
#define LOG_OBJECT "System"

/* Prototypes */
static void resetOutput(StCoreSystem_typ *inst);
static long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args);

/* System core interface */
void StCoreSystem(StCoreSystem_typ *inst) {
	
	/************************************************
	 Dependencies:
	  Global:
	   core.pCyclicControl (w)
	   core.pCyclicStatus
	   core.interface
	   core.error
	   core.statusID
	   core.debug (w)
	  Subroutines:
	   resetOutput
	   logMessage
	************************************************/
	
	/***********************
	 Declare Local Variables
	***********************/
	static StCoreSystem_typ *usedInst;
	unsigned short *pSystemControl, *pSystemStatus;
	long i;
	unsigned short *pSectionStatus;
	unsigned long systemPower;
	static RTInfo_typ fbRTInfo;
	static unsigned long timer;
	
	/**********
	 Cycle Time
	**********/
	fbRTInfo.enable = true;
	RTInfo(&fbRTInfo);
		
	/************
	 Switch State
	************/
	/* Interrupt if disabled */
	if(!inst->Enable)
		inst->Internal.State = CORE_FUNCTION_DISABLED;
		
	switch(inst->Internal.State) {
		case CORE_FUNCTION_DISABLED:
			resetOutput(inst);
			if(inst->Enable) {
				/* Register isntance */
				if(usedInst == NULL) {
					/* Do not proceed until StCoreCyclic had saved interface parameters */
					if(core.ready) {
						usedInst = inst;
						inst->Internal.State = CORE_FUNCTION_EXECUTING;
					}
					/* Monitor for timeout in case StCoreCyclic is not called */
					else if(timer >= 2 * CORE_CONFIGURATION_TIMEOUT) {
						logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_CYCLE), "StCoreSystem cannot enabled due to timeout from communication with StCoreCyclic", NULL);
						inst->Error = true;
						inst->StatusID = stCORE_ERROR_CYCLE;
						inst->Internal.State = CORE_FUNCTION_ERROR;
					}
					else {
						/* Track time when enabled */
						if(fbRTInfo.status == ERR_OK) timer += fbRTInfo.cycle_time;
						else timer += CORE_CYCLE_TIME; /* Fallback increment by cycle time if RTInfo is not successful */
					}
				}
				else {
					logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_INSTANCE), "Multiple instances of StCoreSystem", NULL);
					inst->Error = true;
					inst->StatusID = stCORE_ERROR_INSTANCE;
					inst->Internal.State = CORE_FUNCTION_ERROR;
				}
			}
			/* Unregister instance */
			else if(usedInst == inst) {
				usedInst = NULL;
			}
			/* Manage debug state */
			core.debug = false;
			break;
			
		case CORE_FUNCTION_EXECUTING:
			/* Check core error */
			if(core.error) {
				resetOutput(inst);
				inst->Error = true;
				inst->StatusID = core.statusID;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			/* Check references */
			if(core.pCyclicControl == NULL || core.pCyclicStatus == NULL) {
				logMessage(CORE_LOG_SEVERITY_ERROR, coreLogCode(stCORE_ERROR_ALLOCATION), "StCoreSystem cannot reference cyclic data", NULL);
				resetOutput(inst);
				inst->Error = true;
				inst->StatusID = stCORE_ERROR_ALLOCATION;
				inst->Internal.State = CORE_FUNCTION_ERROR;
				break;
			}
			/* Report valid */
			inst->Valid = true;
			
			/*******
			 Control
			*******/
			pSystemControl = (unsigned short*)(core.pCyclicControl + core.interface.systemControlOffset);
			
			/* Enable */
			if(inst->EnableAllSections) SET_BIT(*pSystemControl, stSYSTEM_ENABLE_ALL_SECTIONS);
			else CLEAR_BIT(*pSystemControl, stSYSTEM_ENABLE_ALL_SECTIONS);
			
			/* Clear faults/warnings */
			if(inst->AcknowledgeFaults) SET_BIT(*pSystemControl, stSYSTEM_ACKNOWLEDGE_FAULTS);
			else CLEAR_BIT(*pSystemControl, stSYSTEM_ACKNOWLEDGE_FAULTS);
			
			/******
			 Status
			******/
			pSystemStatus = (unsigned short*)(core.pCyclicStatus + core.interface.systemStatusOffset);
			
			inst->PalletsStopped = GET_BIT(*pSystemStatus, stSYSTEM_PALLETS_STOPPED);
			inst->WarningPresent = GET_BIT(*pSystemStatus, stSYSTEM_WARNING);
			inst->FaultPresent = GET_BIT(*pSystemStatus, stSYSTEM_FAULT);
			inst->PalletCount = (unsigned char)(*(pSystemStatus + 1)); /* Access the next 16 bits */
			
			/* Extended information */
			SuperTrakServChanRead(0, stPAR_SYSTEM_FAULTS_ACTIVE, 1, 1, (unsigned long)&inst->Info.Warnings, sizeof(inst->Info.Warnings));
			SuperTrakServChanRead(0, stPAR_SYSTEM_FAULTS_ACTIVE, 0, 1, (unsigned long)&inst->Info.Faults, sizeof(inst->Info.Faults));
			
			SuperTrakServChanRead(0, stPAR_SYSTEM_LOAD_POWER, 0, 1, (unsigned long)&systemPower, sizeof(systemPower));
			inst->Info.LoadPower = (float)systemPower;
			SuperTrakServChanRead(0, stPAR_SYSTEM_PEAK_POWER, 0, 1, (unsigned long)&systemPower, sizeof(systemPower));
			inst->Info.PeakPower = (float)systemPower;
			SuperTrakServChanRead(0, stPAR_SYSTEM_AVERAGE_POWER, 0, 1, (unsigned long)&systemPower, sizeof(systemPower));
			inst->Info.AveragePower = (float)systemPower;
			
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
				pSectionStatus = (unsigned short*)(core.pCyclicStatus + core.interface.sectionStatusOffset) + i;
				
				if(GET_BIT(*pSectionStatus, stSECTION_ENABLED)) inst->Info.Disabled = false;
				else inst->Info.Enabled = false;
				if(!GET_BIT(*pSectionStatus, stSECTION_MOTOR_POWER_ON)) inst->Info.MotorPower = false;
				if(GET_BIT(*pSectionStatus, stSECTION_DISABLED_EXTERNALLY)) inst->Info.DisabledExternally = true;
				if(GET_BIT(*pSectionStatus, stSECTION_WARNING)) inst->Info.SectionWarningPresent = true;
				if(GET_BIT(*pSectionStatus, stSECTION_FAULT)) inst->Info.SectionFaultPresent = true;
			}
			
			/* Manage debug state */
			core.debug = inst->Debug;
			
			break;
			
		default:
			if(inst->ErrorReset && !inst->Internal.PreviousErrorReset) {
				resetOutput(inst);
				if(inst == usedInst)
					inst->Internal.State = CORE_FUNCTION_EXECUTING;
				else
					inst->Internal.State = CORE_FUNCTION_DISABLED;
			}
			
			break;
	}
	
	inst->Internal.PreviousErrorReset = inst->ErrorReset;
	
} /* End function */

/* Clear instance outputs */
void resetOutput(StCoreSystem_typ *inst) {
	inst->Valid = false;
	inst->Error = false;
	inst->StatusID = 0;
	inst->PalletsStopped = false;
	inst->WarningPresent = false;
	inst->FaultPresent = false;
	inst->PalletCount = 0;
	memset(&inst->Info, 0, sizeof(inst->Info));
}

/* Create local logging function */
long logMessage(coreLogSeverityEnum severity, unsigned short code, char *message, coreFormatArgumentType *args) {
	return coreLog(core.ident, severity, CORE_LOGBOOK_FACILITY, code, LOG_OBJECT, message, args);
}
