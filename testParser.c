/* 
   This code is based on the GnuCOBOL Debugger extension available at: 
   https://github.com/OlegKunitsyn/gnucobol-debug
   It is provided without any warranty, express or implied. 
   You may modify and distribute it at your own risk.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cobgdb.h"

extern ST_Line * LineDebug;
extern struct program_file program_file;

int testParser()
{
    char buffer[500]; /* PATH_MAX incudes the \0 so +1 is not required */
	{
		char *cFile = realpath("./resources/hello.c", buffer);
		char cobol[] = "/home/developer/projects/gnucobol-debug/test/resources/hello.cbl";
		char c[512];
		char version[50];
		normalizePath(cobol);
		normalizePath(cFile);
		char fileGroup[4][512];
		strcpy(fileGroup[0],cFile);
		strcpy(fileGroup[1], "");
		SourceMap(fileGroup);
		//char *path = realpath('../../../test/resources', buffer);
		printf("Testing-> Parser.c\n");
		printf("------------------------------------------------\n");
		printf("File: %s\n", cFile);
		printf("Test: 'Total parser lines'\n");
		int x = getLinesCount(); assert(x==3); 
		printf("Test: 'Total parser variables'\n");
		x = getVariablesCount(); assert(x==3); 
		printf("Test: 'Cobol variable value check -> hello_.MYVAR=b_6'\n");
		ST_DebuggerVariable * search = getVariableByCobol("hello_.MYVAR");
		assert(search!=NULL && strcmp("b_6", search->cName)==0);
		printf("Test: 'Cobol variable value check ->hello_.MYVAR.MYVAR'\n");
		search = getVariableByCobol("hello_.MYVAR.MYVAR");
		assert(search!=NULL && strcmp("f_6", search->cName)==0);
		printf("Test: 'C variable value check ->hello_.b_6'\n");
		search = getVariableByC("hello_.b_6");
		assert(search!=NULL && strcmp("MYVAR", search->cobolName)==0);
		printf("Test: 'C variable value check ->hello_.b_6'\n");
		search = getVariableByC("hello_.f_6");
		assert(search!=NULL && strcmp("MYVAR", search->cobolName)==0);
		printf("Test: 'Get C line by cobol file name and line number-> line 8/105'\n");
		ST_Line * line = getLineC(cobol, 8);
		assert(line!=NULL && line->lineC==105);
		printf("Test: 'Get C File by cobol file name and line number-> line 8'\n");
		assert(line!=NULL && line->fileC!=NULL);
		printf("File: %s", line->fileC);
		strcpy(c, line->fileC);
		printf("Test: 'Get Cobol line by C file name and line number-> 105/8'\n");
		line = getLineCobol(c, 105);
		assert(line!=NULL && line->lineCobol==8);
		printf("Test: 'Get Cobol File by C file name and line number-> line 105'\n");
		assert(line!=NULL && line->fileCobol!=NULL);
		printf("       File: %s\n", line->fileCobol);
		printf("Test: 'Check the cobol compiler version-> 2.2.0'\n");
		getVersion(version);
		assert(strcmp(version,"2.2.0")==0);
	}
	//
	{
		printf("------------------------------------------------\n");
		char * cFile = realpath("./resources/hello3.c", buffer);
		char c[512];
		char cobol[] = "/home/developer/projects/gnucobol-debug/test/resources/hello3.cbl";
		char version[50];
		normalizePath(cobol);
		normalizePath(cFile);
		char fileGroup[4][512];
		strcpy(fileGroup[0],cFile);
		strcpy(fileGroup[1], "");
		SourceMap(fileGroup);
		//char *path = realpath('../../../test/resources', buffer);
		printf("Testing-> Parser.c\n");
		printf("------------------------------------------------\n");
		printf("File: %s\n", cFile);
		printf("Test: 'Total parser lines'\n");
		int x = getLinesCount(); assert(x==3); 
		printf("Test: 'Total parser variables'\n");
		x = getVariablesCount(); assert(x==3); 
		printf("Test: 'Cobol variable value check -> hello_.MYVAR=b_6'\n");
		ST_DebuggerVariable * search = getVariableByCobol("hello3_.MYVAR");
		assert(search!=NULL && strcmp("b_8", search->cName)==0);
		printf("Test: 'Cobol variable value check ->hello3_.MYVAR.MYVAR'\n");
		search = getVariableByCobol("hello3_.MYVAR.MYVAR");
		assert(search!=NULL && strcmp("f_8", search->cName)==0);
		printf("Test: 'C variable value check ->hello_.b_8'\n");
		search = getVariableByC("hello3_.b_8");
		assert(search!=NULL && strcmp("MYVAR", search->cobolName)==0);
		printf("Test: 'C variable value check ->hello3_.b_8'\n");
		search = getVariableByC("hello3_.f_8");
		assert(search!=NULL && strcmp("MYVAR", search->cobolName)==0);
		printf("Test: 'Get Cobol line by C file name and line number-> 116/7'\n");
		ST_Line * line = getLineCobol(cFile, 116);
		assert(line!=NULL && line->lineCobol==7);
		printf("Test: 'Get Cobol line by C file name and line number-> 123/8'\n");
		line = getLineCobol(cFile, 123);
		assert(line!=NULL && line->lineCobol==8);
		printf("Test: 'Get Cobol line by C file name and line number-> 130/9'\n");
		line = getLineCobol(cFile, 130);
		assert(line!=NULL && line->lineCobol==9);
		printf("Test: 'Get C line by cobol file name and line number-> line 7/116'\n");
		line = getLineC(cobol, 7);
		assert(line!=NULL && line->lineC==116);
		printf("Test: 'Get C line by cobol file name and line number-> line 8/123'\n");
		line = getLineC(cobol, 8);
		assert(line!=NULL && line->lineC==123);
		printf("Test: 'Get C line by cobol file name and line number-> line 9/130'\n");
		line = getLineC(cobol, 9);
		assert(line!=NULL && line->lineC==130);
		printf("Test: 'Check the cobol compiler version-> 3.1.1.0'\n");
		getVersion(version);
		assert(strcmp(version,"3.1.1.0")==0);
	}
	{
		printf("------------------------------------------------\n");
	    printf("Grupo Files:\n");
		char cSample[256];
		strcpy(cSample,realpath("./resources/sample.c", buffer));
		char cSubSample[256];
		strcpy(cSubSample,realpath("./resources/subsample.c", buffer));
		char cSubSubSample[256];
		strcpy(cSubSubSample,realpath("./resources/subsubsample.c", buffer));
		normalizePath(cSample);
		normalizePath(cSubSample);
		normalizePath(cSubSubSample);
		char c[512];
		char version[50];
		char fileGroup[4][512];
		strcpy(fileGroup[0],cSample);
		strcpy(fileGroup[1],cSubSample);
		strcpy(fileGroup[2], cSubSubSample);
		strcpy(fileGroup[3], "");
		SourceMap(fileGroup);
		printf("Test: 'Group Total parser lines'\n");
		int x = getLinesCount(); assert(x==7); 
		printf("Test: 'Group Total parser variables'\n");
		x = getVariablesCount(); assert(x==14); 
		printf("Test: 'C variable by Cobol variable name -> sample_.WS-GROUP=b_11'\n");
		ST_DebuggerVariable * search = getVariableByCobol("sample_.WS-GROUP");
		assert(search!=NULL && strcmp("b_11", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> sample_.WS-GROUP.WS-GROUP=f_11'\n");
		search = getVariableByCobol("sample_.WS-GROUP.WS-GROUP");
		assert(search!=NULL && strcmp("f_11", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> subsample_.WS-GROUP=f_6'\n");
		search = getVariableByCobol("subsample_.WS-GROUP");
		assert(search!=NULL && strcmp("f_6", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> subsubsample_.WS-GROUP-ALPHANUMERIC=f_11'\n");
		search = getVariableByCobol("subsubsample_.WS-GROUP-ALPHANUMERIC");
		assert(search!=NULL && strcmp("f_11", search->cName)==0);
		printf("Test: 'Cobol variable by C name ->sample_.f_11 -> WS-GROUP'\n");
		search = getVariableByC("sample_.f_11");
		assert(search!=NULL && strcmp("WS-GROUP", search->cobolName)==0);
		printf("Test: 'Cobol variable by C name ->subsample_.f_6 -> WS-GROUP'\n");
		search = getVariableByC("subsample_.f_6");
		assert(search!=NULL && strcmp("WS-GROUP", search->cobolName)==0);
		printf("Test: 'Cobol variable by C name ->subsubsample_.f_11 -> WS-GROUP-ALPHANUMERIC'\n");
		search = getVariableByC("subsubsample_.f_11");
		assert(search!=NULL && strcmp("WS-GROUP-ALPHANUMERIC", search->cobolName)==0);
		printf("Test: 'Check the cobol compiler version-> 2.2.0'\n");
		getVersion(version);
		assert(strcmp(version,"2.2.0")==0);
	}
	{
		printf("------------------------------------------------\n");
		printf("Variables Hierarchy:\n");
		char cSample[256];
		strcpy(cSample,realpath("./resources/petstore.c", buffer));
		normalizePath(cSample);
		char c[512];
		char version[50];
		char fileGroup[4][512];
		strcpy(fileGroup[0],cSample);
		strcpy(fileGroup[1], "");
		SourceMap(fileGroup);
		printf("Test: 'C variable by Cobol variable name -> petstore_.WS-BILL=b_14'\n");
		ST_DebuggerVariable * search = getVariableByCobol("petstore_.WS-BILL");
		assert(search!=NULL && strcmp("b_14", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> petstore_.WS-BILL.TOTAL-QUANTITY=f_15'\n");
		search = getVariableByCobol("petstore_.WS-BILL.TOTAL-QUANTITY");
		assert(search!=NULL && strcmp("f_15", search->cName)==0);
		printf("Test: 'Cobol variable by C variable name -> petstore_.b_14=WS-BILL'\n");
		search = getVariableByC("petstore_.b_14");
		assert(search!=NULL && strcmp("WS-BILL", search->cobolName)==0);
		printf("Test: 'Cobol variable by C variable name -> petstore_.f_15=TOTAL-QUANTITY'\n");
		search = getVariableByC("petstore_.f_15");
		assert(search!=NULL && strcmp("TOTAL-QUANTITY", search->cobolName)==0);
	}
	{
		printf("------------------------------------------------\n");
		printf("Find variables by function and COBOL name:\n");
		char cSample[256];
		strcpy(cSample,realpath("./resources/petstore.c", buffer));
		normalizePath(cSample);
		char c[512];
		char version[50];
		char fileGroup[4][512];
		strcpy(fileGroup[0],cSample);
		strcpy(fileGroup[1], "");
		SourceMap(fileGroup);
		printf("Test: 'C variable by Cobol variable name -> petstore_,TOTAL-QUANTITY => f_15'\n");
		ST_DebuggerVariable * search = findVariableByCobol("petstore_", "TOTAL-QUANTITY");
		assert(search!=NULL && strcmp("f_15", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> petstore_,WS-BILL.TOTAL-QUANTITY => f_15'\n");
		search = findVariableByCobol("petstore_", "WS-BILL.TOTAL-QUANTITY");
		assert(search!=NULL && strcmp("f_15", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> petstore_,BLABLABLA => NULL'\n");
		search = findVariableByCobol("petstore_", "BLABLABLA");
		assert(search==NULL);
		printf("Test: 'C variable by Cobol variable name -> blablaba_,WS-BILL.TOTAL-QUANTITY => NULL'\n");
		search = findVariableByCobol("blablaba_", "WS-BILL.TOTAL-QUANTITY");
		assert(search==NULL);
	}
	{
		printf("------------------------------------------------\n");
		printf("Attributes:\n");
		char cSample[256];
		strcpy(cSample,realpath("./resources/datatypes.c", buffer));
		normalizePath(cSample);
		char c[512];
		char version[50];
		char fileGroup[4][512];
		strcpy(fileGroup[0],cSample);
		strcpy(fileGroup[1], "");
		SourceMap(fileGroup);
		ST_DebuggerVariable * search = getVariablesByCobol();
    	while(search!=NULL){        
       		if(search->variablesByCobol!=NULL){
				assert(search->attribute!=NULL);
				assert(search->attribute->type!=NULL);
				assert(strcmp("undefined",search->attribute->type)!=0);
			}
        	search = search->next;
    	}
		printf("Test: 'Type C variable by Cobol variable name -> datatypes_.NUMERIC-DATA.DISPP => f_15'\n");
		search = getVariableByCobol("datatypes_.NUMERIC-DATA.DISPP");
		assert(search!=NULL && strcmp("numeric", search->attribute->type)==0);
		assert(search!=NULL && search->attribute->digits==8);
		assert(search!=NULL && search->attribute->scale==8);
	}	
	{
		printf("------------------------------------------------\n");
		printf("Multiple Functions:\n");
		char cSample[256];
		strcpy(cSample,realpath("./resources/func.c", buffer));
		normalizePath(cSample);
		char c[512];
		char version[50];
		char fileGroup[4][512];
		strcpy(fileGroup[0],cSample);
		strcpy(fileGroup[1], "");
		SourceMap(fileGroup);
		printf("Test: 'Cobol variable by C variable name -> func_.f_6=WS-BILL'\n");
		ST_DebuggerVariable * search = getVariableByC("func_.f_6");
		assert(search!=NULL && strcmp("argA", search->cobolName)==0);
		printf("Test: 'Cobol variable by C variable name -> dvd_.f_14=WS-BILL'\n");
		search = getVariableByC("dvd_.f_14");
		assert(search!=NULL && strcmp("dividend", search->cobolName)==0);
		printf("Test: 'C variable by Cobol variable name -> func_.ARGA=b_6'\n");
		search = getVariableByCobol("func_.ARGA");
		assert(search!=NULL && strcmp("b_6", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> dvd_.DIVIDEND=f_14'\n");
		search = getVariableByCobol("dvd_.DIVIDEND");
		assert(search!=NULL && strcmp("f_14", search->cName)==0);
		printf("Test: 'C variable by Cobol variable name -> mlp_.ARGA=f_23'\n");
		search = getVariableByCobol("mlp_.ARGA");
		assert(search!=NULL && strcmp("f_23", search->cName)==0);
		printf("Test: 'Check the cobol compiler version-> 2.2.0'\n");
		getVersion(version);
		assert(strcmp(version,"2.2.0")==0);
	}
	//
    return 0;
}
