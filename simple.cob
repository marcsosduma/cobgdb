       >>SOURCE FORMAT IS FREE
       IDENTIFICATION DIVISION.
       PROGRAM-ID. simple.
       DATA DIVISION.
       WORKING-STORAGE SECTION.
       77 ONE    PICTURE 9999  VALUE 0.
       77 TWO    PICTURE X(100).
       77 wk-key PIC X(1).
       01  PARM.
            05 SIMPLE-ITEM occurs 20 times pic x(20).
       01 black   constant 0. 
       PROCEDURE DIVISION.
      *>I´m beginning ç á é í ó ú Ç ô Â ê with this graphical console mode, it would be very helpful if someone could tell me how to print that character in the coordinate that I want.  
       001-INIT.
       MOVE "Às vezes, não acentuar parece mesmo a solução." TO TWO.
       PERFORM 002-SHOW.
       ACCEPT WK-KEY.
       DISPLAY "THE END".
       001-END-PRG.
           STOP RUN.
           EXIT.
       002-SHOW.
            MOVE 1 TO ONE
            MOVE "TEST" TO SIMPLE-ITEM(1)
            DISPLAY ONE
            DISPLAY TWO.
       END PROGRAM simple.
       