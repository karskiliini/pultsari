1 DIM KEN(80, 28), XXXX(100), YYYY(100), ELO(30), ZZXX(40): RANDOMIZE TIMER
CLS : SCREEN 13: COLOR 32

A$ = "(C) By Mika  Maaranen  "
B$ = "       Jari  Maaranen"

a$ = "       Kimmo Korhonen"
COLOR 10: k=10:gosub vers1:
color 12: k=9: a$ = "       Tero  Maaranen": gosub vers1:
color 14: k=8: a$ = "       Jari  Maaranen": gosub vers1
color 33: k=7: A$ = "(C) By Mika  Maaranen  ": gosub vers1

A$ = " PULTSARI  "
FOR T = 16 TO 24: LOCATE 1, T: P$ = LEFT$(A$, T - 15): COL = INT(RND * 30) + 30: COLOR COL: PRINT RIGHT$(P$, 1)
FOR TT = 1 TO 500: NEXT TT, T

COLOR 10: LOCATE 23, 1: PRINT "28.12.1991 - 14.1.1992 (Valilla paussi)"
LOCATE 10, 1

PRINT : PRINT : COLOR 34: PRINT "Press any key to continue"
gosub pause:

rem --- yksi lisäefekti vanhalla koodilla (versio 1.01) ---
COLOR 33:cls
a$="(C) Versio 1.01 By Mika Maaranen": k=23:gosub vers1: color 12
a$="Pulsu$oftware 5.4.2022 (TM)": k=21:gosub vers1:color 14
a$="Better late than never, yes ?": k=19:gosub vers1:


goto 2
pause:
      A$ = INKEY$: IF A$ = "" THEN GOTO pause
return
vers1:
      FOR T = 1 TO LEN(A$): YYYY(T) = INT(RND * 10): YYYY(T) = YYYY(T) * -1: NEXT
      FOR T = 1 TO 35: FOR TT = 1 TO len(a$): IF YYYY(TT) > 0 AND YYYY(TT) < 25 THEN LOCATE YYYY(TT), TT + 2: PRINT " "
      IF YYYY(TT) < k THEN YYYY(TT) = YYYY(TT) + 1:
      IF YYYY(TT) > 0 and YYYY(t)<25 THEN LOCATE YYYY(TT), TT + 2: P$ = LEFT$(A$, TT): PRINT RIGHT$(P$, 1)
      NEXT TT, T
return

2 A$ = INKEY$: IF A$ = "" THEN GOTO 2
CONMAN = 0
CLS : RANDOMIZE TIMER
SCREEN 12:
FOR T = 13 TO 79: LOCATE 2, T: PRINT CHR$(205): LOCATE 24, T: PRINT CHR$(205): NEXT
FOR T = 2 TO 24: LOCATE T, 13: PRINT CHR$(186): LOCATE T, 80: PRINT CHR$(186): NEXT
LOCATE 2, 13: PRINT CHR$(201): LOCATE 2, 80: PRINT CHR$(187)
LOCATE 24, 13: PRINT CHR$(200): LOCATE 24, 80: PRINT CHR$(188): MONEY = 0: VOIMA = 10: PROM = 2: WALK = 0: BEER = 1: LON = 0: MAK = 0: BA = 0: OM = 1: PU = 0: KA = 0: KI = 2: KNI = 0: KET = 0: MAK = 1: LEV = 1: VUORO = 0
GOTO ALKU
TULOSTUS: REM ******** TULOSTUS *********

LOCATE 28, 1: COLOR 10: PRINT "RAHAA:"; MONEY:
LOCATE 28, 12: PRINT "VOIMA:"; VOIMA
LOCATE 28, 24: PRINT "PROMILLE:"; PROM / 10
LOCATE 28, 40: PRINT "KÄPPÄILY:"; WALK
LOCATE 28, 56: PRINT "LEVEL:"; LEV

LOCATE 3, 1: PRINT "KALJAA ="; BEER
             PRINT "LONKKA ="; LON
             PRINT "LENKKI ="; MAK
             PRINT "KETJUT ="; KET
             PRINT "VEITET ="; KNI
             PRINT "KIVET  ="; KI
             PRINT "PAMPUT ="; PA
             PRINT "BOOTSIT="; KA
             PRINT "KALAT  ="; PU
             PRINT "OMPPO  ="; OM
             PRINT "BANSKU ="; BA
RETURN
PIIRRAKAUPPA: REM --- TÄSSÄ PIIRRETÄÄN KENTTÄÄN KUULUVA KAUPPARAKENNUS ---
      IF VUORO = 1 THEN
         LOCATE 3, 14: PRINT "ALKO  "; CHR$(219)
         LOCATE 4, 14: PRINT "      "; CHR$(219)
         FOR T = 14 TO 20: KEN(T, 5) = 2: LOCATE 5, T:
         PRINT CHR$(219): NEXT: KEN(20, 4) = 2: KEN(20, 3) = 2:
         KEN(16, 5) = 18: LOCATE 5, 16: PRINT CHR$(177)
         END IF
      IF VUORO = 2 THEN
         LOCATE 3, 14: PRINT "K-RAUTA"; CHR$(219):
         LOCATE 4, 14: PRINT "       "; CHR$(219):
         FOR T = 14 TO 21: LOCATE 5, T: PRINT CHR$(219):
         KEN(T, 5) = 19: NEXT: KEN(21, 3) = 19: KEN(21, 4) = 19:
         KEN(17, 5) = 20: LOCATE 5, 17: PRINT CHR$(176)
         END IF

RETURN



KENT: REM ************ UUSI KENTTA ****************
      FOR T = 2 TO 24: FOR TT = 1 TO 79: KEN(TT, T) = 0: NEXT TT, T
      LOCATE 28, 56: PRINT "LEVEL:"; LEV
      FOR T = 2 TO 24: LOCATE T, 13: PRINT TAB(80); " ": KEN(13, T) = 1: KEN(80, T) = 1: NEXT
      VUORO = VUORO + 1: IF VUORO > 2 THEN VUORO = 1
     
      RANDOMIZE TIMER
      for t=1 to 13
      XXX = INT(RND * 64) + 14: YYY = INT(RND * 20) + 2: LOCATE YYY, XXX: ran=int(rnd*100)

      if t=1 and RAN < 50 THEN PRINT "%": KEN(XXX, YYY) = 4
      if t=2 and RAN < 25 THEN PRINT "(": KEN(XXX, YYY) = 3
      if t=3 and RAN < 10 THEN PRINT CHR$(225): KEN(XXX, YYY) = 5
      if t=4 and RAN < 20 THEN print "\": KEN(XXX, YYY) = 6
      if T=5 and RAN < 55 THEN PRINT ".": KEN(XXX, YYY) = 7
      IF T=6 AND RAN < 15 THEN PRINT "f": KEN(XXX, YYY) = 8
      IF T=7 AND RAN < 5 THEN PRINT CHR$(157): KEN(XXX, YYY) = 9
      IF T=8 AND RAN < 10 THEN PRINT "o": KEN(XXX, YYY) = 10
      IF T=9 AND RAN < 5 THEN PRINT ")": KEN(XXX, YYY) = 11
      IF T=10 AND RAN < 45 THEN PRINT "&": KEN(XXX, YYY) = 16
      IF T=11 AND RAN < 80 THEN PRINT "?": KEN(XXX, YYY) = 17
      IF T=12 AND RAN < 90 AND KEN(XXX,YYY)=0 THEN PRINT "7": KEN(XXX, YYY) = 28: 
      IF T=13 AND RAN < 9 THEN PRINT "$": KEN(XXX, YYY) = 99: REM RAHAA!!
      NEXT t

      LOCATE 23, 79: PRINT "^": KEN(79, 23) = 12
      COLOR 15
      
      FOR T = 13 TO 79: LOCATE 2, T: PRINT CHR$(205): LOCATE 24, T: PRINT CHR$(205): KEN(T, 2) = 1: KEN(T, 24) = 1: NEXT
      FOR T = 2 TO 24: LOCATE T, 13: PRINT CHR$(186): LOCATE T, 80: PRINT CHR$(186): NEXT
      LOCATE 2, 13: PRINT CHR$(201): LOCATE 2, 80: PRINT CHR$(187)
      LOCATE 24, 13: PRINT CHR$(200): LOCATE 24, 80: PRINT CHR$(188)
         
      GOSUB PIIRRAKAUPPA:
     
      PIS = PIS + 1: IF PIS > 20 THEN PIS = 20
      FOR T = 1 TO PIS
PIS:  XXX = INT(RND * 64) + 14: YYY = INT(RND * 11) + 6: IF KEN(XXX, YYY) <> 0 THEN GOTO PIS
      LOCATE YYY, XXX: COLOR 12: PRINT CHR$(248): KEN(XXX, YYY) = 13
      NEXT
   
      RR = INT(RND * 9) + 1: POL = RR: FOR T = 1 TO POL
PPP:  XX = INT(RND * 64) + 14: YY = INT(RND * 15) + 6: IF KEN(XX, YY) <> 0 THEN GOTO PPP
      LOCATE YY, XX: COLOR 10: PRINT "C": KEN(XX, YY) = 14: XXXX(T) = XX: YYYY(T) = YY: ELO(T) = 1
      NEXT: XX = 0: YY = 0
      LOCATE 1, 1: PRINT TAB(80);

      IF LEV > 2 THEN RAN = INT(RND * 100): IF RAN < 60 THEN XXX = INT(RND * 64) + 14: YYY = INT(RND * 12) + 6: IF KEN(XXX, YYY) = 0 THEN VX = XXX: VY = YYY: VELO = 1: LOCATE VY, VX: PRINT "V"
MMUU: XXX = INT(RND * 64) + 14: YYY = INT(RND * 12) + 5: IF KEN(XXX, YYY) <> 0 THEN GOTO MMUU ELSE KEN(XXX, YYY) = 22: COLOR 10: LOCATE YYY, XXX: PRINT "M"
      REM *** DIVARI TAI ISKU ***
      RAN = 0: COLOR 15
      IF LEV > 4 THEN RAN = INT(RND * 100): IF RAN < 50 THEN RAN = 1 ELSE RAN = 2
      IF RAN = 1 THEN DIVARI = 1: ISKU = 0
      IF RAN = 2 THEN DIVARI = 0: ISKU = 1
      IF DIVARI = 1 THEN FOR T = 14 TO 25: LOCATE 20, T: KEN(T, 20) = 24: PRINT "�": NEXT: FOR T = 20 TO 23: LOCATE T, 25: PRINT "�": KEN(25, T) = 24: NEXT: LOCATE 20, 20: PRINT "�": KEN(20, 20) = 25: LOCATE 23, 15: PRINT "DIVARI"
      IF ISKU = 1 THEN FOR T = 14 TO 25: LOCATE 20, T: KEN(T, 20) = 26: PRINT "�": NEXT: FOR T = 20 TO 23: LOCATE T, 25: PRINT "�": KEN(25, T) = 26: NEXT: LOCATE 20, 20: PRINT "�": KEN(20, 20) = 27: LOCATE 23, 15: PRINT "ISKU"
      REM *** POLIISIASEMA+VANKILA ***
      RAN = 0
      RAN = INT(RND * 100): IF RAN < 50 THEN RAN = 0 ELSE RAN = 1
      IF LEV > 6 AND RAN = 1 THEN FOR T = 40 TO 50: LOCATE 20, T: KEN(T, 20) = 29: PRINT "�": LOCATE 20, T + 12: PRINT "�": KEN(T + 12, 20) = 29: NEXT: FOR T = 20 TO 23: KEN(40, T) = 29: KEN(50, T) = 29: KEN(52, T) = 29: KEN(62, T) = 29: LOCATE T,  _
62: PRINT "�": LOCATE T, 52: PRINT "�": LOCATE T, 50: PRINT "�": LOCATE T, 40: PRINT "�": NEXT: LOCATE 20, 45: PRINT "�": KEN(45, 20) = 30: LOCATE 23, 41: PRINT "POLIISI": LOCATE 23, 53: PRINT "PUTKA": PUTKA = 1: LOCATE 20, 57: PRINT "�": KEN(57, 20 _
) = 31
      RAN = INT(RND * 100): IF RAN < 60 THEN YKX = 18: YKY = 12: IF KEN(YKX, YKY) = 0 THEN KEN(YKX, YKY) = 32: LOCATE YKY, YKX: PRINT "Y": YKA = 1
     
      REM *** ASEMA ***
      IF LEV > 8 THEN FOR T = 65 TO 77: LOCATE 22, T: PRINT "�": KEN(T, 22) = 29: NEXT: LOCATE 23, 70: PRINT "�ASEMA��": KEN(70, 23) = 29: KEN(77, 23) = 29: ASEM = 1: SPOW = 7



RETURN

HEITA: REM ****************** HEITTO ********************
       LOCATE 1, 1: INPUT "ANNA NUMERO .."; AA
       IF AA = 1 THEN BEER = BEER - 1: IF BEER < 0 THEN BEER = 0: RETURN ELSE GOTO HEIT
       IF AA = 2 THEN LON = LON - 1: IF LON < 0 THEN LON = 0: RETURN ELSE GOTO HEIT
       IF AA = 3 THEN MAK = MAK - 1: IF MAK < 0 THEN MAK = 0: RETURN ELSE GOTO HEIT
       IF AA = 4 THEN KET = KET - 1: IF KET < 0 THEN KET = 0: RETURN ELSE GOTO HEIT
       IF AA = 5 THEN KNI = KNI - 1: IF KNI < 0 THEN KNI = 0: RETURN ELSE GOTO HEIT
       IF AA = 6 THEN KI = KI - 1: IF KI < 0 THEN KI = 0: RETURN ELSE GOTO HEIT
       IF AA = 7 THEN PA = PA - 1: IF PA < 0 THEN PA = 0: RETURN ELSE GOTO HEIT
       IF AA = 8 THEN KA = KA - 1: IF KA < 0 THEN KA = 0: RETURN ELSE GOTO HEIT
       IF AA = 9 THEN PU = PU - 1: IF PU < 0 THEN PU = 1: RETURN ELSE GOTO HEIT
       IF AA = 10 THEN OM = OM - 1: IF OM < 0 THEN OM = 0: RETURN ELSE GOTO HEIT
       IF AA = 11 THEN BA = BA - 1: IF BA < 0 THEN BA = 0: RETURN ELSE GOTO HEIT
       RETURN
HEIT: LOCATE 1, 1: PRINT TAB(80);
      LOCATE 1, 1: COLOR 7: PRINT "Suunta ...??              ": XX = 0: YY = 0
HEHE: A$ = INKEY$: IF A$ = "8" THEN YY = -1
      IF A$ = "6" THEN XX = 1
      IF A$ = "5" THEN YY = 1
      IF A$ = "4" THEN XX = -1
      IF A$ <> "8" AND A$ <> "6" AND A$ <> "5" AND A$ <> "4" THEN GOTO HEHE
      GOSUB TULOSTUS: XXX = X + XX: YYY = Y + YY: LOCATE 1, 1: PRINT "                                               "
HEI:
      IF KEN(XXX, YYY) <> 0 THEN XX = XX * -1: YY = YY * -1: GOTO HE
      LOCATE YYY, XXX: PRINT "*": FOR T = 1 TO 250: NEXT
      LOCATE YYY, XXX: PRINT " "
      XXX = XXX + XX: YYY = YYY + YY:
      GOTO HEI
HE:   IF KEN(XXX, YYY) = 14 THEN LOCATE 1, 1: COLOR 12: PRINT "POLLARI KUPSAHTI !!!": KEN(XXX, YYY) = 15: LOCATE YYY, XXX: PRINT "!": HY = 1: GOSUB ELOTON: ALL = ALL + 1
      IF KEN(XXX, YYY) = 1 OR KEN(XXX, YYY) = 2 THEN LOCATE 1, 1: COLOR 10: PRINT "Kops !!!": XX = XX * -1: YY = YY * -1: XXX = XXX + XX: YYY = YYY + YY: KEN(XXX, YYY) = AA: GOSUB TARK
      IF KEN(XXX, YYY) = 21 THEN RAN = INT(RND * 100): IF RAN < 40 THEN LOCATE 1, 1: PRINT "OHI !!!": RETURN ELSE LOCATE 1, 1: PRINT "Osuit varkaaseen, joka kupsahti.": KEN(XXX, YYY) = 23: LOCATE YYY, XXX: PRINT "$": VELO = 0: ALL = ALL + 1
      IF KEN(XXX, YYY) = 22 THEN RAN = INT(RND * 100): IF RAN < 50 THEN LOCATE 1, 1: PRINT "Mummo vaistaa !!": RETURN ELSE LOCATE 1, 1: PRINT "Kolkkasit juuri mumumummurun": KEN(XXX, YYY) = 23: LOCATE YYY, XXX: PRINT "$": ALL = ALL + 1: KM = KM + 1
      IF KEN(XXX, YYY) = 33 THEN RAN = INT(RND * 100): IF RAN < 40 THEN LOCATE 1, 1: PRINT "Vankikarkuri vaistaa.": RETURN ELSE LOCATE 1, 1: PRINT "Heittosi osuu vankikarkuria otsaan. Eh...se nayttaa vihastuvan.": KONP = KONP - 1: IF KONP < 2 THEN  _
LOCATE 1, 1: PRINT "Vankikarkuri pokertyy heittosi ansiosta maahan...         ": CONMAN = 0: KEN(XXX, YYY) = 0: IF KON < 1 THEN LOCATE KONY, KONX: PRINT " ": ALL = ALL + 1:
      IF KEN(XXX, YYY) = 34 THEN LOCATE 1, 1: PRINT "Heittosi ossuupi skinukkelia vyon alle. Se koskee tuntuvasti !!!": SPOW = SPOW - 2: GOSUB TULOSTUS: IF SPOW < 1 THEN LOCATE SKIY, SKIX: PRINT " ": ALL = ALL + 1: RETURN ELSE RETURN

      RETURN
TARK: REM *********** ETSITaaN, MIK'a' TAVARA OSUI SEINaaN ***************
      LOCATE YYY, XXX
      IF AA = 1 THEN PRINT "%"
      IF AA = 2 THEN PRINT "("
      IF AA = 3 THEN PRINT "&"
      IF AA = 4 THEN PRINT "f"
      IF AA = 5 THEN PRINT "\"
      IF AA = 6 THEN PRINT "!"
      IF AA = 7 THEN PRINT "7"
      IF AA = 8 THEN PRINT "F"
      IF AA = 9 THEN PRINT "o"
      IF AA = 10 THEN PRINT ")"
      RETURN
     
ELOTON: REM ***** ETSITaaN KUOLLUT POLLARI *****
        FOR T = 1 TO POL
        IF XXXX(T) = XXX AND YYYY(T) = YYY THEN ELO(T) = 0
        NEXT
        RETURN

GLU:  REM ************** JUONTI ****************
      IF BEER < 1 AND LON < 1 THEN LOCATE 1, 1: COLOR 12: PRINT "Ei oo ehtaa tavaraa.": RETURN
      IF BEER > 0 AND LON > 0 THEN LOCATE 1, 1: PRINT "Kumpaakos horaat,kaljaa vai lonkkaa (1,2) ?": GOSUB JOKU: RETURN
      IF BEER < 1 THEN LON = LON - 1: GOSUB TULOSTUS: LOCATE 1, 1: COLOR 10: PRINT "Glub glub .. gulb gulu... ooorbbbs.": PROM = PROM + 4: RETURN
      IF LON < 1 THEN BEER = BEER - 1: GOSUB TULOSTUS: LOCATE 1, 1: COLOR 10: PRINT "<Tsuuuhss> .. gluuuub gluub glub ... ooorrroyyh hh hh": PROM = PROM + 3: RETURN
      STOP
JOKU: A$ = INKEY$: IF A$ <> "1" AND A$ <> "2" THEN GOTO JOKU
      IF A$ = "1" THEN BEER = BEER - 1: PROM = PROM + 2
      IF A$ = "2" THEN LON = LON - 1: PROM = PROM + 4
      LOCATE 1, 1: COLOR 10: PRINT " Glub glub glub glub ..... Burb !!!                     "
      GOSUB TULOSTUS
      RETURN
     
ALKU: REM ************* PELI ***************
      GOSUB TULOSTUS
      GOSUB KENT
      X = 17: Y = 12
PELI: A$ = INKEY$
NAP:  LOCATE Y, X: PRINT " "
      LOCATE 1, 1: PRINT TAB(80);
      IF A$ = "4" THEN WALK = WALK + 1: X = X - 1: IF KEN(X, Y) <> 0 THEN XX = 1: GOSUB REAGOINTI
      IF A$ = "6" THEN WALK = WALK + 1: X = X + 1: IF KEN(X, Y) <> 0 THEN XX = -1: GOSUB REAGOINTI
      IF A$ = "8" THEN WALK = WALK + 1: Y = Y - 1: IF KEN(X, Y) <> 0 THEN YY = 1: GOSUB REAGOINTI
      IF A$ = "5" THEN WALK = WALK + 1: Y = Y + 1: IF KEN(X, Y) <> 0 THEN YY = -1: GOSUB REAGOINTI
      if a$="z" or a$="Z" then end
      COLOR 14: LOCATE Y, X: PRINT CHR$(234)
      IF A$ = "H" OR A$ = "h" THEN GOSUB HEITA
      IF A$ = "Q" OR A$ = "q" THEN GOSUB GLU
      IF A$ = "E" OR A$ = "e" THEN GOSUB EAT
      IF PROM > 23 THEN LOCATE 26, 1: COLOR 14: PRINT "Horjahtelet.": GOSUB HORJUNTA ELSE LOCATE 26, 1: PRINT "              "
      IF PROM < 4 THEN LOCATE 26, 30: COLOR 14: PRINT "Sua janottaa !!!" ELSE LOCATE 26, 30: PRINT "                 "
      IF PROM > 28 THEN RAN = INT(RND * 100): IF RAN < 30 THEN LOCATE 25, 1: PRINT "Oksennat !!": VOIMA = VOIMA - 1: GOSUB SONTA: GOSUB TULOSTUS ELSE LOCATE 25, 1: PRINT "            "
      IF PROM < 28 THEN LOCATE 25, 1: PRINT "               "
      IF PROM > 34 THEN LOCATE 1, 1: COLOR 14: PRINT "HIC ! ... Kompastut omiin jalkoihisi...huomaat ettet el� ennee...": GOTO GOVER

      GOSUB POLSKI
      POV = POV + 1: IF POV > 260 THEN POV = 1: PROM = PROM - 1: GOSUB TULOSTUS: IF PROM < .1 THEN LOCATE 1, 1: COLOR 14: PRINT "Yhtakkii tajuat kraapuliuksen tulevan. Et jaksa jatkaa...": GOTO GOVER
      IF PROM < 2 THEN COLOR 14: LOCATE 27, 1: PRINT "Voit pahoin !!" ELSE LOCATE 27, 1: PRINT "              "
      VOI = VOI + 1: IF VOI > 50 THEN VOI = 1: VOIMA = VOIMA + 1: GOSUB TULOSTUS
      IF HY = 1 THEN GOSUB POLLAR
      IF VELO = 1 THEN GOSUB VARAS
      IF BEER > 0 AND YKA = 1 THEN GOSUB YKA
      IF PUTKA = 1 AND CONMAN = 0 THEN PUTKIS = PUTKIS + 1: IF PUTKIS > 50 THEN PUTKIS = 0: CONMAN = 1: KONX = 57: KONY = 19: KEN(KONX, KONY) = 34: KONP = 4
      IF CONMAN = 1 THEN GOSUB KONNA
      IF SPOW < 1 THEN SPOW = 7: SKIH = 0: KEN(SKIX, SKIY) = 0: SKIX = 0: SKIY = 0
      IF SKIH = 0 AND ASEM = 1 THEN ASE = ASE + 1: IF ASE > 30 THEN ASE = 0: SKIH = 1: SKI = 5: KEN(69, 23) = 34: SKIX = 69: SKIY = 23: LOCATE SKIY, SKIX: PRINT "O"
      IF SKIH = 1 THEN GOSUB SKINHEAD
      IF VANK = 1 AND X = KONX OR VANK = 1 AND Y = KONY THEN GOSUB VANHEITTO
      

NAAP: A$ = INKEY$:
      IF VOIMA < 1 THEN LOCATE 1, 1: COLOR 14: PRINT "Terveytesi pettaa... kemahdat tantereeseen pitkaksesi .........": GOTO GOVER

      IF A$ <> "" THEN GOTO NAP
      TULO = 0
      LOCATE 28, 40: COLOR 10: PRINT "KaPPaILY:"; WALK
      IF NEX = 1 THEN NEX = 0:  LEV = LEV + 1: SIP = 0: GOTO ALKU

GOTO NAAP:

EAT: REM *********** SY�P�TTELY ***************
     COLOR 10
     IF MAK < 1 AND OM < 1 AND BA < 1 AND PU < 1 THEN LOCATE 1, 1: PRINT "Ei syotavaa.": RETURN
     IF MAK < 1 AND OM < 1 AND PU < 1 THEN LOCATE 1, 1: BA = BA - 1: RAN = INT(RND * 100): IF RAN < 50 THEN PRINT "Ei vaikutusta.": RETURN ELSE PRINT "Bansku teki teraa !!!": VOIMA = VOIMA + 1: GOSUB TULOSTUS: RETURN
     IF MAK < 1 AND PU < 1 AND BA < 1 THEN LOCATE 1, 1: OM = OM - 1: RAN = INT(RND * 100): IF RAN < 50 THEN PRINT "Omppo oli pilaantunut,oksennat.": VOIMA = VOIMA - 1: RETURN ELSE PRINT "Omena oli herkullinen !!!": VOIMA = VOIMA + 2: GOSUB TULOSTUS _
: RETURN
     IF MAK < 1 AND OM < 1 AND BA < 1 THEN LOCATE 1, 1: PU = PU - 1: RAN = INT(RND * 100): IF RAN < 70 THEN PRINT "Syomasi kala sisalsi matoja...": VOIMA = VOIMA - 2: GOSUB TULOSTUS: RETURN ELSE PRINT "Kala antaa sinulle uutta puhtia !!!": VOIMA =  _
VOIMA + 4: GOSUB TULOSTUS: RETURN
     IF OM < 1 AND BA < 1 AND BA < 1 THEN MAK = MAK - 1: LOCATE 1, 1: PRINT "Oispa edes kossua kaveriksi.": VOIMA = VOIMA + 1: GOSUB TULOSTUS: RETURN
     LOCATE 1, 1: PRINT "Mitas...1=makkara 2=kala 3=omppo 4=bansku ???"
AAA: A$ = INKEY$: IF A$ <> "1" AND A$ <> "2" AND A$ <> "3" AND A$ <> "4" THEN GOTO AAA
     LOCATE 1, 1: PRINT TAB(80);
     LOCATE 1, 1
     IF A$ = "1" THEN IF MAK < 1 THEN PRINT "Ei ole niita.": RETURN ELSE PRINT "Mamamamakkaraa.": VOIMA = VOIMA + 1: MAK = MAK - 1: GOSUB TULOSTUS: RETURN
     IF A$ = "2" THEN IF PU < 1 THEN PRINT "Kalat loppu.": RETURN ELSE RAN = INT(RND * 100): IF RAN < 70 THEN PRINT "Joku oli piilottanut papanoita kalaasi !": VOIMA = VOIMA - 2: GOSUB TULOSTUS: RETURN ELSE PRINT "Kaloreja kalasta.": VOIMA = VOIMA + _
 4: PU = PU - 1: GOSUB TULOSTUS: RETURN
     IF A$ = "3" THEN IF OM < 1 THEN PRINT "EE OO OMMPPOJA": RETURN ELSE RAN = INT(RND * 100): IF RAN < 50 THEN VOIMA = VOIMA - 1: PRINT "Joku oli k�ytellyt keski keppi��n omppoosi.": GOSUB TULOSTUS: RETURN ELSE PRINT "�rb.": VOIMA = VOIMA + 2: OM  _
= OM - 1: GOSUB TULOSTUS: RETURN
     IF A$ = "4" THEN IF BA < 1 THEN PRINT "Bailu ilman banskuu ?": RETURN ELSE RAN = INT(RND * 100): IF RAN < 50 THEN PRINT "Bansku ei vaikuttanut.": RETURN ELSE PRINT "Babababananiii": VOIMA = VOIMA + 1: BA = BA - 1: GOSUB TULOSTUS: RETURN
     STOP
REAGOINTI: COLOR 12: TULO = 1
    IF KEN(X, Y) = 1 THEN LOCATE 1, 2: PRINT "Ei karata pelialueelta !!": WALK = WALK - 1: X = X + XX: XX = 0: Y = Y + YY: YY = 0: LOCATE Y, X: PRINT CHR$(234)
    IF KEN(X, Y) = 2 THEN LOCATE 1, 2: PRINT "Isket paasi alkon seinaan. Mielenkiintoista.": WALK = WALK - 1: X = X + XX: XX = 0: Y = Y + YY: YY = 0
    IF KEN(X, Y) = 3 THEN LOCATE 1, 2: PRINT "Nam, lenkki makkaraa.": MAK = MAK + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 4 THEN LOCATE 1, 2: PRINT "Katohhan, miehen mittainen.": BEER = BEER + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 5 THEN LOCATE 1, 2: PRINT "Kutkuta pollaria riimuilla.": KET = KET + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 6 THEN LOCATE 1, 2: PRINT "Teroita veitsesi mummoihin.": KNI = KNI + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 7 THEN LOCATE 1, 2: PRINT "Loysit kiven.Onnitteluni.": KI = KI + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 8 THEN LOCATE 1, 2: PRINT "Camel boots,miehille,jotka kusevat omille jaloileen.": KEN(X, Y) = 0: KA = KA + 1
    IF KEN(X, Y) = 9 THEN LOCATE 1, 2: PRINT "Kalasi lemuaa.": PU = PU + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 10 THEN LOCATE 1, 2: PRINT "Omppo.Sen voi syoda, jos uskaltaa.": OM = OM + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 11 THEN LOCATE 1, 2: PRINT "Loysit banaanin. Kokeileppas syoda se.": BA = BA + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 12 THEN IF PIS > SIP THEN LOCATE 1, 2: PRINT "Keraa pisteet ensin....": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 13 THEN SIP = SIP + 1: KEN(X, Y) = 0: LOCATE 1, 1: PRINT "Bona"
    IF KEN(X, Y) = 15 THEN LOCATE 1, 2: PRINT "Pamppu. Mahtaakohan t�m� olla kadoksissa joltain poltsulta ?": PA = PA + 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 14 THEN LOCATE 1, 2: PRINT "Poltsu pamputtaa ,ja se v�s�hti.": KEN(X, Y) = 15: HY = 1:  GOSUB KUOL: X = X + XX: Y = Y + YY: ALL = ALL + 1
    IF KEN(X, Y) = 16 THEN LOCATE 1, 2: PRINT "Kappas, lonkan mittainen !!!": KEN(X, Y) = 0: LON = LON + 1: GOSUB TULOSTUS
    IF KEN(X, Y) = 17 THEN LOCATE 1, 2: GOSUB APU: KEN(X, Y) = 0:
    IF KEN(X, Y) = 18 THEN GOSUB ALKO: RETURN
    IF KEN(X, Y) = 19 THEN LOCATE 1, 2: PRINT "K-RAUDAN  seina on vahvempaa tekoa,kuin paasi.Et paassyt sisaan.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 20 THEN LOCATE 1, 2: PRINT "K-RAUTAA ei ole viela ohjelmoitu loppuun. Kokeile uudempaa versiota.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 23 THEN LOCATE 1, 2: RAN = INT(RND * 30): PRINT "Kukkarosi lihavoituu "; RAN; " markalla.": KEN(X, Y) = 0: MONEY = MONEY + RAN
    IF KEN(X, Y) = 22 THEN LOCATE 1, 2: GOSUB MUMMO: RETURN
    IF KEN(X, Y) = 24 THEN LOCATE 1, 2: PRINT "Divarin sein� on vankkumaton,vaikka kuinka yrit�t k�vell� sit� p�in.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 25 THEN GOSUB DIVARI
    IF KEN(X, Y) = 26 THEN LOCATE 1, 2: PRINT "Iskun seina on iskunkestava,et paassyt sisaan.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 27 THEN LOCATE 1, 2: PRINT "Emme myy juopoille.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 28 THEN LOCATE 1, 2: PRINT "Huomaat sen liian myohaan... liukastut paskaan.": VOIMA = VOIMA - 1: KEN(X, Y) = 0
    IF KEN(X, Y) = 29 THEN LOCATE 1, 2: PRINT "Lyot paatasi seinaan.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 30 THEN LOCATE 1, 2: PRINT "Putkaan otetaan kylla juoppoja, tuletko (K/E) ?": GOSUB PUTKA
    IF KEN(X, Y) = 32 THEN LOCATE 1, 2: IF BEER > 0 THEN PRINT "Huitaiset ykaa,joka pollii sulta yhen kaljan.": YKA = 0: KEN(YKX, YKY) = 0: LOCATE YKY, YKX: PRINT " ": BEER = BEER - 1 ELSE PRINT "Matat ykaa sikana naamaan.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 31 THEN LOCATE 1, 2: PRINT "Jos haluat putkaan,mene poliisiaseman ovesta.": X = X + XX: Y = Y + YY
    IF KEN(X, Y) = 33 THEN LOCATE 1, 2: PRINT "Hakkaat hullun lailla vankikarkuria. ": X = X + XX: Y = Y + YY: KONP = KONP - 1: IF KONP < 1 THEN CONMAN = 0: LOCATE 1, 1: PRINT "Hakkasit vankikarkurin kuoliaaksi...                                 ":  _
KEN(KONX, KONY) = 0: LOCATE KONY, KONX: PRINT " ": ALL = ALL + 1
    IF KEN(X, Y) = 34 THEN LOCATE 1, 2: PRINT "Pusket SHITHEADia naamaan,mutta sehan murjoo myos sua."; : X = X + XX: Y = Y + YY: SPOW = SPOW - 1: RAN = INT(RND * 100): IF RAN < 50 THEN PRINT "Nakkasit �ij�� munille !!!": SPOW = SPOW - 1: IF SPOW < 1 THEN  _
LOCATE SKIY, SKIX: PRINT " ": ALL = ALL + 1

      LOCATE 1,2: 
REM **** UUSIA EFEKTEJÄ, VERSIO 1.01 *****
    IF KEN(X,Y)=99 THEN PRINT"Jukoliste - nyt iski tuurilla, nappaat maasta satkun taskuusi!!!!": money=money+100



XX = 0: YY = 0: IF PIS = SIP AND KEN(X, Y) = 12 THEN NEX = 1: HY = 0: LOCATE 1, 1: PRINT "Seuraavaan kenttaan...": VELO = 0: CONMAN = 0: PUTKIS = 0: YKA = 0: PUTKA = 0
GOSUB TULOSTUS
RETURN
KUOL:
      FOR T = 1 TO POL
      IF XXXX(T) = X AND YYYY(T) = Y THEN ELO(T) = 0
      NEXT: R = INT(RND * 3) + 1: VOIMA = VOIMA - R
      COLOR 10: LOCATE Y, X: PRINT "!"
      RETURN

POLLAR: FOR G = 1 TO POL
        IF ELO(G) > 0 THEN GOSUB POLLA
        NEXT G
        RETURN
        
POLLA:
      
       LOCATE YYYY(G), XXXX(G): PRINT " ": KEN(XXXX(G), YYYY(G)) = 0
       IF X + 1 < XXXX(G) THEN XXXX(G) = XXXX(G) - 1: IF KEN(XXXX(G), YYYY(G)) <> 0 THEN XXXX(G) = XXXX(G) + 1: LOCATE 26, 40
       IF X - 1 > XXXX(G) THEN XXXX(G) = XXXX(G) + 1: IF KEN(XXXX(G), YYYY(G)) <> 0 THEN XXXX(G) = XXXX(G) - 1: LOCATE 26, 40
       IF Y - 1 > YYYY(G) THEN YYYY(G) = YYYY(G) + 1: IF KEN(XXXX(G), YYYY(G)) <> 0 THEN YYYY(G) = YYYY(G) - 1: LOCATE 26, 40
       IF Y + 1 < YYYY(G) THEN YYYY(G) = YYYY(G) - 1: IF KEN(XXXX(G), YYYY(G)) <> 0 THEN YYYY(G) = YYYY(G) + 1: LOCATE 26, 40
       LOCATE YYYY(G), XXXX(G): COLOR 10: PRINT "C": KEN(XXXX(G), YYYY(G)) = 14
      
RETURN
POLL: REM ********* POLLARIN HAKKAUS **********
      COLOR 14
      IF HHH = 1 THEN LOCATE 1, 1: IF TULO < 1 THEN PRINT "Pollari varotteleepi sua,ettei kantsi retteloida.": RETURN ELSE RETURN
      IF TULO < 1 THEN LOCATE 1, 1: PRINT "Pollari pamputtaa !!!"
      R = INT(RND * 3) + 1: VOIMA = VOIMA - R: GOSUB TULOSTUS
RETURN
       
GOVER: A$ = ""
GOOVER: A$ = INKEY$: IF A$ = "" THEN GOTO GOOVER
        CLS
        PRINT "Hakkaamasi mummelit  :"; KM
        PRINT "Kaikki hakatut oliot :"; ALL
        PRINT "-----------------------------"
        PRINT "Yhteispistetilanteesi:"; KM + ALL
        FOR T = 1 TO 5: PRINT : NEXT
        PRINT "Kiitos PULTSARIn pelaamisesta !"
        PRINT "Uuvvestaan (k/e)?"
GOO: A$ = INKEY$: IF A$ = "K" OR A$ = "k" THEN GOTO 1
     IF A$ = "E" OR A$ = "e" THEN END
     GOTO GOO

APU: REM ************** TAIVAALLISET TEKSTIT *****************
     RANDOMIZE TIMER
     RAN = INT(RND * 36) + 1
     RESTORE
     FOR T = 1 TO RAN
     READ D$
     NEXT
     PRINT "Ilma alkaa vareilla ja kuulet taivaallisen aanen :       <LISaa>"
AP:  AA$ = INKEY$: IF AA$ = "" THEN GOTO AP
     LOCATE 1, 1: PRINT TAB(80); : LOCATE 1, 1
     PRINT D$
     RETURN

     DATA "Mummoilla on rahaa."
     DATA "Poliisit on kayny punttisalilla."
     DATA "Alcossa myydaan tavaraa."
     DATA "Varkaille kannattaa antaa pamppua."
     DATA "Sven Olof on niita miehia ."
     DATA "Vankikarkuri kupsahtaa vasta kolmannella pampulla"
     DATA "PULTSARI   -  Written By Mika Maaranen                          28.12.1991"
     DATA "Kalaa kannatta syoda harkiten."
     DATA "Sinulla on 50 % mahdollisuus,siihen,etta omena on herkullinen"
     DATA "Poliisien kanssa ei ole leikkimista. "
     DATA "Mummot saattavat huitaista kasilaukulla."
     DATA "Tassakin pelissa alco ottaa ylihintaa."
     DATA "Varkaat voi nuijia vain heittamalla."
     DATA "Vauhtia siihen peliin !!"
     DATA "Mitas oikein kuppaat ? Olet hakannut vasta pari mummoa !"
     DATA "Mummelit vaistelevat heittoja."
     DATA "Skinhead on saattanut sekoontua elaimiin..."
     DATA "Kankkusta kannatta valtella. Tai ainakin tassa pelissa."
     DATA "K-RAUTAa ei ole ohjelmoitu loppuun."
     DATA "Tavatessasi kaljapullon etikettia , tavaat: L-A-H-D-E-N IIIIIIIIIIIIIIII"
     DATA "Varo paskaa.Siihen liukastuu."
     DATA "Oispa tassa pelissa vessa."
     DATA "Alco ei myy humaltuneille."
     DATA "Divari ei osta juopuneilta."
     DATA "PULTSARI:  -  Idea By Mika -,Jari -,Tero Maaranen and Kimmo Korhonen."
     DATA "Mummot on vaarallisia."
     DATA "AIDS tappaa."
     DATA "Jos otat,et aja."
     DATA "Poliisi asemalla on putka,kokeile sita(vasta viidennen levelin jalkeen)"
     DATA "Burb !!"
     DATA "Tama ohjelma on laadittu kayttamalla Qbasic 4.5 - ohjelmaa."
     DATA "Vankikarkurit ne heittelleepi ilkeesti kivilla - jos ne loytaa sellasen."
     DATA "By Miksu Soft. Eikun pulsusoftware 5.4.2022."
     DATA "Tiesitteko,etta radiota kuunnellessa on hyva ohjelmoida ?"
     DATA "Konekirjoitustaidosta on hyotya."
     DATA "Mietit viimeoista untasi,ja sitten muistat,ettet nukkunut ollenkaan."
     DATA "kakakakalaa."
     DATA "Muistat KIROSANAN,jonka joku joskus sulle selitti : P E L I  K O N S O L I"
     DATA "Ultima 6 onnesi uus"
     DATA "Kysy uudempaa versiota,soita 12345... (hp vain vaivaiset 999999999999999 mk.)"
     DATA "We wish you a merry cristmas and a happy new year...1992."
     DATA "Zak McKracken voisi olla tan pelin kunkku."
     DATA "Leisuresuit Larryssa on puhtia."
     DATA "Putkassa mieli sekoaa."
     DATA "Punaista pain !!!"
     DATA "Vinkki :poliisit vaarantavat terveytesi !!!"
     DATA "The Last Zone...sorry soor, The Last Bone."
     DATA ""




ALKO: X = X + XX: Y = Y + YY: COLOR 10
      IF PROM > 14 THEN LOCATE 1, 1: PRINT "Vartija heitti sinut pihalle. Olet juopunut.": RETURN
      IF PROM < 5 THEN LOCATE 1, 1: PRINT "Tehan olette selva. Meilla sen voi korjata.": GOSUB NAPISKA
      IF MONEY < 15 THEN LOCATE 1, 1: COLOR 12: PRINT "Putiikki on kiinni.": RETURN
      LOCATE 1, 1: PRINT "Ja mitas herra ostaa ,kalja vai lonkka (1/2)?"
ALK:  A$ = INKEY$: IF A$ <> "1" AND A$ <> "2" THEN GOTO ALK
      IF A$ = "1" THEN BEER = BEER + 1: MONEY = MONEY - 10: GOSUB TULOSTUS: LOCATE 1, 1: PRINT "Kiitos,ja hyv�� p�iv�n jatkoa.                                  ": RETURN
      MONEY = MONEY - 15: LON = LON + 1: LOCATE 1, 1: PRINT "A wise choice.                                              ": GOSUB TULOSTUS: RETURN
      STOP
VARAS: REM ******************* VARKAAN LIIKKEET ***********************
       LOCATE VY, VX: PRINT " "
       KEN(VX, VY) = 0
       IF X + 1 < VX THEN VX = VX - 1: IF KEN(VX, VY) <> 0 THEN GOSUB TESTI: VX = VX + 1
       IF X - 1 > VX THEN VX = VX + 1: IF KEN(VX, VY) <> 0 THEN GOSUB TESTI: VX = VX - 1
       IF Y - 1 > VY THEN VY = VY + 1: IF KEN(VX, VY) <> 0 THEN GOSUB TESTI: VY = VY - 1
       IF Y + 1 < VY THEN VY = VY - 1: IF KEN(VX, VY) <> 0 THEN GOSUB TESTI: VY = VY + 1
       IF VALI = 0 THEN KEN(VX, VY) = 21: LOCATE VY, VX: PRINT "V"
       IF KEN(X, Y - 1) = 21 OR KEN(X + 1, Y - 1) = 21 THEN GOSUB EDD: GOTO SEUR
       IF KEN(X + 1, Y) = 21 OR KEN(X + 1, Y + 1) = 21 OR KEN(X, Y + 1) = 21 OR KEN(X, Y + 1) = 21 OR KEN(X - 1, Y + 1) = 21 OR KEN(X - 1, Y) = 21 OR KEN(X - 1, Y - 1) = 21 THEN GOSUB EDD
       GOTO SEUR
EDD:
      
      
       LOCATE 1, 1: COLOR 10: PRINT "Kateiskassasi kutistuu !!!":
       RAN = INT(RND * 15) + 1: MONEY = MONEY - RAN:
       LOCATE VY, VX: PRINT " ": KEN(VX, VY) = 0: VELO = 0:
       IF MONEY < 0 THEN MONEY = 0
       RETURN
SEUR:
      
       IF VALI = 1 THEN VELO = 0: KEN(VX, VY) = 0: VALI = 0
       RETURN
TESTI: REM ********* TESTATAAN, MIHIN VARAS MENEE ************
       IF KEN(VX, VY) = 28 THEN VALI = 1: KEN(VX, VY) = 0: LOCATE VY, VX: PRINT " ": LOCATE 1, 1: PRINT "Varas liukastui paskaan,ja kuoli.": RETURN
       IF KEN(VX, VY) = 22 THEN KEN(VX, VY) = 0: LOCATE 1, 1: PRINT "Mummo jai varkaan kynsiin.": LOCATE VY, VX: PRINT " ": RETURN
       IF KEN(VX, VY) = 14 THEN VALI = 1: LOCATE 1, 1: PRINT "Poliisi pidattaa varkaan.": VELO = 0: RETURN
       RETURN

MUMMO: REM ***** LY�K� MUMMELI ? ******
       HY = 1
       RAN = INT(RND * 100): IF RAN < 50 THEN PRINT "Elakellainen hakkaa sua kasilaukullaan !!!": RAN = INT(RND * 2) + 1: VOIMA = VOIMA - RAN: GOSUB TULOSTUS: X = X + XX: Y = Y + YY: RETURN
       PRINT "Mukiloit mummon kumoon. Haa,lompsa !!!"
       KEN(X, Y) = 23: LOCATE Y, X: COLOR 10: PRINT "$"
       KM = KM + 1: ALL = ALL + 1: X = X + XX: Y = Y + YY: RETURN

POLSKI: REM ***TARKISTELLAAN,ONKO KENTIES POLLARI L�HELL� PAMPUTUS AIKEISSA***
        IF HY = 0 THEN HHH = 1 ELSE HHH = 0
        IF KEN(X + 1, Y) = 14 THEN GOSUB POLL
        IF KEN(X + 1, Y + 1) = 14 THEN GOSUB POLL
        IF KEN(X, Y + 1) = 14 THEN GOSUB POLL
        IF KEN(X - 1, Y + 1) = 14 THEN GOSUB POLL
        IF KEN(X - 1, Y) = 14 THEN GOSUB POLL
        IF KEN(X - 1, Y - 1) = 14 THEN GOSUB POLL
        IF KEN(X, Y - 1) = 14 THEN GOSUB POLL
        IF KEN(X + 1, Y - 1) = 14 THEN GOSUB POLL
RETURN
NAPISKA: A$ = INKEY$: IF A$ = "" THEN GOTO NAPISKA
         RETURN
DIVARI: REM ************  MYYNTI DIVARISSA  *************
        COLOR 10
        IF PROM > 15 THEN LOCATE 1, 1: PRINT "Teh�n olette r�k�k�nniss�. Ulos t��lt� !"
        IF KA < 1 AND KNI < 1 AND PA < 1 AND KET < 1 THEN LOCATE 1, 1: PRINT "Emme osta teilt� mit��n. Out."
        LOCATE 1, 1: PRINT "Mit�s myisit ? Bootsit=1  Pamppu=2  Ketjut=3  Veitsi=4  ": X = X + XX: Y = Y + YY
DIV:    A$ = INKEY$: IF A$ <> "1" AND A$ <> "2" AND A$ <> "3" AND A$ <> "4" THEN GOTO DIV
        LOCATE 1, 1: PRINT "                                                               "
        IF A$ = "1" THEN IF KA < 1 THEN LOCATE 1, 1: PRINT "Eihan teilla ole kenkia edes jaloissanne.": RETURN ELSE LOCATE 1, 1: MONEY = MONEY + 100: PRINT "Bootsit meni satasella.": KA = KA - 1: GOSUB TULOSTUS: RETURN
        IF A$ = "2" THEN IF PA < 1 THEN LOCATE 1, 1: PRINT "Joo,montahan noita pamppuja sinulla onkin.": RETURN ELSE LOCATE 1, 1: RAN = INT(RND * 40) + 1: PRINT "Pamppu hurahti "; RAN; " markalla.": PA = PA - 1: MONEY = MONEY + RAN: GOSUB TULOSTUS:  _
RETURN
        IF A$ = "3" THEN IF KET < 1 THEN LOCATE 1, 1: PRINT "Mutta , kun sinulla ei ole yhtikas yhtaan ketjun patkaakaan.": RETURN ELSE LOCATE 1, 1: PRINT "Ketjut onnistuit myymaan kolmella kympilla.": MONEY = MONEY + 30: KET = KET - 1: GOSUB  _
TULOSTUS: RETURN
        IF KNI < 1 THEN LOCATE 1, 1: PRINT "Et omista veitsia.": RETURN
        KNI = KNI - 1
        LOCATE 1, 1: RAN = INT(RND * 50) + 1: PRINT "Voitit veitellas "; RAN; " markkaa.": MONEY = MONEY + RAN: GOSUB TULOSTUS: RETURN

PUTKA:  REM ******** PUTKA *********
        A$ = INKEY$: IF A$ <> "k" AND A$ <> "K" AND A$ <> "E" AND A$ <> "e" THEN GOTO PUTKA
        IF A$ = "E" OR A$ = "e" THEN LOCATE 1, 1: PRINT "No haivy sitten !!!                                               ": Y = Y + YY: RETURN
        LOCATE 1, 1: PRINT "Putkassa istuskellessasi huomaat parin tunnin paasta miten          <LISaa>   "
PUTK:   A$ = INKEY$: IF A$ = "" THEN GOTO PUTK
        LOCATE 1, 1: PRINT "Paasi alkaa selveta,ja miten kankkunen tulee. Game over !!!!!              "
PUTT:   A$ = INKEY$: IF A$ = "" THEN GOTO PUTT
        LOCATE 1, 1: PRINT "Uudestaan (k/e)?"
PU:     A$ = INKEY$: IF A$ = "E" OR A$ = "e" THEN CLS : SCREEN 0: END
        IF A$ = "K" OR A$ = "k" THEN GOTO 1
        GOTO PU
    
YKA:    REM *********** YKA *************     
        COLOR 14
        for tx=x-1 to x+1: for ty=y-1 to y+1
        IF KEN(tx,ty) = 32 THEN 
            LOCATE 1, 1: PRINT "Yka pyytaa yhta kaljaa, annatko(k/e)?": GOSUB KYSY: RETURN
            end if
        next ty,tx

        COLOR 15: KEN(YKX, YKY) = 0: LOCATE YKY, YKX: PRINT " "
        IF X + 1 < YKX THEN YKX = YKX - 1: IF KEN(YKX, YKY) <> 0 THEN YKX = YKX + 1
        IF X - 1 > YKX THEN YKX = YKX + 1: IF KEN(YKX, YKY) <> 0 THEN YKX = YKX - 1
        IF Y - 1 > YKY THEN YKY = YKY + 1: IF KEN(YKX, YKY) <> 0 THEN YKY = YKY - 1
        IF Y + 1 < YKY THEN YKY = YKY - 1: IF KEN(YKX, YKY) <> 0 THEN YKY = YKY + 1
        KEN(YKX, YKY) = 32: LOCATE YKY, YKX: PRINT "Y"
        RETURN


KYSY: A$ = INKEY$: IF A$ <> "K" AND A$ <> "k" AND A$ <> "E" AND A$ <> "e" THEN GOTO KYSY
      
      IF A$ = "E" OR A$ = "e" THEN LOCATE 1, 1: PRINT "Yka taputteleepi palleaasi.                        ": VOIMA = VOIMA - 1: GOSUB TULOSTUS: RETURN
      LOCATE 1, 1: PRINT "Yka kiitteleepi, ja istuu alas kittaamaan kaljaasi.                           ": BEER = BEER - 1: GOSUB TULOSTUS: RETURN

KONNA: REM ******************** VANKIKARKURI ********************
       COLOR 14
       for tx=x-1 to x+1: for ty=y-1 to y+1

       IF KEN(tx,ty) = 33 THEN 
            LOCATE 1, 1: PRINT "Vankikarkuri kurmottaa sinua.": GOSUB KONN: RETURN
            end if
       next ty,tx

       KEN(KONX, KONY) = 0: LOCATE KONY, KONX: PRINT " "
       IF X + 1 < KONX THEN KONX = KONX - 1: IF KEN(KONX, KONY) <> 0 THEN GOSUB TESTII: KONX = KONX + 1
       IF X - 1 > KONX THEN KONX = KONX + 1: IF KEN(KONX, KONY) <> 0 THEN GOSUB TESTII: KONX = KONX - 1
       IF Y - 1 > KONY THEN KONY = KONY + 1: IF KEN(KONX, KONY) <> 0 THEN GOSUB TESTII: KONY = KONY - 1
       IF Y + 1 < KONY THEN KONY = KONY - 1: IF KEN(KONX, KONY) <> 0 THEN GOSUB TESTII: KONY = KONY + 1
       IF VALI = 0 THEN KEN(KONX, KONY) = 33: LOCATE KONY, KONX: PRINT "E"
       IF VALI = 1 THEN VALI = 0: KEN(KONX, KONY) = 0
       RETURN
TESTII: REM **** TESTATAAN,MINNE KARKURI MENEE ****
        IF KEN(KONX, KONY) = 14 THEN VALI = 1: LOCATE 1, 1: PRINT "Poliisi pid�tt�� vankikarkurin.": CONMAN = 0: RETURN
        IF KEN(KONX, KONY) = 3 THEN LOCATE 1, 1: PRINT "Vankikarkuri ahmii kiekuralenkin.": KEN(KONX, KONY) = 0: RETURN
        IF KEN(KONX, KONY) = 7 THEN LOCATE 1, 1: PRINT "Vankikarkuri nappaa kiven.": VANK = 1: KEN(KONX, KONY) = 0: RETURN
RETURN
KONN: RAN = INT(RND * 3) + 1: VOIMA = VOIMA - RAN: GOSUB TULOSTUS: RETURN
VANHEITTO: REM ********** KARKURI HEITT�� KIVEN *************
           COLOR 10: LOCATE 1, 1: PRINT "Vankikarkuri heitt�� kivell� !!!": COLOR 15
           VALX = KONX: VALY = KONY: IF Y = KONY THEN GOTO AX
           IF Y < KONY THEN HEIY = -1 ELSE HEIY = 1
HHHH:      IF VALX = X AND VALY = Y THEN GOTO OKEI
           LOCATE VALY, VALX: PRINT " ": LOCATE KONY, KONX: PRINT "E"
           VALY = VALY + HEIY: LOCATE VALY, VALX: PRINT "*"
           FOR T = 1 TO 500: NEXT
           GOTO HHHH
AX:        IF X < KONX THEN HEIX = -1 ELSE HEIX = 1
H:         IF VALX = X AND VALY = Y THEN GOTO OKEI
           LOCATE VALY, VALX: PRINT " ": LOCATE KONY, KONX: PRINT "E"
           VALX = VALX + HEIX: LOCATE VALY, VALX: PRINT "*"
           FOR T = 1 TO 500: NEXT
           GOTO H

OKEI: VANK = 0: RAN = INT(RND * 5) + 1: VOIMA = VOIMA - RAN: GOSUB TULOSTUS
      RETURN
     
SKINHEAD: REM ***** KALJUN LIIKKEET *****

      for tx=1-1 to x+1: for ty=y-1 to y+1
       IF KEN(tx, ty) = 34 THEN 
            LOCATE 1, 1: IF TULO < 1 THEN PRINT "Skinhead murjoo sinuu apinan raivolla.": GOSUB KONNN: RETURN ELSE GOSUB KONNN: RETURN
            end if
      next ty,tx
      KEN(SKIX, SKIY) = 0: LOCATE SKIY, SKIX: PRINT " "
      IF X + 1 < SKIX THEN SKIX = SKIX - 1: IF KEN(SKIX, SKIY) <> 0 THEN SKIX = SKIX + 1
      IF X - 1 > SKIX THEN SKIX = SKIX + 1: IF KEN(SKIX, SKIY) <> 0 THEN SKIX = SKIX - 1
      IF Y - 1 > SKIY THEN SKIY = SKIY + 1: IF KEN(SKIX, SKIY) <> 0 THEN SKIY = SKIY - 1
      IF Y + 1 < SKIY THEN SKIY = SKIY - 1: IF KEN(SKIX, SKIY) <> 0 THEN SKIY = SKIY + 1
      KEN(SKIX, SKIY) = 34: LOCATE SKIY, SKIX: PRINT "O"
      
      RETURN
  
KONNN: RAN = INT(RND * 3) + 1: VOIMA = VOIMA - RAN: GOSUB TULOSTUS: RETURN

HORJUNTA: REM ******** HORJUMINEN ***********
          LOCATE Y, X: PRINT " "
          RAN = INT(RND * 100): IF RAN < 40 THEN COLOR 14: LOCATE Y, X: PRINT "�": RETURN
          RAN = INT(RND * 100): IF RAN < 50 THEN XX = -1 ELSE XX = 1
          X = X + XX: IF KEN(X, Y) <> 0 THEN XX = XX * -1: GOSUB REAGOINTI
          RAN = INT(RND * 100): IF RAN < 40 THEN LOCATE Y, X: COLOR 14: PRINT "�": RETURN
          RAN = INT(RND * 100): IF RAN < 50 THEN YY = -1 ELSE YY = 1
          Y = Y + YY: IF KEN(X, Y) <> 0 THEN YY = YY * -1: GOSUB REAGOINTI
          COLOR 14: LOCATE Y, X: PRINT "�"
          RETURN

SONTA:    REM ************** MINNE TULEE YRJ� *****************
          RAN = INT(RND * 100): IF RAN < 50 THEN ZX = -1 ELSE ZX = 1
          RAN = INT(RND * 100): IF RAN < 50 THEN ZY = -1 ELSE ZY = 1
          IF KEN(X + ZX, Y + ZY) = 22 THEN LOCATE 1, 1: COLOR 14: PRINT "Yrjos meni mummon niskoille. Se kuoli."
          IF KEN(X + ZX, Y + ZY) = 14 THEN LOCATE 1, 1: COLOR 14: PRINT "Polsu tukehtui ykaas,muut pollarit ei taija tykata susta.": HY = 1
          IF KEN(X + ZX, Y + ZY) <> 0 AND KEN(X + ZX, Y + ZY) <> 22 AND KEN(X + ZX, Y + ZY) <> 14 THEN GOTO JEPULIS
          RETURN
JEPULIS:
          KEN(X + ZX, Y + ZY) = 28: LOCATE Y + ZY, X + ZX: COLOR 14: PRINT "�"
          RETURN
