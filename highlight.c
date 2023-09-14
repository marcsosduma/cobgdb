#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "cobgdb.h"

const wchar_t COBOL_RESERVED_WORDS[] = L" ABSENT ACCEPT ACCESS ACTION ACTIVATING ACTIVE-CLASS ACTIVE-X ACTUAL ADD ADDRESS ADJUSTABLE-COLUMNS ADVANCING AFTER ALIGNED ALIGNMENT ALL ALLOCATE ALLOWING ALPHABET ALPHABETIC ALPHABETIC-LOWER ALPHABETIC-UPPER ALPHANUMERIC ALPHANUMERIC-EDITED ALSO ALTER ALTERNATE AND ANUM ANY ANYCASE APPLY ARE AREA AREAS ARGUMENT-NUMBER ARGUMENT-VALUE ARITHMETIC AS ASCENDING ASCII ASSIGN AT ATTRIBUTE ATTRIBUTES  AUTO AUTO-DECIMAL AUTO-SKIP AUTO-SPIN AUTOMATIC AUTOTERMINATE AWAY-FROM-ZERO B-AND B-NOT B-OR B-SHIFT-L B-SHIFT-LC B-SHIFT-R B-SHIFT-RC B-XOR BACKGROUND-COLOR BACKGROUND-COLOUR BACKGROUND-HIGH BACKGROUND-LOW BACKGROUND-STANDARD BACKWARD BAR BASED BEEP BEFORE BELL BINARY BINARY-C-LONG BINARY-CHAR BINARY-DOUBLE BINARY-INT BINARY-LONG BINARY-LONG-LONG BINARY-SEQUENTIAL BINARY-SHORT BIT BITMAP BITMAP-END BITMAP-HANDLE BITMAP-NUMBER BITMAP-START BITMAP-TIMER BITMAP-TRAILING BITMAP-TRANSPARENT-COLOR BITMAP-WIDTH BLANK BLINK BLOCK BOOLEAN BOTTOM BOX BOXED BULK-ADDITION BUSY BUTTONS BY BYTE BYTE-LENGTH BYTES C CALENDAR-FONT CANCEL CANCEL-BUTTON CAPACITY CARD-PUNCH CARD-READER CASSETTE CCOL CD CELL CELL-COLOR CELL-DATA CELL-FONT CELL-PROTECTION CELLS CENTER CENTERED CENTERED-HEADINGS CENTURY-DATE CF CH CHAIN CHAINING CHANGED CHARACTER CHARACTERS CHECK-BOX CLASS CLASS-ID CLASSIFICATION CLEAR-SELECTION CLINE CLINES CLOSE COBOL CODE CODE-SET COL COLLATING COLOR COLORS COLOURS COLS COLUMN COLUMN-COLOR COLUMN-DIVIDERS COLUMN-FONT COLUMN-HEADINGS COLUMN-PROTECTION COLUMNS COMBO-BOX COMMA COMMAND-LINE COMMIT COMMON "
                                        " COMMUNICATION COMP COMP-0 COMP-1 COMP-10 COMP-15 COMP-2 COMP-3 COMP-4 COMP-5 COMP-6 COMP-9 COMP-N COMP-X COMPUTATIONAL COMPUTATIONAL-0 COMPUTATIONAL-1 COMPUTATIONAL-2 COMPUTATIONAL-3 COMPUTATIONAL-4 COMPUTATIONAL-5 COMPUTATIONAL-6 COMPUTATIONAL-N COMPUTATIONAL-X COMPUTE CONDITION CONFIGURATION CONSTANT CONTAINS CONTENT CONTROL CONTROLS CONVERSION CONVERTING COPY COPY-SELECTION CORE-INDEX CORR CORRESPONDING COUNT CRT CRT-UNDER CSIZE CURRENCY CURRENT CURSOR CURSOR-COL CURSOR-COLOR CURSOR-FRAME-WIDTH CURSOR-ROW CURSOR-X CURSOR-Y CUSTOM-PRINT-TEMPLATE CYCLE CYL-INDEX CYL-OVERFLOW DASHED DATA DATA-COLUMNS DATA-POINTER DATA-TYPES DATE DATE-COMPILED DATE-ENTRY DATE-MODIFIED DATE-WRITTEN DAY DAY-OF-WEEK DE DEBUGGING DECIMAL-POINT DECLARATIVES DEFAULT DEFAULT-BUTTON DEFAULT-FONT DELETE DELIMITED DELIMITER DEPENDING DESCENDING DESTINATION DESTROY DETAIL DISABLE DISC DISK DISP DISPLAY DISPLAY-1 DISPLAY-COLUMNS DISPLAY-FORMAT DIVIDE DIVIDER-COLOR DIVIDERS DIVISION DOTDASH DOTTED DOUBLE DOWN DRAG-COLOR DROP-DOWN DROP-LIST DUPLICATES DYNAMIC EBCDIC EC ECHO EDITING EGI ELEMENT  EMI EMPTY-CHECK ENABLE ENCODING ENCRYPTION END END-ACCEPT END-ADD END-CHAIN END-COLOR END-COMPUTE END-DELETE END-DISPLAY END-DIVIDE END-JSON END-MODIFY END-MULTIPLY END-OF-PAGE END-READ END-RECEIVE END-RETURN END-REWRITE END-SEARCH END-SEND END-START END-STRING END-SUBTRACT END-UNSTRING END-WRITE END-XML ENGRAVED ENSURE-VISIBLE ENTRY ENTRY-CONVENTION ENTRY-FIELD ENTRY-REASON ENVIRONMENT ENVIRONMENT-NAME "
                                        " ENVIRONMENT-VALUE EO EOL EOP EOS EQUAL EQUALS ERASE ERROR ESCAPE ESCAPE-BUTTON ESI EVENT EVENT-LIST EVERY EXCEPTION EXCEPTION-OBJECT EXCEPTION-VALUE EXCLUSIVE EXCLUSIVE-OR EXHIBIT EXIT EXPAND EXPANDS EXTEND EXTENDED-SEARCH EXTERN EXTERNAL EXTERNAL-FORM FACTORY FALSE FD FH--FCD FH--KEYDEF FILE FILE-CONTROL FILE-ID FILE-LIMIT FILE-LIMITS FILE-NAME FILE-POS FILL-COLOR FILL-COLOR2 FILL-PERCENT FILLER FINAL FINALLY FINISH-REASON FIRST FIXED FIXED-FONT FIXED-WIDTH FLAT FLAT-BUTTONS FLOAT FLOAT-BINARY-128 FLOAT-BINARY-32 FLOAT-BINARY-64 FLOAT-DECIMAL-16 FLOAT-DECIMAL-34 FLOAT-EXTENDED FLOAT-INFINITY FLOAT-LONG FLOAT-NOT-A-NUMBER FLOAT-SHORT FLOATING FONT FOOTING FOR FOREGROUND-COLOR FOREGROUND-COLOUR FOREVER FORMAT FRAME FRAMED FREE FROM FULL FULL-HEIGHT FUNCTION FUNCTION-ID FUNCTION-POINTER GENERATE GET GIVING GLOBAL GO-BACK GO-FORWARD GO-HOME GO-SEARCH GOBACK GRAPHICAL GREATER GRID GROUP GROUP-USAGE GROUP-VALUE HANDLE HAS-CHILDREN HEADING HEADING-COLOR HEADING-DIVIDER-COLOR HEADING-FONT HEAVY HEIGHT-IN-CELLS HEX HIDDEN-DATA HIGH-COLOR HIGH-VALUE HIGH-VALUES HIGHLIGHT HOT-TRACK HSCROLL HSCROLL-POS I-O I-O-CONTROL ICON ID IDENTIFICATION IDENTIFIED IF IGNORE IGNORING IMPLEMENTS IN INDEPENDENT INDEX INDEXED INDICATE INHERITS INITIAL INITIALISE INITIALISED INITIALIZE INITIALIZED INITIATE INPUT INPUT-OUTPUT INQUIRE INSERT-ROWS INSERTION-INDEX INSPECT INSTALLATION INTERFACE INTERFACE-ID INTERMEDIATE INTO INTRINSIC INVALID INVOKE IS ITEM ITEM-TEXT ITEM-TO-ADD ITEM-TO-DELETE ITEM-TO-EMPTY ITEM-VALUE JSON JUST JUSTIFIED "
                                        " KEPT KEY KEYBOARD LABEL LABEL-OFFSET LARGE-FONT LARGE-OFFSET LAST LAST-ROW LAYOUT-DATA LAYOUT-MANAGER LC_ALL LC_COLLATE LC_CTYPE LC_MESSAGES LC_MONETARY LC_NUMERIC LC_TIME LEADING LEADING-SHIFT LEAVE LEFT LEFT-JUSTIFY LEFT-TEXT LEFTLINE LENGTH LENGTH-CHECK LESS LIKE LIMIT LIMITS LINAGE LINAGE-COUNTER LINE LINE-COUNTER LINE-SEQUENTIAL LINES LINES-AT-ROOT LINKAGE LIST-BOX LM-RESIZE LOC LOCAL-STORAGE LOCALE LOCATION LOCK LOCK-HOLDING LONG-DATE LOW-COLOR LOW-VALUE LOW-VALUES LOWER LOWERED LOWLIGHT MAGNETIC-TAPE MANUAL MASS-UPDATE MASTER-INDEX MAX-LINES MAX-PROGRESS MAX-TEXT MAX-VAL MEDIUM-FONT MEMORY MENU MERGE MESSAGE MESSAGE-TAG METHOD METHOD-ID MICROSECOND-TIME MIN-VAL MINUS MODE MODIFY MODULES MOVE MULTILINE MULTIPLE MULTIPLY NAME NAMED NAMESPACE NAMESPACE-PREFIX NAT NATIONAL NATIONAL-EDITED NATIVE NAVIGATE-URL NEAREST-AWAY-FROM-ZERO NEAREST-EVEN NEAREST-TOWARD-ZERO NEGATIVE NESTED NEW NEXT NEXT-ITEM NO NO-AUTO-DEFAULT NO-AUTOSEL NO-BOX NO-DIVIDERS NO-ECHO NO-F4 NO-FOCUS NO-GROUP-TAB NO-KEY-LETTER NO-SEARCH NO-UPDOWN NOMINAL NONE NONNUMERIC NORMAL NOT NOTAB NOTHING NOTIFY NOTIFY-CHANGE NOTIFY-DBLCLICK NOTIFY-SELCHANGE NULL NULLS NUM-COL-HEADINGS NUM-ROWS NUMBER NUMBERS NUMERIC NUMERIC-EDITED OBJECT OBJECT-COMPUTER OBJECT-REFERENCE OCCURS OF OFF OK-BUTTON OMITTED ON ONLY OPEN OPTIONAL OPTIONS OR ORDER ORGANISATION ORGANIZATION OTHER OTHERS OUTPUT OVERFLOW OVERLAP-LEFT OVERLAP-TOP OVERLINE OVERRIDE PACKED-DECIMAL PADDING PAGE PAGE-COUNTER PAGE-SETUP PAGED PARAGRAPH PARENT PARSE PASCAL PASSWORD PERMANENT PF PH "
                                        " PHYSICAL PIC PICTURE PIXEL PIXELS PLACEMENT PLUS POINTER POP-UP POS POSITION POSITION-SHIFT POSITIVE PREFIXED PRESENT PREVIOUS PRINT PRINT-NO-PROMPT PRINT-PREVIEW PRINTER PRINTER-1 PRINTING PRIORITY PROCEDURE PROCEDURE-POINTER PROCEDURES PROCEED PROCESSING PROGRAM PROGRAM-ID PROGRAM-POINTER PROGRESS PROHIBITED PROMPT PROPERTIES PROPERTY PROTECTED PROTOTYPE PURGE PUSH-BUTTON QUERY-INDEX QUEUE QUOTE QUOTES RADIO-BUTTON RAISE RAISED RAISING RANDOM RD READ READ-ONLY READERS RECEIVE RECEIVED RECORD RECORD-DATA RECORD-OVERFLOW RECORD-TO-ADD RECORD-TO-DELETE RECORDING RECORDS RECURSIVE REDEFINES REEL REFERENCE REFERENCES REFRESH REGION-COLOR RELATION RELATIVE RELEASE REMAINDER REMARKS REMOVAL RENAMES REORG-CRITERIA REPEATED REPLACE REPLACING REPORT REPORTING REPORTS REPOSITORY REQUIRED REREAD RERUN RESERVE RESET RESET-GRID RESET-LIST RESET-TABS RESUME RETRY RETURN RETURNING REVERSE REVERSE-VIDEO REVERSED REWIND REWRITE RF RH RIGHT RIGHT-ALIGN RIGHT-JUSTIFY RIGHTLINE RIMMED ROLLBACK ROUNDED ROUNDING ROW-COLOR ROW-COLOR-PATTERN ROW-DIVIDERS ROW-FONT ROW-HEADINGS ROW-PROTECTION RUN S SAME SAVE-AS SAVE-AS-NO-PROMPT SCREEN SCROLL SCROLL-BAR SD SEARCH SEARCH-OPTIONS SEARCH-TEXT SECONDS SECTION SECURE SECURITY SEGMENT SEGMENT-LIMIT SELECT SELECT-ALL SELECTION-INDEX SELECTION-TEXT SELF SELF-ACT SEND SENTENCE SEPARATE SEPARATION SEQUENCE SEQUENTIAL SET SHADING SHADOW SHARING SHORT-DATE SHOW-LINES SHOW-NONE SHOW-SEL-ALWAYS SIGN SIGNED SIGNED-INT SIGNED-LONG SIGNED-SHORT SIZE SMALL-FONT SORT SORT-MERGE SORT-ORDER SOURCE SOURCE-COMPUTER "
                                        " SOURCES SPACE SPACE-FILL SPACES SPECIAL-NAMES SPINNER SQUARE STACK STANDARD STANDARD-1 STANDARD-2 STANDARD-BINARY STANDARD-DECIMAL START START-X START-Y STATEMENT STATIC STATIC-LIST STATUS STATUS-BAR STATUS-TEXT STDCALL STEP STOP STRING STRONG STYLE SUB-QUEUE-1 SUB-QUEUE-2 SUB-QUEUE-3 SUBTRACT SUBWINDOW SUM SUPER SUPPRESS SYMBOL SYMBOLIC SYNC SYNCHRONISED SYNCHRONIZED SYSTEM-DEFAULT SYSTEM-INFO SYSTEM-OFFSET TAB TAB-TO-ADD TAB-TO-DELETE TABLE TALLYING TAPE TEMPORARY TERMINAL-INFO TERMINATE TERMINATION-VALUE TEST TEXT THAN THREAD THREADS THROUGH THUMB-POSITION TILED-HEADINGS TIME TIME-OUT TIMEOUT TIMES TITLE TITLE-POSITION TO TOP TOP-LEVEL TOWARD-GREATER TOWARD-LESSER TRACK TRACK-AREA TRACK-LIMIT TRACKS TRADITIONAL-FONT TRAILING TRAILING-SHIFT TRAILING-SIGN TRANSFORM TRANSPARENT TREE-VIEW TRUE TRUNCATION TYPE TYPEDEF U UCS-4 UNBOUNDED UNDERLINE UNFRAMED UNIT UNIVERSAL UNLOCK UNSIGNED UNSIGNED-INT UNSIGNED-LONG UNSIGNED-SHORT UNSORTED UNSTRING UNTIL UP UPDATE UPDATERS UPON UPPER USAGE USE USE-ALT USE-RETURN USE-TAB USER USER-DEFAULT USING UTF-16 UTF-8 V VAL-STATUS VALID VALIDATE VALIDATE-STATUS VALIDATING VALUE VALUE-FORMAT VALUES VARIABLE VARIANT VARYING VERTICAL VERY-HEAVY VIRTUAL-WIDTH VOLATILE VPADDING VSCROLL VSCROLL-BAR VSCROLL-POS VTOP WAIT WEB-BROWSER WIDTH WIDTH-IN-CELLS WINDOW WITH WORDS WORKING-STORAGE WRAP WRITE WRITE-ONLY WRITE-VERIFY WRITERS X XML XML-DECLARATION XML-SCHEMA XOR Y YYYYDDD YYYYMMDD ZERO ZERO-FILL ZEROES ZEROS 'ADDRESS OF' COB-CRT-STATUS DEBUG-ITEM "
                                        " 'LENGTH OF' NUMBER-OF-CALL-PARAMETERS RETURN-CODE SORT-RETURN TALLY WHEN-COMPILED XML-CODE XML-EVENT XML-INFORMATION XML-NAMESPACE XML-NAMESPACE-PREFIX XML-NNAMESPACE XML-NNAMESPACE-PREFIX XML-NTEXT XML-TEXT JSON-CODE JSON-STATUS ";

const wchar_t COBOL_SPECIAL_WORDS[] =  L" STOP RUN IF THEN ELSE END-IF PERFORM CALL END-PERFORM END-CALL AUTHOR GO CONTINUE EVALUATE END-EVALUATE WHEN THRU ";

extern struct program_file program_file;
extern int color_frame;
boolean isProcedure = FALSE;
int isProc=0;

enum types{
    TP_SPACES,
    TP_TOKEN,
    TP_SYMBOL,
    TP_NUMBER,
    TP_COMMENT,
    TP_STRING1,
    TP_STRING2,
    TP_ALPHA,
    TP_OTHER
};


wchar_t * toWUpper(wchar_t* str) {
  int j = 0;
  wchar_t ch;
 
  while (str[j]) {
        str[j]=towupper(str[j]);
        j++;
  }
  return str;
}

int parseSpaces(wchar_t * ch){
    int count=0;
    while(*ch==L' '){
        ch = ch + 1;
        count++;
    }
    return count;
}

int parseStringHigh(wchar_t * ch){
    int count=0;
    if(*ch==L'\''){
        count++;
        ch = ch+1;
        while(*ch!=L'\'' && *ch!=L'\0'  && *ch!=L'\n'){
            ch = ch + 1;
            count++;
        }
        if(*ch==L'\''){
            ch = ch + 1;
            count++;
        }
    }else{
        count++;
        ch = ch+1;
        while(*ch!=L'"' && *ch!=L'\0'  && *ch!=L'\n'){
            ch = ch + 1;
            count++;
        }
        if(*ch==L'"'){
            ch = ch + 1;
            count++;
        }
    }
    return count;
}


int parseSymbols(wchar_t * ch){
    int count=0;
    while(!iswalpha(*ch) && *ch!=L' ' && *ch!=L'\n' && *ch!=L'\0' && *ch!=L'.'){
        ch = ch + 1;
        count++;
    }
    return count;
}

int parseAlpha(wchar_t * ch){
    int count=0;
    while(iswalpha(*ch) || *ch==L'-' || *ch==L'_' || iswdigit(*ch)){
        ch = ch + 1;
        count++;
    }
    return count;
}

int parseDigitOrAlpha(wchar_t * ch,struct st_highlt * h){
    int count=0;
    int alpha=0;
    while(iswdigit(*ch)){
        ch = ch + 1;
        count++;
    }
    while(iswalpha(*ch) || *ch==L'-' || *ch==L'_'){
        ch = ch + 1;
        count++;
        alpha++;
    }
    if(alpha){
         h->color=15;
    }
    return count;
}

int printHighlight(struct st_highlt * hight, int bkg, int start, int total){
    wchar_t vl[100];
    struct st_highlt * h = hight;
    int tot=total;
    int qtdMove=0;

    //setlocale(LC_ALL,"");
    wchar_t *wcBuffer = (wchar_t *)malloc(512);

    int st = start;
    while (st > 0 && h != NULL) {
        if (st < h->size) break;
        st -= h->size;
        h = h->next;
    }
    bkg=(bkg>0)?bkg:color_black;
    print_colorBK(bkg, bkg);
    while(h!=NULL && tot>0){
        if(h->size>total){
            int x=0;
        }
        qtdMove = (tot < h->size - st)? tot : h->size - st;
        if(qtdMove>0){
            print_colorBK(h->color,bkg);
            wcsncpy(wcBuffer, &h->token[st], qtdMove);
            wcBuffer[qtdMove]='\0';
            printf("%*ls",qtdMove,wcBuffer);
            //wprintf(L"%.*ls",qtdMove,&h->token[st]);
        }
        tot-=qtdMove;
        h=h->next;
        st=0;
    }
    if(tot>0){
        print_colorBK(color_black, bkg);
        printf("%*ls", tot, L" ");
    }
    print_color_reset();
    free(wcBuffer);
}

int freeHighlight(struct st_highlt * hight){
    struct st_highlt * h = hight;
    struct st_highlt * ant = hight;
    while(h!=NULL){
        ant=h;
        h=h->next;
        free(ant);
    }
}

void isReserved(struct st_highlt * h){
    wchar_t test[50];
    if(h->size<50){
        wcscpy(test,L" ");
        wcsncpy(&test[1],h->token,h->size);
        test[h->size+1]=L'\0';
        toWUpper(test);
        wcscat(test,L" ");
        if(wcsstr(COBOL_RESERVED_WORDS, test)){
            h->color=color_blue;
            if(isProc==0 && wcsstr(L" PROCEDURE ",test)) isProc++;
            if(isProc==1 && wcsstr(L" DIVISION ",test)) isProcedure=TRUE;
        }
        if(isProcedure && wcsstr(COBOL_SPECIAL_WORDS, test)){
            h->color=color_pink;
        }        
    }
}

int executeParse(){
    Lines * lines = program_file.lines;
    char * line;
    char *saveptr1;
    struct st_highlt * tk_before = NULL;
    struct st_highlt * high = NULL;
    int pos=1;
    int tp_before=-1;
    int type=-1;
    clearScreen();
    gotoxy(1,1);
    printf("\n");
    while(lines!=NULL){
        isProc=0;
        char * ch=lines->line;
        int lenLine=strlen(lines->line);
        wchar_t *wcharString = (wchar_t *)malloc((lenLine + 1) * sizeof(wchar_t));
        #if defined(_WIN32)
        MultiByteToWideChar(CP_UTF8, 0, lines->line, -1, wcharString,(lenLine + 1) * sizeof(wchar_t) / sizeof(wcharString[0]));
        #else
        mbstowcs(wcharString, lines->line, strlen(lines->line) + 1);
        #endif
        struct st_highlt * tk_before = NULL;
        struct st_highlt * high = NULL;
        do{
            struct st_highlt * h = malloc(sizeof(struct st_highlt));
            if(high==NULL) high=h;
            if(tk_before!=NULL) tk_before->next=h;
            h->color=color_black;
            h->next=NULL;
            if(*wcharString==L' '){
                h->token=wcharString;
                type=TP_SPACES;
                h->size = parseSpaces(wcharString);
                h->color = color_black;
                wcharString+=h->size;
            }else if(*wcharString==L'\''){
                h->token=wcharString;
                type=TP_STRING1;
                h->size = parseStringHigh(wcharString);
                h->color = color_yellow;
                wcharString+=h->size;
            }else if(*wcharString==L'"'){
                h->token=wcharString;
                type=TP_STRING2;
                h->size = parseStringHigh(wcharString);
                h->color = color_yellow;
                wcharString+=h->size;
            }else if(iswdigit(*wcharString)){
                h->token=wcharString;
                h->color = color_green;
                type=TP_NUMBER;
                h->size = parseDigitOrAlpha(wcharString, h);
                isReserved(h);
                wcharString+=h->size;
            }else if(*wcharString==L'.'){
                h->token=wcharString;
                type=TP_SYMBOL;
                h->size = 1;
                h->color = color_white;
                wcharString+=h->size;
            }else if(iswalpha(*wcharString)){
                h->token=wcharString;
                type=TP_ALPHA;
                h->size = parseAlpha(wcharString);
                h->color = color_white;
                isReserved(h);
                wcharString+=h->size;
            }else if(*wcharString==L'*' && tp_before==TP_SPACES && pos==7){
                h->token=wcharString;
                type=TP_COMMENT;
                h->size = wcslen(h->token);
                h->color=color_dark_green;
                wcharString+=h->size;
                break;
            }else if(!iswalpha(*wcharString) && *wcharString!=L' ' && *wcharString!=L'\n' && *wcharString!=L'\0'){
                h->token=wcharString;
                type=TP_SYMBOL;
                h->size = parseSymbols(wcharString);
                h->color=color_green;
                if(wcsncmp(h->token,L"*>",2)==0){
                    h->color=color_dark_green;
                    type=TP_COMMENT;
                    h->size = wcslen (h->token);
                    wcharString+=h->size;
                    break;
                }else if(tp_before==TP_SPACES && wcsncmp(h->token,L">>",2)==0){
                    h->color=color_dark_green;
                    type=TP_COMMENT;
                    h->size = wcslen (h->token);
                    wcharString+=h->size;
                    break;
                }
                wcharString+=h->size;
            }else{
                h->token=wcharString;
                type=TP_OTHER;
                h->size = wcslen(wcharString);
                h->color=color_gray;
                wcharString+=h->size;
                break;
            }
            tk_before=h;
            tp_before=type;
            pos++;
        }while(*wcharString!=L'\0');
        if(high->token!=NULL && high->size>0){
            high->token[wcscspn(high->token,L"\r")]=L' ';
            high->token[wcscspn(high->token,L"\n")]=L' ';
        }
        if(high!=NULL){
            lines->high=high;
        }
        lines=lines->line_after;     
    }
    //while(key_press()==0);
}
