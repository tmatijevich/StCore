/*******************************************************************************
 * File: StCoreLog.c
 * Author: Tyler Matijevich
 * Date: 2022-02-17
*******************************************************************************/

#include "StCoreMain.h"
#define FORMAT_SIZE 125

void StCoreLogMessage(UserLogSeverityEnum severity, unsigned short code, char *message) {
	CustomMessage(severity, code, message, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
}

void StCoreFormatMessage(UserLogSeverityEnum severity, unsigned short code, char *message, FormatStringArgumentsType *args) {
	CustomFormatMessage(severity, code, message, args, stCORE_LOGBOOK_NAME, stCORE_LOGBOOK_FACILITY);
}

unsigned short StCoreEventCode(long eventID) {
	return (unsigned short)eventID;
}

long StCoreLogServChan(unsigned short result, unsigned short parameter) {
	
	char format[FORMAT_SIZE];
	struct FormatStringArgumentsType args;
	
	if(result == scERR_SUCCESS)
		return 0;
		
	format[FORMAT_SIZE - 1] = '\0';
	strncpy(format, "Serv. chan. error accessing par %i: ", FORMAT_SIZE - 1);
	args.i[0] = parameter;
	
	switch(result) {
		case scERR_INVALID_SECTION:	
			strncat(format, "An invalid section number was specified", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_PARAM:
			strncat(format, "The specified parameter number was not recognized", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_TASK:
			strncat(format, "The specified task code was not recognized", FORMAT_SIZE - 1);
			break;
		case scERR_TASK_UNAVAILABLE:
			strncat(format, "The requested operation is currently unavailable", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_INDEX:
			strncat(format, "The specified starting index is invalid", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_VALUE:
			strncat(format, "The value to be written was outside of the valid range", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_COUNT:
			strncat(format, "The requested number of elements is invalid", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_ARGUMENT:
			strncat(format, "An invalid argument was specified", FORMAT_SIZE - 1);
			break;
		case scERR_COMMAND_TIMEOUT:
			strncat(format, "The command did not complete in a timely manner", FORMAT_SIZE - 1);
			break;
		case scERR_UNAUTHORIZED:
			strncat(format, "The request was denied due to a lack of permissions", FORMAT_SIZE - 1);
			break;
		case scERR_BUFFER_SIZE:
			strncat(format, "The buffer size is insufficient", FORMAT_SIZE - 1);
			break;
		case scERR_INVALID_PACKET:
			strncat(format, "The packet is malformed, orits length is incorrect", FORMAT_SIZE - 1);
			break;
		case scERR_INTERNAL_ERROR:
			strncat(format, "An internal error occurred while processing the request", FORMAT_SIZE - 1);
			break;
		default:
			strncat(format, "Unknown error", FORMAT_SIZE - 1);
			break;
	}
	
	CustomFormatMessage(USERLOG_SEVERITY_ERROR, 50100, format, &args, "StCoreLog", 1);
	
	return ArEventLogMakeEventID(arEVENTLOG_SEVERITY_ERROR, 1, 50100);
	
} /* Function defintion */
