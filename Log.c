/*******************************************************************************
 * File: StCore\Log.c
 * Author: Tyler Matijevich
 * Date: 2022-02-17
*******************************************************************************/

#include "Main.h"

void coreLogMessage(UserLogSeverityEnum severity, unsigned short code, char *message) {
	CustomMessage(severity, code, message, CORE_LOGBOOK_NAME, CORE_LOGBOOK_FACILITY);
}

void coreLogFormat(UserLogSeverityEnum severity, unsigned short code, char *message, FormatStringArgumentsType *args) {
	CustomFormatMessage(severity, code, message, args, CORE_LOGBOOK_NAME, CORE_LOGBOOK_FACILITY);
}

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
	unsigned char severityMap[] = {
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
	
	coreLogFormat(USERLOG_SEVERITY_CRITICAL, coreLogCode(stCORE_ERROR_SERVCHAN), "Serv. chan. error %i accessing par %i: %s", &args);
	
} /* Function defintion */

/* Log SuperTrak faults and warnings */
void coreLogFaultWarning(unsigned char index, unsigned char section) {
	
	/* Local variables */
	FormatStringArgumentsType args;
	char temp[80];
	UserLogSeverityEnum severity;
	
	/* Out of range */
	if(index > 63)
		return;
	
	/* Context */
	if(section == 0)
		coreStringCopy(args.s[0], "System", sizeof(args.s[0]));
	else {
		args.i[0] = section;
		IecFormatString(temp, sizeof(temp), "Section %i", &args);
		coreStringCopy(args.s[0], temp, sizeof(args.s[0]));
	}
		
	/* Fault or warning */
	if(index < 32) {
		severity = USERLOG_SEVERITY_ERROR;
		coreStringCopy(args.s[1], "fault", sizeof(args.s[1]));
	}
	else {
		severity = USERLOG_SEVERITY_WARNING;
		coreStringCopy(args.s[1], "warning", sizeof(args.s[1]));
	}
	
	/* Write */
	CustomFormatMessage(severity, index % 32, "%s %s", &args, CORE_LOGBOOK_NAME, 2);
	
} /* Function definition */
