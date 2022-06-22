/*******************************************************************************
 * File: StCore\Log.c
 * Author: Tyler Matijevich
 * Date: 2022-02-17
*******************************************************************************/

#include "Main.h"

/* Safely copy a string */
char* coreStringCopy(char *destination, const char *source, unsigned long size) {
	destination[size - 1] = '\0'; /* Ensure null terminator */
	return strncpy(destination, source, size - 1); /* Copy up to size-1 characters */
}

/* Format a string with specifiers and arguments */
unsigned long coreFormat(char *str, unsigned long size, char *format, coreFormatArgumentType *args) {
	
	/* Local variables */
	const char sBool[][6] = {"FALSE", "TRUE"}; 	/* Boolean arguments = 5 + null terminator */
	char sNum[13]; 								/* Floats: [<+->]1.23456[e<+->12] = 12 + null terminator, Longs: -2147483648 to 2147483647 = 11 + null terminator */
	unsigned char countBools = 0;
	unsigned char countFloats = 0;
	unsigned char countLongs = 0;
	unsigned char countStrings = 0; 
	unsigned long length, bytesLeft = size - 1; /* Guard output, leave room for null terminator */
	
	/* Guard null pointers */
	if(str == NULL || format == NULL || args == NULL) 
		return 0;
	
	while(*format != '\0' && bytesLeft > 0) {
		if(*format != '%') {
			*str++ = *format++; /* Direct copy */
			bytesLeft--;
			continue;
		}
		
		*str = '\0'; /* Temporarily add null terminator to perform concatination */
		length = 0; /* Set the length to zero if the format specifier is invalid */
		
		switch(*(++format)) {
			case 'b':
				if(countBools <= CORE_FORMAT_ARGUMENT_COUNT) 
					length = strlen(strncat(str, sBool[args->b[countBools++]], bytesLeft));
				break;
			
			 case 'r':
			 	if(countFloats <= CORE_FORMAT_ARGUMENT_COUNT) {
					brsftoa((float)(args->f[countFloats++]), (unsigned long)sNum);
					length = strlen(strncat(str, sNum, bytesLeft));
			 	}
			 	break;
			 
			 case 'i':
			 	if(countLongs <= CORE_FORMAT_ARGUMENT_COUNT) {
					brsitoa(args->i[countLongs++], (unsigned long)sNum);
					length = strlen(strncat(str, sNum, bytesLeft));
			 	}
			 	break;
			 
			 case 's':
			 	if(countStrings <= CORE_FORMAT_ARGUMENT_COUNT) 
					length = strlen(strncat(str, args->s[countStrings++], bytesLeft));
			 	break;
			 
			 case '%':
			 	*str = '%';
				length = 1;
				break;
		} /* End switch */
		
		str += length;
		bytesLeft -= length;
		format++;
		
	} /* End while */
	
	*str = '\0'; /* Add the null terminator to end the string */
	return size - bytesLeft - 1;
	
} /* End function */

/* Log messages to the library's logbook with ArEventLog */
long coreLog(ArEventLogIdentType ident, coreLogSeverityEnum severity, unsigned char facility, unsigned short code, char *object, char *message, coreFormatArgumentType *args) {

	/* Declare local variables */
	ArEventLogWrite_typ fbWrite;
	char asciiMessage[CORE_FORMAT_SIZE + 1];
	const unsigned char severityMap[] = {
		arEVENTLOG_SEVERITY_ERROR, /* CORE_LOG_SEVERITY_ERROR */
		arEVENTLOG_SEVERITY_WARNING, /* CORE_LOG_SEVERITY_WARNING */
		arEVENTLOG_SEVERITY_INFO, /* CORE_LOG_SEVERITY_INFO */
		arEVENTLOG_SEVERITY_SUCCESS, /* CORE_LOG_SEVERITY_SUCCESS */
		arEVENTLOG_SEVERITY_INFO, /* CORE_LOG_SEVERITY_DEBUG */
	};
	long status;
	
	/* Gaurd null pointers */
	if(ident == 0 || message == NULL)
		return -1;
		
	/* Suppress debug messages */
	if(!core.debug && severity >= CORE_LOG_SEVERITY_DEBUG)
		return -1;
	
	/* Write message */
	memset(&fbWrite, 0, sizeof(fbWrite));
	fbWrite.Ident = ident;
	fbWrite.EventID = ArEventLogMakeEventID(severityMap[severity], facility, code);
	if(args == NULL)
		coreStringCopy(asciiMessage, message, sizeof(asciiMessage));
	else
		coreFormat(asciiMessage, sizeof(asciiMessage), message, args);
	fbWrite.AddDataFormat = arEVENTLOG_ADDFORMAT_TEXT;
	fbWrite.AddDataSize = strlen(asciiMessage) + 1;
	fbWrite.AddData = (unsigned long)asciiMessage;
	coreStringCopy(fbWrite.ObjectID, object, sizeof(fbWrite.ObjectID));
	fbWrite.Execute = true;
	ArEventLogWrite(&fbWrite);
	
	status = fbWrite.StatusID;
	
	fbWrite.Execute = false;
	ArEventLogWrite(&fbWrite);
	
	return status;

} /* End function */

/* Read 16 bit code from 32 bit event */
unsigned short coreLogCode(long event) {
	return (unsigned short)event;
}

/* Log service channel error responses for all StCore functions */
void coreLogServiceChannel(unsigned short result, unsigned short parameter, char *object) {
	
	/* Declare local variables */
	coreFormatArgumentType args;
	
	/* Check result */
	if(result == scERR_SUCCESS)
		return;
	
	/* Arguments */
	args.i[0] = result;
	args.i[1] = parameter;
	
	/**********************************
	 Concatenate specific error message
	**********************************/
	switch(result) {
		case scERR_INVALID_SECTION:	
			coreStringCopy(args.s[0], "An invalid section number was specified", sizeof(args.s[0]));
			break;
		case scERR_INVALID_PARAM:
			coreStringCopy(args.s[0], "The specified parameter number was not recognized", sizeof(args.s[0]));
			break;
		case scERR_INVALID_TASK:
			coreStringCopy(args.s[0], "The specified task code was not recognized", sizeof(args.s[0]));
			break;
		case scERR_TASK_UNAVAILABLE:
			coreStringCopy(args.s[0], "The requested operation is currently unavailable", sizeof(args.s[0]));
			break;
		case scERR_INVALID_INDEX:
			coreStringCopy(args.s[0], "The specified starting index is invalid", sizeof(args.s[0]));
			break;
		case scERR_INVALID_VALUE:
			coreStringCopy(args.s[0], "The value to be written was outside of the valid range", sizeof(args.s[0]));
			break;
		case scERR_INVALID_COUNT:
			coreStringCopy(args.s[0], "The requested number of elements is invalid", sizeof(args.s[0]));
			break;
		case scERR_INVALID_ARGUMENT:
			coreStringCopy(args.s[0], "An invalid argument was specified", sizeof(args.s[0]));
			break;
		case scERR_COMMAND_TIMEOUT:
			coreStringCopy(args.s[0], "The command did not complete in a timely manner", sizeof(args.s[0]));
			break;
		case scERR_UNAUTHORIZED:
			coreStringCopy(args.s[0], "The request was denied due to a lack of permissions", sizeof(args.s[0]));
			break;
		case scERR_BUFFER_SIZE:
			coreStringCopy(args.s[0], "The buffer size is insufficient", sizeof(args.s[0]));
			break;
		case scERR_INVALID_PACKET:
			coreStringCopy(args.s[0], "The packet is malformed, orits length is incorrect", sizeof(args.s[0]));
			break;
		case scERR_INTERNAL_ERROR:
			coreStringCopy(args.s[0], "An internal error occurred while processing the request", sizeof(args.s[0]));
			break;
		default:
			coreStringCopy(args.s[0], "Unknown error", sizeof(args.s[0]));			
			break;
	}
	
	coreLog(core.ident, CORE_LOG_SEVERITY_ERROR, CORE_LOGBOOK_FACILITY, coreLogCode(stCORE_ERROR_PARAMETER), object, "Serv. chan. error %i accessing par %i: %s", &args);
	
} /* End function */

void logSuperTrakFault(unsigned char section, unsigned char instance) {
	
	/***********************
	 Declare Local Variables
	***********************/
	coreFormatArgumentType args;
	char temp[80];
	coreLogSeverityEnum severity;
	unsigned short fault;
	long i, status, detail[CORE_FAULT_DETAIL_MAX];
	
	/**************************
	 Read Fault Code and Detail
	**************************/
	status = SuperTrakServChanRead(section, 
		(unsigned short)(section == 0) * 1465 + (unsigned short)(section > 0) * 1485,
		instance, 1, (unsigned long)&fault, sizeof(fault));
	if(status) return;
	
	for(i = 0; i < CORE_FAULT_DETAIL_MAX; i++) {
		status = SuperTrakServChanRead(section, 
			(unsigned short)(section == 0) * 1474 + (unsigned short)(section > 0) * 1494 + i,
			instance, 1, (unsigned long)&detail[i], sizeof(detail[i]));
		if(status) return;
		args.i[i + 1] = detail[i];
	}
	
	/**************
	 Detail Message
	**************/
	/* Give context to the detail integers depending on the fault or warning */
	switch(fault) {
		case 0: /* Motor supply voltage out of range */
			coreStringCopy(args.s[2], "N/A", sizeof(args.s[2]));
			coreStringCopy(args.s[3], "Left motor voltage", sizeof(args.s[3]));
			coreStringCopy(args.s[4], "Right motor voltage", sizeof(args.s[4]));
			coreStringCopy(args.s[5], "N/A", sizeof(args.s[5]));
			break;
		case 15: /* Excessive pallet following error */
			coreStringCopy(args.s[2], "Pallet ID", sizeof(args.s[2]));
			coreStringCopy(args.s[3], "Position setpoint", sizeof(args.s[3]));
			coreStringCopy(args.s[4], "Actual position", sizeof(args.s[4]));
			coreStringCopy(args.s[5], "N/A", sizeof(args.s[5]));
			break;
		case 26: /* Disabled pallet preset */
			coreStringCopy(args.s[2], "Pallet ID", sizeof(args.s[2]));
			coreStringCopy(args.s[3], "Position", sizeof(args.s[3]));
			coreStringCopy(args.s[4], "N/A", sizeof(args.s[4]));
			coreStringCopy(args.s[5], "N/A", sizeof(args.s[5]));
			break;
		default:
			coreStringCopy(args.s[2], "Detail 0", sizeof(args.s[2]));
			coreStringCopy(args.s[3], "Detail 1", sizeof(args.s[3]));
			coreStringCopy(args.s[4], "Detail 2", sizeof(args.s[4]));
			coreStringCopy(args.s[5], "Detail 3", sizeof(args.s[5]));
			break;
	}
	
	/*************
	 Fault Message
	*************/
	if(section == 0)
		coreStringCopy(args.s[0], "System", sizeof(args.s[0]));
	else {
		args.i[0] = section;
		coreFormat(temp, sizeof(temp), "Section %i", &args);
		coreStringCopy(args.s[0], temp, sizeof(args.s[0]));
	}
	
	/* Fault or warning */
	if(fault < CORE_FAULT_MAX) {
		severity = CORE_LOG_SEVERITY_ERROR;
		coreStringCopy(args.s[1], "fault", sizeof(args.s[1]));
	}
	else {
		severity = CORE_LOG_SEVERITY_WARNING;
		coreStringCopy(args.s[1], "warning", sizeof(args.s[1]));
	}
	
	args.i[0] = fault % CORE_FAULT_MAX;
	coreLog(core.ident, severity, 2, fault % CORE_FAULT_MAX, "SuperTrak", "%s %s %i (%s: %i, %s: %i, %s: %i, %s: %i)", &args);
	
}

/* Monitor SuperTrak system and section faults and warnings */
void coreMonitorSuperTrakFault(void) {
	
	/***********************
	 Declare Local Variables
	***********************/
	static unsigned short systemInstance[CORE_FAULT_INSTANCE_MAX];
	static unsigned short sectionInstance[CORE_SECTION_MAX][CORE_FAULT_INSTANCE_MAX];
	unsigned short instance[CORE_FAULT_INSTANCE_MAX];
	long i, j, status;
	
	/******
	 System
	******/
	status = SuperTrakServChanRead(0, 1465, 0, CORE_FAULT_INSTANCE_MAX, (unsigned long)&instance, sizeof(instance));
	if(status) return;
	for(i = 0; i < CORE_FAULT_INSTANCE_MAX; i++) {
		/* Skip if 65535 (unused) */
		if(instance[i] == USHRT_MAX) continue;
		
		/* Log when a new instance is recorded */
		if(instance[i] != systemInstance[i])
			logSuperTrakFault(0, i);
	}
	memcpy(systemInstance, instance, sizeof(systemInstance));
	
	/*******
	 Section
	*******/
	for(j = 1; j < CORE_SECTION_ADDRESS_MAX; j++) {
		/* Skip undefined sections */
		if(core.sectionMap[j] == -1) continue;
		
		/* Read the fault instances */
		status = SuperTrakServChanRead(j, 1485, 0, CORE_FAULT_INSTANCE_MAX, (unsigned long)&instance, sizeof(instance));
		if(status) return;
		
		for(i = 0; i < CORE_FAULT_INSTANCE_MAX; i++) {
			/* Skip if 65535 (unused) */
			if(instance[i] == USHRT_MAX) continue;
			
			/* Log when a new instance is recorded */
			if(instance[i] != sectionInstance[core.sectionMap[j]][i])
				logSuperTrakFault(j, i);
		}
		memcpy(sectionInstance[core.sectionMap[j]], instance, sizeof(sectionInstance[core.sectionMap[j]]));
	}
	
} /* End function */
