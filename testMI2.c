#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cobgdb.h"

int testMI2()
{
    {
        printf("------------------------------------------------\n");
        printf("Testing-> couldBeOutput\n");
        printf("------------------------------------------------\n");
        printf("Test: ''\n");
        int x = couldBeOutput("");
        assert(x==1); 
        printf("Test: '\\n'\n");
        x = couldBeOutput("\n");
        assert(x==1); 
        printf("Test: Hello\n");
        x=couldBeOutput("Hello");
        assert(x==1); 
        printf("Test: ==\n");
        x=couldBeOutput("==");
        assert(x==1); 
        printf("Test: __\n");
        x=couldBeOutput("__");
        assert(x==1); 
        printf("Test: --\n");
        x=couldBeOutput("--");
        assert(x==1); 
        printf("Test: **\n");
        x=couldBeOutput("**");
        assert(x==1); 
        printf("Test: ++\n");
        x=couldBeOutput("++");
        assert(x==1); 
        printf("Test: 2^done\n");
        x=couldBeOutput("2^done");
        assert(x==0); 
        printf("Test: undefined=thread-group-added,id=\"i1\"\n");
        x=couldBeOutput("undefined=thread-group-added,id=\"i1\"");
        assert(x==0); 
        printf("Test: *stopped,reason\n");
        x=couldBeOutput("*stopped,reason");
        assert(x==0); 
        printf("Test: =breakpoint-modified,bkpt\n");
        x=couldBeOutput("=breakpoint-modified,bkpt");
        assert(x==0); 
    }
    {
        printf("------------------------------------------------\n");
        printf("Testing-> MI Parse\n");
        printf("------------------------------------------------\n");
        ST_MIInfo * parsed;
        ST_TableValues * search= NULL;
        printf("Test: 4=thread-exited,id=\"3\",group-id=\"i1\"\n");
        parsed = parseMI("4=thread-exited,id=\"3\",group-id=\"i1\"");
        assert(parsed!=NULL);
        assert(parsed->token == 4);  
        assert(parsed->outOfBandRecord!=NULL && parsed->outOfBandRecord->next==NULL);
        assert(parsed->outOfBandRecord->isStream==FALSE);
        assert(strcmp(parsed->outOfBandRecord->aysncClass, "thread-exited")==0);
        assert(parsed->outOfBandRecord->output!=NULL);
        int x=0;
        search=parsed->outOfBandRecord->output;
        while(search!=NULL){
            x++;search=search->next;
        }    
        assert(x==2);
        search=parsed->outOfBandRecord->output;
        assert(search!=NULL && strcmp(search->key,"id")==0 && strcmp(search->value,"3")==0);
        search=search->next;
        assert(search!=NULL && strcmp(search->key,"group-id")==0 && strcmp(search->value,"i1")==0);
        freeParsed(parsed);
        parsed = parseMI("~\"[Thread 0x7fffe993a700 (LWP 11002) exited]\n\"");
        assert(parsed!=NULL);
        assert(parsed->token == 0);
        assert(parsed->outOfBandRecord!=NULL && parsed->outOfBandRecord->next==NULL);
        assert(parsed->outOfBandRecord->isStream==TRUE);  
        assert(strcmp(parsed->outOfBandRecord->content,"[Thread 0x7fffe993a700 (LWP 11002) exited]\n")==0 );
        assert(parsed->resultRecords==NULL);
        freeParsed(parsed);
        parsed = parseMI("~\"[Depuraci\303\263n de hilo usando libthread_db enabled]\n\"");
        assert(parsed!=NULL);
        assert(parsed->token == 0);
        assert(parsed->outOfBandRecord!=NULL && parsed->outOfBandRecord->next==NULL);
        assert(parsed->outOfBandRecord->isStream==TRUE);  
        assert(strcmp(parsed->outOfBandRecord->content,"[Depuración de hilo usando libthread_db enabled]\n")==0 );
//        assert(parsed->resultRecords==NULL);
//        freeParsed(parsed);
//        parsed = parseMI("~\"4\\t  std::cout << \\\"\345\245\275\345245\275\345\255\246\344\271\240\357\274\214\345\244\251\345\244\251\345\220\221\344\270\212\\\" << std::endl;\\n\"");
//        assert(parsed!=NULL);
//        assert(parsed->token == 0);
//        assert(parsed->outOfBandRecord!=NULL && parsed->outOfBandRecord->next==NULL);
//        assert(parsed->outOfBandRecord->isStream==TRUE);  
//        assert(strcmp(parsed->outOfBandRecord->content,"4\t  std::cout << \"好好学习，天天向上\" << std::endl;\n")==0 );
//        assert(parsed->resultRecords==NULL);
        printf("Test->Empty line\n");
        freeParsed(parsed);
        parsed = parseMI("");
        assert(parsed!=NULL);
        assert(parsed->token == 0);
        assert(parsed->outOfBandRecord==NULL);
        assert(parsed->resultRecords==NULL);
        printf("Test->'(gdb)' line\n");
        freeParsed(parsed);
        parsed = parseMI("(gdb)");
        assert(parsed!=NULL);
        assert(parsed->token == 0);
        assert(parsed->outOfBandRecord==NULL);
        assert(parsed->resultRecords==NULL);
        printf("Test->Simple result record\n");
        freeParsed(parsed);
        parsed = parseMI("1^running");
        assert(parsed!=NULL);
        assert(parsed->token == 1);
        assert(parsed->outOfBandRecord==NULL);
        assert(parsed->resultRecords!=NULL);
        printf("Testando: %s\n", parsed->resultRecords->resultClass);
        assert(strcmp(parsed->resultRecords->resultClass, "running")==0);
        assert(parsed->resultRecords->results==NULL);
        freeParsed(parsed);
    }
    {
        printf("------------------------------------------------\n");
        printf("Testing-> Values\n");
        printf("------------------------------------------------\n");
        printf("Test-> *stopped,reason=\"breakpoint-hit\",disp=\"keep\",bkptno=\"1\",frame={addr=\"0x00000000004e807f\",func=\"D main\",args=[{name=\"args\",value=\"...\"}],file=\"source/app.d\",fullname=\"/path/to/source/app.d\",line=\"157\"},thread-id=\"1\",stopped-threads=\"all\",core=\"0\"\n");
        ST_MIInfo * parsed;
        ST_TableValues * search= NULL;
        int x=0;
        parsed = parseMI("*stopped,reason=\"breakpoint-hit\",disp=\"keep\",bkptno=\"1\",frame={addr=\"0x00000000004e807f\",func=\"D main\",args=[{name=\"args\",value=\"...\"}],file=\"source/app.d\",fullname=\"/path/to/source/app.d\",line=\"157\"},thread-id=\"1\",stopped-threads=\"all\",core=\"0\"");
        assert(parsed!=NULL);
        assert(parsed->token == 0);
        assert(parsed->outOfBandRecord!=NULL);
        assert(parsed->outOfBandRecord->isStream==FALSE);
        assert(strcmp(parsed->outOfBandRecord->aysncClass, "stopped")==0);
        x=0;
        search=parsed->outOfBandRecord->output;
        while(search!=NULL){
            x++;search=search->next;
        }    
        assert(x==7);
        search=parsed->outOfBandRecord->output;
        printf("   [\"reason\", \"breakpoint-hit\"]\n");
        assert(strcmp(search->key,"reason")==0);
        assert(strcmp(search->value,"breakpoint-hit")==0);
        search=search->next;
        printf("   [\"disp\", \"keep\"]\n");
        assert(strcmp(search->key,"disp")==0);
        assert(strcmp(search->value,"keep")==0);
        search=search->next;
        printf("   [\"bkptno\", \"1\"]\n");
        assert(strcmp(search->key,"bkptno")==0);
        assert(strcmp(search->value,"1")==0);
        search=search->next;
        printf("   [\"frame\" = [\n");
        assert(strcmp(search->key,"frame")==0);
        ST_TableValues * back1= search;
        search=search->array;
        printf("        [\"addr\", \"0x00000000004e807f\"]\n");
        assert(strcmp(search->key,"addr")==0);
        assert(strcmp(search->value,"0x00000000004e807f")==0);
        search=search->next;
        printf("        [\"func\", \"D main\"]\n");
        assert(strcmp(search->key,"func")==0);
        assert(strcmp(search->value,"D main")==0);
        search=search->next;
        ST_TableValues * back2= search;
        printf("        [\"args\" = [\n");
        assert(strcmp(search->key,"args")==0);
        search=search->array;
        printf("                [\"name\", \"args\"]\n");
        assert(strcmp(search->key,"name")==0);
        assert(strcmp(search->value,"args")==0);
        search=search->next;
        printf("                [\"value\", \"...\"]\n");
        assert(strcmp(search->key,"value")==0);
        assert(strcmp(search->value,"...")==0);
        printf("        ]\n");
        search=back2;
        search=search->next;
        printf("        [\"file\", \"source/app.d\"]\n");
        assert(strcmp(search->key,"file")==0);
        assert(strcmp(search->value,"source/app.d")==0);
        search=search->next;
        printf("        [\"fullname\", \"/path/to/source/app.d\"]\n");
        assert(strcmp(search->key,"fullname")==0);
        assert(strcmp(search->value,"/path/to/source/app.d")==0);
        search=search->next;
        printf("        [\"line\", \"157\"]\n");
        assert(strcmp(search->key,"line")==0);
        assert(strcmp(search->value,"157")==0);
        printf("   ]\n");
        search=back1;
        search=search->next;
        printf("   [\"thread-id\", \"1\"]\n");
        assert(strcmp(search->key,"thread-id")==0);
        assert(strcmp(search->value,"1")==0);
        search=search->next;
        printf("   [\"stopped-threads\", \"all\"]\n");
        assert(strcmp(search->key,"stopped-threads")==0);
        assert(strcmp(search->value,"all")==0);
        search=search->next;
        printf("   [\"core\", \"0\"]\n");
        assert(strcmp(search->key,"core")==0);
        assert(strcmp(search->value,"0")==0);
        printf("Test-> parsed.resultRecords = NULL");
        assert(parsed->resultRecords==NULL);
        printf("   ]\n");
        freeParsed(parsed);
    }
    {
        printf("------------------------------------------------\n");
        printf("Test-> 2^done,asm_insns=[src_and_asm_line={line=\"134\",file=\"source/app.d\",fullname=\"/path/to/source/app.d\",line_asm_insn=[{address=\"0x00000000004e7da4\",func-name=\"_Dmain\",offset=\"0\",inst=\"push   %%rbp\"},{address=\"0x00000000004e7da5\",func-name=\"_Dmain\",offset=\"1\",inst=\"mov    %%rsp,%%rbp\"}]}]");
        ST_MIInfo * parsed;
        ST_TableValues * search= NULL;
        int x=0;
        parsed = parseMI("2^done,asm_insns=[src_and_asm_line={line=\"134\",file=\"source/app.d\",fullname=\"/path/to/source/app.d\",line_asm_insn=[{address=\"0x00000000004e7da4\",func-name=\"_Dmain\",offset=\"0\",inst=\"push   %rbp\"},{address=\"0x00000000004e7da5\",func-name=\"_Dmain\",offset=\"1\",inst=\"mov    %rsp,%rbp\"}]}]");
        assert(parsed!=NULL);
        assert(parsed->token == 2);
        assert(parsed->outOfBandRecord==NULL);
        assert(parsed->resultRecords!=NULL);
        assert(strcmp(parsed->resultRecords->resultClass, "done")==0);
        search=parsed->resultRecords->results;
        printf("   [\"asm_insns=[\"");
        assert(strcmp(search->key,"asm_insns")==0);
        search=search->array;
        printf("        [\"src_and_asm_line=[\"");
        assert(strcmp(search->key,"src_and_asm_line")==0);
        search=search->array;
        printf("        [\"line\", \"134\"]\n");
        assert(strcmp(search->key,"line")==0);
        assert(strcmp(search->value,"134")==0);
        search=search->next;
        printf("        [\"file\", \"source/app.d\"]\n");
        assert(strcmp(search->key,"file")==0);
        assert(strcmp(search->value,"source/app.d")==0);
        search=search->next;
        printf("        [\"fullname\", \"/path/to/source/app.d\"]\n");
        assert(strcmp(search->key,"fullname")==0);
        assert(strcmp(search->value,"/path/to/source/app.d")==0);
        search=search->next;
        printf("             line_asm_insn=[\"\n");
        assert(strcmp(search->key,"line_asm_insn")==0);
        search=search->array;
        ST_TableValues * back1=search;
        printf("                 [\"address\", \"0x00000000004e7da4\"]\n");
        assert(strcmp(search->key,"address")==0);
        assert(strcmp(search->value,"0x00000000004e7da4")==0);
        search=search->next;
        printf("                 [\"func-name\", \"_Dmain\"]\n");
        assert(strcmp(search->key,"func-name")==0);
        assert(strcmp(search->value,"_Dmain")==0);
        search=search->next;
        printf("                 [\"offset\", \"0\"]\n");
        assert(strcmp(search->key,"offset")==0);
        assert(strcmp(search->value,"0")==0);
        search=search->next;
        printf("                 [\"inst\", \"push   %%rbp\"]\n");
        assert(strcmp(search->key,"inst")==0);
        assert(strcmp(search->value,"push   %rbp")==0);
        search=back1;
        search=search->next_array;
        search=search->array;
        printf("                 [\"address\", \"0x00000000004e7da5\"]\n");
        assert(strcmp(search->key,"address")==0);
        assert(strcmp(search->value,"0x00000000004e7da5")==0);
        search=search->next;
        printf("                 [\"func-name\", \"_Dmain\"]\n");
        assert(strcmp(search->key,"func-name")==0);
        assert(strcmp(search->value,"_Dmain")==0);
        search=search->next;
        printf("                 [\"offset\", \"1\"]\n");
        assert(strcmp(search->key,"offset")==0);
        assert(strcmp(search->value,"1")==0);
        search=search->next;
        printf("                 [\"inst\", \"mov    %%rsp,%%rbp\"]\n");
        assert(strcmp(search->key,"inst")==0);
        assert(strcmp(search->value,"mov    %rsp,%rbp")==0);
        boolean find=FALSE;
        printf("Test->asm_insns.src_and_asm_line.line_asm_insn[1].address = 0x00000000004e7da5\n");
        search=parseMIvalueOf(parsed->resultRecords->results, "asm_insns.src_and_asm_line.line_asm_insn[1].address", NULL, &find);
        assert(strcmp(search->value,"0x00000000004e7da5")==0);
        freeParsed(parsed);
        parsed = parseMI("2^done,obj=["
                    "[frame=[level=\"0\",addr=\"0x0000000000435f70\",func=\"D main\",file=\"source/app.d\",fullname=\"/path/to/source/app.d\",line=\"5\"]],"
                    "[frame=[level=\"1\",addr=\"0x00000000004372d3\",func=\"rt.dmain2._d_run_main()\"]],"
                    "[frame=[level=\"2\",addr=\"0x0000000000437229\",func=\"rt.dmain2._d_run_main()\"]]"
                    "]");

        printf("Test->@Obj[0]\n");
        find=FALSE;
        ST_TableValues * obj0=parseMIvalueOf(parsed->resultRecords->results, "@obj[0]", NULL, &find);
        assert(obj0->array!=NULL);

        printf("Test->@frame.level = 0\n");
        find=FALSE;
        search=parseMIvalueOf(obj0, "@frame.level", NULL, &find);
        assert(strcmp(search->value,"0")==0);
        printf("Test->@frame.addr = 0x0000000000435f70\n");
        find=FALSE;
        search=parseMIvalueOf(obj0, "@frame.addr", NULL, &find);
        assert(strcmp(search->value,"0x0000000000435f70")==0);
        printf("Test->@frame.func = D main\n");
        find=FALSE;
        search=parseMIvalueOf(obj0, "@frame.func", NULL, &find);
        assert(strcmp(search->value,"D main")==0);

        printf("Test->@frame.file = source/app.d\n");
        find=FALSE;
        search=parseMIvalueOf(obj0, "@frame.file", NULL, &find);
        assert(strcmp(search->value,"source/app.d")==0);

        printf("Test->@frame.fullname = /path/to/source/app.d\n");
        find=FALSE;
        search=parseMIvalueOf(obj0, "@frame.fullname", NULL, &find);
        assert(strcmp(search->value,"/path/to/source/app.d")==0);

        printf("Test->@frame.line = 5\n");
        find=FALSE;
        search=parseMIvalueOf(obj0, "@frame.line", NULL, &find);
        assert(strcmp(search->value,"5")==0);

        printf("Test->@Obj[1]\n");
        find=FALSE;
        ST_TableValues * obj1=parseMIvalueOf(parsed->resultRecords->results, "@obj[1]", NULL, &find);
        assert(obj1->array!=NULL);

        printf("Test->@frame.level = 1\n");
        find=FALSE;
        search=parseMIvalueOf(obj1, "@frame.level", NULL, &find);
        assert(strcmp(search->value,"1")==0);

        printf("Test->@frame.addr = 0x00000000004372d3\n");
        find=FALSE;
        search=parseMIvalueOf(obj1, "@frame.addr", NULL, &find);
        assert(strcmp(search->value,"0x00000000004372d3")==0);

        printf("Test->@frame.func = rt.dmain2._d_run_main()\n");
        find=FALSE;
        search=parseMIvalueOf(obj1, "@frame.func", NULL, &find);
        assert(strcmp(search->value,"rt.dmain2._d_run_main()")==0);

        printf("Test->@frame.file = NULL\n");
        find=FALSE;
        search=parseMIvalueOf(obj1, "@frame.file", NULL, &find);
        assert(search->value==NULL);

        printf("Test->@frame.fullname = NULL\n");
        find=FALSE;
        search=parseMIvalueOf(obj1, "@frame.fullname", NULL, &find);
        assert(search->value==NULL);

        printf("Test->@frame.line = NULL\n");
        find=FALSE;
        search=parseMIvalueOf(obj1, "@frame.line", NULL, &find);
        assert(search->value==NULL);
        freeParsed(parsed);
    }
    {
        printf("------------------------------------------------\n");
        printf("Testing -> Empty string values\n");
        printf("------------------------------------------------\n");
        ST_MIInfo * parsed;
        ST_TableValues * search= NULL;
        boolean find=FALSE;
        int x=0;
        printf("Test-> 15^done,register-names=[\"r0\",\"pc\",\"\",\"xpsr\",\"\",\"control\"]\n");
        parsed = parseMI("15^done,register-names=[\"r0\",\"pc\",\"\",\"xpsr\",\"\",\"control\"]");
        search=parseMIvalueOf(parsed->resultRecords->results, "register-names", NULL, &find);
        search=search->array;
        assert(strcmp(search->value,"r0")==0);
        search=search->next;
        assert(strcmp(search->value,"pc")==0);
        search=search->next;
        assert(strcmp(search->value,"")==0);
        search=search->next;
        assert(strcmp(search->value,"xpsr")==0);
        search=search->next;
        assert(strcmp(search->value,"")==0);
        search=search->next;
        assert(strcmp(search->value,"control")==0);
        freeParsed(parsed);
    }
    {
        printf("------------------------------------------------\n");
        printf("Testing -> Empty array values\n");
        printf("------------------------------------------------\n");
        ST_MIInfo * parsed;
        ST_TableValues * search= NULL;
        boolean find=FALSE;
        printf("Test-> 15^done,foo={x=[],y=\"y\"}\n");
        parsed = parseMI("15^done,foo={x=[],y=\"y\"}");
        search=parseMIvalueOf(parsed->resultRecords->results, "foo.x", NULL, &find);
        assert(search->array==NULL);
        find=FALSE;
        search=parseMIvalueOf(parsed->resultRecords->results, "foo.y", NULL, &find);
        assert(strcmp(search->value,"y")==0);
        freeParsed(parsed);
    }
    {
        printf("------------------------------------------------\n");
        printf("Testing -> variables\n");
        printf("------------------------------------------------\n");
        ST_MIInfo * parsed;
        ST_TableValues * search1= NULL;
        ST_TableValues * search= NULL;
        boolean find=FALSE;
        printf("Test-> 12^done,variables=[{name=\"b_6\",type=\"unsigned char [3]\"}]\n");
        parsed = parseMI("12^done,variables=[{name=\"b_6\",type=\"unsigned char [3]\"}]");
        search1=parseMIvalueOf(parsed->resultRecords->results, "variables", NULL, &find);
        find=FALSE;
        search=parseMIvalueOf(search1, "name", NULL, &find);
        assert(strcmp(search->value,"b_6")==0);
        find=FALSE;
        search=parseMIvalueOf(search1, "value", NULL, &find);
        assert(search->value==NULL);
        find=FALSE;
        search=parseMIvalueOf(search1, "type", NULL, &find);
        assert(strcmp(search->value,"unsigned char [3]")==0);
        freeParsed(parsed);
    }
    printf("Test - MI2 - Fim\n");
    return 0;
}
