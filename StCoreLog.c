/*******************************************************************************
 * File: StCoreLog.c
 * Author: Tyler Matijevich
 * Date: 2022-02-17
*******************************************************************************/

#include "StCoreMain.h"

void coreLogMessage(UserLogSeverityEnum severity, unsigned short code, char *message) {
	CustomMessage(severity, code, message, CORE_LOGBOOK_NAME, CORE_LOGBOOK_FACILITY);
}

void coreLogFormatMessage(UserLogSeverityEnum severity, unsigned short code, char *message, FormatStringArgumentsType *args) {
	CustomFormatMessage(severity, code, message, args, CORE_LOGBOOK_NAME, CORE_LOGBOOK_FACILITY);
}

unsigned short coreEventCode(long eventID) {
	return (unsigned short)eventID;
}

/* Safely copy a string with a specified destination size */
char* coreStringCopy(char *destination, const char *source, unsigned long size) {
	/* Ensure null terminator */
	destination[size - 1] = '\0';
	/* Copy size - 1 characters */
	return strncpy(destination, source, size - 1);
}

/* Log service channel error responses for all StCore functions */
void coreLogServiceChannel(unsigned short result, unsigned short parameter) {
	
	/* Declare local variables */
	FormatStringArgumentsType args;
	
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
	
	coreLogFormatMessage(USERLOG_SEVERITY_ERROR, coreEventCode(stCORE_ERROR_SERVCHAN), "Serv. chan. error %i accessing par %i: %s", &args);
	
} /* Function defintion */
