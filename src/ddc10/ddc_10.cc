// **************************************************************
// 
// kodiaq data acquisition software
// 
// File     : ddc_10.cc
// Author   : Lukas Buetikofer, LHEP, Universitaet Bern
// Date     : 3.12.2013
// 
// Brief    : Class for comunication with DDC-10 board (HE veto)
// 
// ***************************************************************

#include <iostream>
#include "ddc_10.hh"
#include <expect.h>
#include <sys/wait.h>
#include <math.h>

ddc_10::ddc_10()
{

}

ddc_10::~ddc_10()
{

}

int ddc_10::Initialize(ddc10_par_t arg0)
{

//===== expext with telnet ===== (not used anymore)
/*
	FILE *expect = exp_popen((char *) "telnet");
	if (expect == 0) return 1;

	enum { usage, command_not_found,command_failed, prompt };

	string temp = "open ";
	temp += IPAddress;
	fprintf(expect, "%s\r", temp.c_str()); // open connection to DDC-10

	switch (exp_fexpectl(expect,
                exp_glob, "root:/>", prompt,
                exp_end)) {
        case prompt:
                cout << endl << "Connection successful" << endl;
                // continue
                break;
        case EXP_TIMEOUT:
                cout << "Timeout,  may be invalid host" << endl;
                return 1;
    }
*/


//====== establish ssh connection to DDC-10 =====
        string temp = "ssh root@";
        temp += arg0.IPAddress;

        // exp_open spawn ssh and returns a stream.
        FILE *expect = exp_popen((char *) temp.c_str());
        if (expect == 0) return 1;

        enum { usage, permission_denied, command_not_found,
               command_failed, prompt };

        // exp_fexpectl takes variable parameters terminated by
        // exp_end.  A parameter takes the form
        // {type, pattern, and return value}  corresponding to
        // exp_glob, "root:/>", prompt,  in exp_fexpectl call below.
        // If a pattern matches, exp_fexpectl returns with the value
        // corresponding to the pattern. 

        switch (exp_fexpectl(expect,
                exp_glob, "password:", prompt,
                exp_end)) {
        case prompt:
                // continue
                break;
        case EXP_TIMEOUT:
                cout << "Timeout,  may be invalid host" << endl;
                return 1;
        }

        fprintf(expect, "uClinux\r"); // Password for ssh connection to DDC-10
        switch (exp_fexpectl(expect,
                exp_glob, "root:~>", prompt,
                exp_glob, "Permission denied", permission_denied,
                exp_end)) {
        case prompt:
                cout << endl << "Connection successful" << endl;
                // continue
                break;
        case permission_denied:
                cout << endl << "Permission denied" << endl;
                return 1;
        case EXP_TIMEOUT:
                cout << "Timeout,  may be invalid host" << endl;
                return 1;
        }


//========= send commands ==========
	bool success = true;


	//convert double par into int variables
	long Par_long[4];
	int ParLow_int[4]; //int type needed for blackfin
	int ParHi_int[4];  //int type needed for blackfin
	for(int i=0; i<4; i++) {
		Par_long[i]=round(arg0.Par[i] * pow(2,48));
		ParLow_int[i]=Par_long[i] & 0x00000000FFFFFFFF;
		ParHi_int[i]=(Par_long[i] >> 32);
	}

	// send Initialization command
	char command [1000];

	sprintf(command, "./../usr/bin/Initialize ");
	sprintf(command + strlen(command), "%d ",arg0.Sign);
        sprintf(command + strlen(command), "%d ",arg0.IntWindow);
        sprintf(command + strlen(command), "%d ",arg0.VetoDelay);
        sprintf(command + strlen(command), "%d ",arg0.SigThreshold);
        sprintf(command + strlen(command), "%d ",arg0.IntThreshold);
        sprintf(command + strlen(command), "%d ",arg0.WidthCut);
        sprintf(command + strlen(command), "%d ",arg0.RiseTimeCut);
        sprintf(command + strlen(command), "%d ",arg0.ComponentStatus);
        sprintf(command + strlen(command), "%d ",ParHi_int[0]);  //par1 hi
        sprintf(command + strlen(command), "%d ",ParLow_int[0]); //par1 low
        sprintf(command + strlen(command), "%d ",ParHi_int[1]);  //par2 hi
        sprintf(command + strlen(command), "%d ",ParLow_int[1]); //par2 ...
        sprintf(command + strlen(command), "%d ",ParHi_int[2]);
        sprintf(command + strlen(command), "%d ",ParLow_int[2]);
        sprintf(command + strlen(command), "%d ",ParHi_int[3]);
        sprintf(command + strlen(command), "%d ",ParLow_int[3]);
        sprintf(command + strlen(command), "%d ",arg0.OuterRingFactor);
        sprintf(command + strlen(command), "%d ",arg0.InnerRingFactor);
        sprintf(command + strlen(command), "%d ",arg0.PreScaling);
	fprintf(expect, "%s\r", command);


	// check return of telnet
	switch (exp_fexpectl(expect,
                exp_glob, "not found", command_not_found, // 1 case
                exp_glob, "wrong usage", usage, // another case
                exp_glob, "initialization done", prompt, // third case
                exp_end)) {
        case command_not_found:
                cout << endl << "unknown command" << endl;
                success = false;
                break;
        case usage:
                success = false;
                cout << endl << "wrong usage of \"Initialize\". 8 arguments are needed" << endl;
                break;
        case EXP_TIMEOUT:
		success = false;
                cout << "Login timeout" << endl;
                break;
        case prompt:
                // continue;
                cout << endl << "initialization successful" << endl;
                break;
        default:
		success = false;
		cout << endl << "unknown error" << endl;
		break;
	}

//======= close ssh connection ===========
	fclose(expect);
	waitpid(exp_pid, 0, 0);// wait for expect to terminate

	if (success) return 0;
	else return 1;
}



int ddc_10::LEDTestFlash(string IPAddress)
{


//====== establish ssh connection to DDC-10 =====
	string temp = "ssh root@";
	temp += IPAddress;

	// exp_open spawn telnet and returns a stream.
        FILE *expect = exp_popen((char *) temp.c_str());
	if (expect == 0) return 1;

	enum { usage, permission_denied, command_not_found,
	       command_failed, prompt };

	// exp_fexpectl takes variable parameters terminated by
	// exp_end.  A parameter takes the form
	// {type, pattern, and return value}  corresponding to
	// exp_glob, "root:/>", prompt,  in exp_fexpectl call below.
	// If a pattern matches, exp_fexpectl returns with the value
	// corresponding to the pattern. 

	switch (exp_fexpectl(expect,
                exp_glob, "password:", prompt,
                exp_end)) {
        case prompt:
                // continue
                break;
        case EXP_TIMEOUT:
                cout << "Timeout,  may be invalid host" << endl;
                return 1;
	}

	fprintf(expect, "uClinux\r"); // Password for ssh connection to DDC-10
	switch (exp_fexpectl(expect,
                exp_glob, "root:~>", prompt,
		exp_glob, "Permission denied", permission_denied,
                exp_end)) {
        case prompt:
                cout << endl << "Connection successful" << endl;
                // continue
                break;
	case permission_denied:
		cout << endl << "Permission denied" << endl;
		return 1;
        case EXP_TIMEOUT:
                cout << "Timeout,  may be invalid host" << endl;
                return 1;
	}

//========= send commands ==========
	bool success = true;

	// send LED flash  command
	char command [1000];
	sprintf(command, "./../usr/bin/LEDTestFlash");
	fprintf(expect, "%s\r", command);

	// check return of telnet
	switch (exp_fexpectl(expect,
                exp_glob, "not found", command_not_found, // 1 case
                exp_glob, "wrong usage", usage, // another case
                exp_glob, "LEDTesetFlash done", prompt, // third case
                exp_end)) {
        case command_not_found:
                cout << endl << "unknown command" << endl;
                success = false;
                break;
        case usage:
                success = false;
                cout << endl << "wrong usage of \"Initialize\". 8 arguments are needed" << endl;
                break;
        case EXP_TIMEOUT:
                success = false;
                cout << "Login timeout" << endl;
                break;
        case prompt:
                // continue;
                cout << endl << "LEDTestFlash successful" << endl;
                break;
        default:
                success = false;
                cout << endl << "unknown error" << endl;
                break;
	}

//======= close ssh connection ======
	fclose(expect);

	waitpid(exp_pid, 0, 0);// wait for expect to terminate

	if (success) return 0;
    	else return 1;

}

