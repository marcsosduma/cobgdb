        >>SOURCE FORMAT IS FREE
IDENTIFICATION DIVISION.
program-id. GCACCEPT9.
*>***********************************************************************************
*> GnuCOBOL
*> Purpose:    SHOWS HOW TO ACCEPT & CHECK A NUMBER WITH DCIMALS & SIGN FROM A FILED ON SCREEN
*> Tectonics:  cobc -x GCACCEPT.COB  (use GnuCOBOL 2.0 or greater)
*> Usage:      GCACCEPT
*> Author:     Eugenio Di Lorenzo - Italia (DILO)
*> License:    Copyright 2017 E.Di Lorenzo - GNU Lesser General Public License, LGPL, 3.0 (or greater)
*> Version:    1.0 2017.03.01
*> Changelog:  1.0 first release.
*>***********************************************************************************
ENVIRONMENT DIVISION.
Configuration Section.
 SPECIAL-NAMES.
   CRT STATUS IS wKeyPressed
   Decimal-Point is Comma.
DATA DIVISION.
WORKING-STORAGE SECTION.
78  K-ESCAPE      VALUE 2005.

01 black   constant as 0.
01 blue    constant as 1.
01 green   constant as 2.
01 cyan    constant as 3.
01 red     constant as 4.
01 magenta constant as 5.
01 yellow  constant as 6.
01 white   constant as 7.
01 pro         pic X value  '_'.
01 wKeyPressed pic 9999.
01 wRetCode    PIC 9999.

*>***************************************************************************************
*> HOW IT WORKS:
*>***************************************************************************************
*> Field9 is your numeric field you have to accept and next you can store for example in a file
*> in this example it is PIC S9(7)V99 = 9 bytes, 7 integers & 2 decimals signed
*> FieldX is the field you have to use in the ACCEPT statement
*> in this example it is 11 bytes = 9 digits + the sign (+ or -) + the comma
*> FieldZ is a working filed to display the number on screen after the ACCEPT (11 bytes)
*> it is same length than FieldX but it is edited

01 Field9   PIC S9(7)V99. *> this is the numeric field (example to be stored in a file)
01 FieldX   PIC X(11).
01 FieldZ   PIC -(7)9,99. *> max edited number is -9999999,99 (11 chars)

*> **************************************************************
*>           P R O C E D U R E   D I V I S I O N
*> **************************************************************
PROCEDURE DIVISION.
  *> sets in order to detect the PgUp, PgDn, PrtSc(screen print), Esc keys,
  set environment 'COB_SCREEN_EXCEPTIONS' TO 'Y'.
  set environment 'COB_SCREEN_ESC'        TO 'Y'.

  display 'GnuCOBOL - HOW TO MANAGE NUMERIC DATA ON SCREEN'
                                     at 0205 with  Background-Color white Foreground-Color blue reverse-video
  display '-----------------------------------------------'
                                     at 0305 with  Background-Color white Foreground-Color blue reverse-video
  display 'Type an amount .....:'    at 0505 with  Background-Color white Foreground-Color blue reverse-video
  display 'signed with 2 decimals'   at 0540 with  Background-Color white Foreground-Color blue reverse-video
  display '12345678901'              at 0627 with  Background-Color white Foreground-Color blue reverse-video
  display '(decimal point is comma)' at 0640 with  Background-Color white Foreground-Color blue reverse-video
  display 'ESC = EXIT'               at 2305 with  Background-Color white Foreground-Color blue reverse-video
  .
Accept-Field.
  accept  FieldX at 0527 with  Background-Color 
        blue Foreground-Color cyan
         update prompt character is pro auto-skip reverse-video

  if wKeyPressed = K-ESCAPE go to End-Program end-if

*> INTRINSIC FUNCTION: TEST-NUMVAL(STRING)
*> --------------------------------------
*> tests the given string for conformance to the rules used by intrinsic FUNCTION NUMVAL.
*> Returns 0 if the value conforms, a character position of the first non conforming character,
*> or the length of the field plus one for other cases such as all spaces.
*> example: you can type +123,44 (is ok) ; -145,,23 (is ko) ; 123- (is ok) etc
  move function test-numval(FieldX) to wRetCode
  display  'RetCode.............:' at 1305 with  Background-Color white Foreground-Color blue reverse-video
  display  wRetCode   at 1334 with  Background-Color white Foreground-Color blue reverse-video

  display ' '
        at 1505 with  Background-Color white Foreground-Color black reverse-video

  if wRetCode > length of FieldX
     *> the field is empty ! program move zero to the field
     move zero to FieldX

     move function numval(FieldX) to Field9 FieldZ
     *> following statement is used to display the amount on screen after the ACCEPT
     move FieldZ to FieldX
     display FieldX                             at 0527 with  Background-Color white Foreground-Color blue reverse-video

     display 'correct format number '           at 1505 with  Background-Color white Foreground-Color green reverse-video
     display '=> empty field ! forced to ZERO.' at 1520 with  Background-Color red   Foreground-Color green reverse-video
     display 'Edited Number.......:'                  at 0905 with Background-Color white Foreground-Color blue reverse-video
     display  FieldZ                                  at 0927 with Background-Color white Foreground-Color blue reverse-video
     display 'Number in memory....:'                  at 1105 with Background-Color white Foreground-Color blue reverse-video
     display  Field9                                  at 1129 with Background-Color white Foreground-Color blue reverse-video
     display 'PIC S9(7)V99 = 9 bytes, 7 int & 2 dec.' at 1140 with Background-Color white Foreground-Color blue reverse-video
  else
     *> field is not empty
     if wRetCode not = ZERO
        *> field is not correct
        display 'incorrect format number '            at 1505 with Background-Color white Foreground-Color red reverse-video
        display '- 1st wrong character at position: ' at 1529 with Background-Color white Foreground-Color red reverse-video
        display wRetCode                              at 1564 with Background-Color white Foreground-Color red reverse-video
     else
        *> field is correct

        move function numval(FieldX) to Field9 FieldZ
        *> following statement is used to display the amount on screen after the ACCEPT
        move FieldZ to FieldX
        display FieldX                             at 0527 with Background-Color white Foreground-Color blue reverse-video

        display 'correct format number '                 at 1505 with Background-Color white Foreground-Color green reverse-video
        display 'Edited Number.......:'                  at 0905 with Background-Color white Foreground-Color blue reverse-video
        display  FieldZ                                  at 0927 with Background-Color white Foreground-Color blue reverse-video
        display 'Number in memory....:'                  at 1105 with Background-Color white Foreground-Color blue reverse-video
        display  Field9                                  at 1129 with Background-Color white Foreground-Color blue reverse-video
        display 'PIC S9(7)V99 = 9 bytes, 7 int & 2 dec.' at 1140 with Background-Color white Foreground-Color blue reverse-video

      end-if
  end-if

 go Accept-Field
 .
End-Program.
 goback.

*>*****************************************************************************************************************************
*> HOW TO MANAGE A SIGE NUMERIC FIELD ON SCREEN (short form whitout demo statements)
*>*****************************************************************************************************************************
*>  display 'Type an amount .....:'  at 0505 with  Background-Color white Foreground-Color blue reverse-video
*>  .
*>Accept-Field.
*>  accept FieldX at 0527 with  Background-Color blue Foreground-Color cyan
*>         update prompt character is pro auto-skip reverse-video
*>  if function test-numval(FieldX) > length of FieldX
*>     move zero to FieldX
*>     move function numval(FieldX) to Field9 FieldZ
*>     move FieldZ to FieldX
*>     display FieldX  at 0527 with Background-Color white Foreground-Color blue reverse-video
*>  else
*>     if function test-numval(FieldX) not = ZERO
*>        display 'incorrect format number ' at 1505 with  Background-Color white Foreground-Color red reverse-video
*>        go to Accept-Field
*>       else
*>        move function numval(FieldX) to Field9 FieldZ
*>        move FieldZ to FieldX
*>        display FieldX at 0527 with Background-Color white Foreground-Color blue reverse-video
*>      end-if
*>  end-if
