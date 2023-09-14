       >>SOURCE FORMAT IS FREE
       IDENTIFICATION DIVISION.
       PROGRAM-ID. CUSTOMER.
       ENVIRONMENT DIVISION.
           CONFIGURATION SECTION.
              SOURCE-COMPUTER.      
                    GNUCOBOL.
                  OBJECT-COMPUTER.      
                    GNUCOBOL.
              SPECIAL-NAMES.
           DECIMAL-POINT IS COMMA.    
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT FILE1 ASSIGN TO DISK
               ORGANIZATION IS INDEXED
               ACCESS MODE IS RANDOM
               FILE STATUS IS FS-STAT
               RECORD KEY IS FS-KEY.

       DATA DIVISION.
       FILE SECTION.
       FD FILE1 VALUE OF FILE-ID IS "customers.dat".
       01 FILE1-REC.
           05 FS-KEY.
               10 FS-PHONE PIC 9(09) BLANK WHEN ZEROS.
           05 FS-NAME     PIC X(40).
           05 FS-ADDRESS PIC X(40).
           05 FILLER      PIC X(20).

       WORKING-STORAGE SECTION.

       01 WS-MODULE.
           05 FILLER PIC X(12) VALUE "CUSTOMERS - ".
           05 WS-OP PIC X(20) VALUE SPACES.

       77 WS-CHOICE PIC X.
           88 E-INCLUDE   VALUE IS "1".
           88 E-CONSULT VALUE IS "2".
           88 E-UPDATE   VALUE IS "3".
           88 E-DELETE   VALUE IS "4".
           88 E-EXIT  VALUE IS "X" "x".
       77 FS-STAT PIC 9(02).
           88 FS-OK         VALUE ZEROS.
           88 FS-CANCEL    VALUE 99.
           88 FS-NOT-EXIST VALUE 35.
       77 WS-ERROR PIC X.
           88 E-YES VALUES ARE "Y" "y".

       77 WS_NUMR PIC 999.
       77 WS-NUMC012 PIC 999.
       77 BACK-COLOR PIC 9 VALUE 1.
       77 FRONT-COLOR PIC 9 VALUE 6.

       77 WS-STATUS PIC X(30).
       77 WS-ERRMSG PIC X(80).

       COPY screenio.

       SCREEN SECTION.
       01 SS-CLS.
           05 SS-FILLER.
               10 BLANK SCREEN.
               10 LINE 01 COLUMN 01 ERASE EOL
                  BACKGROUND-COLOR BACK-COLOR.
               10 LINE WS_NUMR COLUMN 01 ERASE EOL
                  BACKGROUND-COLOR BACK-COLOR.
           05 SS-HEADER.
               10 LINE 01 COLUMN 02 PIC X(31) FROM WS-MODULE
                  HIGHLIGHT FOREGROUND-COLOR FRONT-COLOR
                  BACKGROUND-COLOR BACK-COLOR.
           05 SS-STATUS.
               10 LINE WS_NUMR COLUMN 2 ERASE EOL PIC X(30)
                  FROM WS-STATUS HIGHLIGHT
                  FOREGROUND-COLOR FRONT-COLOR
                  BACKGROUND-COLOR BACK-COLOR.

       01 SS-MENU FOREGROUND-COLOR 6.
           05 LINE 07 COLUMN 15 VALUE "1 - INCLUDE".
           05 LINE 08 COLUMN 15 VALUE "2 - CONSULT".
           05 LINE 09 COLUMN 15 VALUE "3 - UPDATE".
           05 LINE 10 COLUMN 15 VALUE "4 - DELETE".
           05 LINE 11 COLUMN 15 VALUE "X - EXIT".
           05 LINE 13 COLUMN 15 VALUE "CHOICE: ".
           05 LINE 13 COL PLUS 1 USING WS-CHOICE AUTO.

       01 SS-RECORD-SCREEN.
           05 SS-KEY FOREGROUND-COLOR 2.
               10 LINE 10 COLUMN 10 VALUE "  PHONE:".
               10 COLUMN PLUS 2 PIC 9(09) USING FS-PHONE
                  BLANK WHEN ZEROS.
           05 SS-DATA.
               10 LINE 11 COLUMN 10 VALUE "   NAME:".
               10 COLUMN PLUS 2 PIC X(40) USING FS-NAME.
               10 LINE 12 COLUMN 10 VALUE "ADDRESS:".
               10 COLUMN PLUS 2 PIC X(40) USING FS-ADDRESS.

       01 SS-ERROR.
           05 FILLER FOREGROUND-COLOR 4 BACKGROUND-COLOR 1 HIGHLIGHT.
               10 LINE WS_NUMR COLUMN 2 PIC X(80) FROM WS-ERRMSG BELL.
               10 COLUMN PLUS 2 TO WS-ERROR.

       PROCEDURE DIVISION.
       001-START.
           SET ENVIRONMENT 'COB_SCREEN_EXCEPTIONS' TO 'Y'
           SET ENVIRONMENT 'COB_SCREEN_ESC' TO 'Y'
           SET ENVIRONMENT 'ESCDELAY' TO '25'
           *>CALL "SYSTEM" USING "chcp 437" WS-STATUS
           *>CALL "SYSTEM" USING "mode con: lines=24 cols=80" *> WS-STATUS
           ACCEPT WS_NUMR FROM LINES
           ACCEPT WS-NUMC012 FROM COLUMNS *> WS-STATUS
           PERFORM 007-OPEN-FILES
           PERFORM UNTIL E-EXIT
               MOVE "MENU" TO WS-OP
               MOVE "CHOOSE AN OPTION" TO WS-STATUS  *> WS-STATUS
               MOVE SPACES TO WS-CHOICE
               DISPLAY SS-CLS
               ACCEPT SS-MENU
               EVALUATE TRUE
                   WHEN E-INCLUDE
                       PERFORM 002-INCLUDE THRU 002-INCLUDE-END
                   WHEN E-CONSULT
                       PERFORM 003-CONSULT THRU 003-CONSULT-END
                   WHEN E-UPDATE
                       PERFORM 004-UPDATE THRU 004-UPDATE-END
                   WHEN E-DELETE
                       PERFORM 005-DELETE THRU 005-DELETE-END
               END-EVALUATE
           END-PERFORM.
       001-FINISH.
           CLOSE FILE1.
           STOP RUN.

      *> -----------------------------------
       002-INCLUDE.
           MOVE "INCLUDE" TO WS-OP.
           MOVE "ESC TO EXIT" TO WS-STATUS.
           DISPLAY SS-CLS.
           MOVE SPACES TO FILE1-REC.
       002-INCLUDE-LOOP.
           ACCEPT SS-RECORD-SCREEN.
           IF COB-CRT-STATUS = COB-SCR-ESC
               GO 002-INCLUDE-END
           END-IF
           IF FS-NAME EQUAL SPACES OR FS-ADDRESS EQUAL SPACES
               MOVE "PLEASE PROVIDE NAME AND ADDRESS" TO WS-ERRMSG
               PERFORM 008-SHOW-ERROR
               GO 002-INCLUDE-LOOP
           END-IF
           WRITE FILE1-REC
           INVALID KEY
               MOVE "CUSTOMER ALREADY EXISTS" TO WS-ERRMSG
               PERFORM 008-SHOW-ERROR
               MOVE ZEROS TO FS-KEY
           END-WRITE.
           GO 002-INCLUDE-LOOP.
       002-INCLUDE-END.

      *> -----------------------------------
       003-CONSULT.
           MOVE "CONSULT" TO WS-OP.
           MOVE "ESC TO EXIT" TO WS-STATUS.
           DISPLAY SS-CLS.
       003-CONSULT-LOOP.
           MOVE SPACES TO FILE1-REC.
           DISPLAY SS-RECORD-SCREEN.
           PERFORM 006-READ-CUSTOMER THRU 006-READ-CUSTOMER-END.
           IF FS-CANCEL
               GO 003-CONSULT-END
           END-IF
           IF FS-OK
               DISPLAY SS-DATA
               MOVE "PRESS ENTER" TO WS-ERRMSG
               PERFORM 008-SHOW-ERROR
           END-IF.
           GO 003-CONSULT-LOOP.
       003-CONSULT-END.

      *> -----------------------------------
       004-UPDATE.
           MOVE "UPDATE" TO WS-OP.
           MOVE "ESC TO EXIT" TO WS-STATUS.
           DISPLAY SS-CLS.
       004-UPDATE-LOOP.
           MOVE SPACES TO FILE1-REC.
           DISPLAY SS-RECORD-SCREEN.
           PERFORM 006-READ-CUSTOMER THRU 006-READ-CUSTOMER-END.
           IF FS-CANCEL
               GO TO 004-UPDATE-END
           END-IF
           IF FS-OK
               ACCEPT SS-DATA
               IF COB-CRT-STATUS = COB-SCR-ESC
                   GO 004-UPDATE-LOOP
               END-IF
           ELSE
               GO 004-UPDATE-LOOP
           END-IF
           REWRITE FILE1-REC
                INVALID KEY
                    MOVE "ERROR WRITING RECORD" TO WS-ERRMSG
                    PERFORM 008-SHOW-ERROR
                NOT INVALID KEY
                    CONTINUE
           END-REWRITE.
           GO 004-UPDATE-LOOP.
       004-UPDATE-END.

      *> -----------------------------------
       005-DELETE.
           MOVE "DELETE" TO WS-OP.
           MOVE "ESC TO EXIT" TO WS-STATUS.
           DISPLAY SS-CLS.
           MOVE SPACES TO FILE1-REC.
           DISPLAY SS-RECORD-SCREEN.
           PERFORM 006-READ-CUSTOMER THRU 006-READ-CUSTOMER-END.
           IF FS-CANCEL
               GO 005-DELETE-END
           END-IF
           IF NOT FS-OK
               GO 005-DELETE
           END-IF
           DISPLAY SS-DATA.
           MOVE "N" TO WS-ERROR.
           MOVE "CONFIRM CUSTOMER DELETION (Y/N)?" TO WS-ERRMSG.
           ACCEPT SS-ERROR.
           IF NOT E-YES
               GO 005-DELETE-END
           END-IF
           DELETE FILE1
               INVALID KEY
                   MOVE "ERROR DELETING RECORD" TO WS-ERRMSG
                   PERFORM 008-SHOW-ERROR
           END-DELETE.
       005-DELETE-END.

      *> -----------------------------------
      *> READS CUSTOMER AND SHOWS ERROR MESSAGE IF KEY DOESN'T EXIST
       006-READ-CUSTOMER.
           ACCEPT SS-KEY.
           IF NOT COB-CRT-STATUS = COB-SCR-ESC
               READ FILE1
                   INVALID KEY
                       MOVE "CUSTOMER NOT FOUND" TO WS-ERRMSG
                       PERFORM 008-SHOW-ERROR
               END-READ
           ELSE
               MOVE 99 to FS-STAT
           END-IF.
       006-READ-CUSTOMER-END.

      *> -----------------------------------
      *> OPENS FILES FOR INPUT AND OUTPUT
       007-OPEN-FILES.
           OPEN I-O FILE1
           IF FS-NOT-EXIST THEN
               OPEN OUTPUT FILE1
               CLOSE FILE1
               OPEN I-O FILE1
           END-IF.

      *> -----------------------------------
      *> SHOWS MESSAGE, WAITS FOR ENTER, UPDATES STATUS BAR
       008-SHOW-ERROR.
           DISPLAY SS-ERROR
           ACCEPT SS-ERROR
           DISPLAY SS-STATUS.
