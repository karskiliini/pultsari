1 Dim KEN(80, 28), XXXX(30), YYYY(30), ELO(30), ZZXX(40): Randomize Timer
Cls: Screen 13: Color 32

A$ = "(C) By Mika  Maaranen  "
B$ = "       Jari  Maaranen"
C$ = "       Tero  Maaranen"
D$ = "       Kimmo Korhonen"
Color 10
For T = 1 To 21: YYYY(T) = Int(Rnd * 10): YYYY(T) = YYYY(T) * -1: Next
For T = 1 To 20: For TT = 1 To 21: If YYYY(TT) > 0 And YYYY(TT) < 10 Then Locate YYYY(TT), TT + 2: Print " "
    If YYYY(TT) < 10 Then YYYY(TT) = YYYY(TT) + 1:
        If YYYY(TT) > 0 Then Locate YYYY(TT), TT + 2: P$ = Left$(D$, TT): Print Right$(P$, 1)
Next TT, T

Color 12
For T = 1 To 21: YYYY(T) = Int(Rnd * 10): YYYY(T) = YYYY(T) * -1: Next
For T = 1 To 20: For TT = 1 To 21: If YYYY(TT) > 0 And YYYY(TT) < 9 Then Locate YYYY(TT), TT + 2: Print " "
    If YYYY(TT) < 9 Then YYYY(TT) = YYYY(TT) + 1:
        If YYYY(TT) > 0 Then Locate YYYY(TT), TT + 2: P$ = Left$(C$, TT): Print Right$(P$, 1)
Next TT, T

Color 14
For T = 1 To 21: YYYY(T) = Int(Rnd * 10): YYYY(T) = YYYY(T) * -1: Next
For T = 1 To 20: For TT = 1 To 21: If YYYY(TT) > 0 And YYYY(TT) < 8 Then Locate YYYY(TT), TT + 2: Print " "
    If YYYY(TT) < 8 Then YYYY(TT) = YYYY(TT) + 1:
        If YYYY(TT) > 0 Then Locate YYYY(TT), TT + 2: P$ = Left$(B$, TT): Print Right$(P$, 1)
Next TT, T

Color 33
For T = 1 To 21: YYYY(T) = Int(Rnd * 10): YYYY(T) = YYYY(T) * -1: Next
For T = 1 To 20: For TT = 1 To 21: If YYYY(TT) > 0 And YYYY(TT) < 7 Then Locate YYYY(TT), TT + 2: Print " "
    If YYYY(TT) < 7 Then YYYY(TT) = YYYY(TT) + 1:
        If YYYY(TT) > 0 Then Locate YYYY(TT), TT + 2: P$ = Left$(A$, TT): Print Right$(P$, 1)
Next TT, T

A$ = " PULTSARI  "
For T = 16 To 24: Locate 1, T: P$ = Left$(A$, T - 15): COL = Int(Rnd * 30) + 30: Color COL: Print Right$(P$, 1)
For TT = 1 To 500: Next TT, T

Color 10: Locate 23, 1: Print "28.12.1991 - 14.1.1992 (V„lill„ paussi)"
Locate 10, 1

Print: Print: Color 34: Print "Press any key to continue"

2 A$ = InKey$: If A$ = "" Then GoTo 2
CONMAN = 0
Cls: Randomize Timer
Screen 12:
For T = 13 To 79: Locate 2, T: Print Chr$(205): Locate 24, T: Print Chr$(205): Next
For T = 2 To 24: Locate T, 13: Print Chr$(186): Locate T, 80: Print Chr$(186): Next
Locate 2, 13: Print Chr$(201): Locate 2, 80: Print Chr$(187)
Locate 24, 13: Print Chr$(200): Locate 24, 80: Print Chr$(188): MONEY = 0: VOIMA = 10: PROM = 2: WALK = 0: BEER = 1: LON = 0: MAK = 0: BA = 0: OM = 1: PU = 0: KA = 0: KI = 2: KNI = 0: KET = 0: MAK = 1: LEV = 1: VUORO = 0
GoTo ALKU
TULOSTUS: Rem ******** TULOSTUS *********

Locate 28, 1: Color 10: Print "RAHAA:"; MONEY:
Locate 28, 12: Print "VOIMA:"; VOIMA
Locate 28, 24: Print "PROMILLE:"; PROM / 10
Locate 28, 40: Print "KŽPPŽILY:"; WALK
Locate 28, 56: Print "LEVEL:"; LEV

Locate 3, 1: Print "KALJAA ="; BEER
Print "LONKKA ="; LON
Print "LENKKI ="; MAK
Print "KETJUT ="; KET
Print "VEITET ="; KNI
Print "KIVET  ="; KI
Print "PAMPUT ="; PA
Print "BOOTSIT="; KA
Print "KALAT  ="; PU
Print "OMPPO  ="; OM
Print "BANSKU ="; BA
Return

KENT: Rem ************ UUSI KENTTŽ ****************
For T = 2 To 24: For TT = 1 To 79: KEN(TT, T) = 0: Next TT, T
Locate 28, 56: Print "LEVEL:"; LEV
For T = 2 To 24: Locate T, 13: Print Tab(80); " ": KEN(13, T) = 1: KEN(80, T) = 1: Next
VUORO = VUORO + 1: If VUORO > 2 Then VUORO = 1
Randomize Timer
RAN = Int(Rnd * 100): If RAN < 50 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print "%": KEN(XXX, YYY) = 4
RAN = Int(Rnd * 100): If RAN < 25 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print "(": KEN(XXX, YYY) = 3
RAN = Int(Rnd * 100): If RAN < 10 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print Chr$(225): KEN(XXX, YYY) = 5
RAN = Int(Rnd * 100): If RAN < 20 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print "\": KEN(XXX, YYY) = 6
RAN = Int(Rnd * 100): If RAN < 55 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print ".": KEN(XXX, YYY) = 7
RAN = Int(Rnd * 100): If RAN < 15 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print "œ": KEN(XXX, YYY) = 8
RAN = Int(Rnd * 100): If RAN < 5 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print Chr$(157): KEN(XXX, YYY) = 9
RAN = Int(Rnd * 100): If RAN < 10 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print "o": KEN(XXX, YYY) = 10
RAN = Int(Rnd * 100): If RAN < 5 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print ")": KEN(XXX, YYY) = 11
RAN = Int(Rnd * 100): If RAN < 45 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print "&": KEN(XXX, YYY) = 16
RAN = Int(Rnd * 100): If RAN < 80 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: Locate YYY, XXX: Print "?": KEN(XXX, YYY) = 17
RAN = Int(Rnd * 100): If RAN < 90 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 20) + 2: If KEN(XXX, YYY) = 0 Then Locate YYY, XXX: Print "‘": KEN(XXX, YYY) = 28

Locate 23, 79: Print "^": KEN(79, 23) = 12
Color 15
      
For T = 13 To 79: Locate 2, T: Print Chr$(205): Locate 24, T: Print Chr$(205): KEN(T, 2) = 1: KEN(T, 24) = 1: Next
For T = 2 To 24: Locate T, 13: Print Chr$(186): Locate T, 80: Print Chr$(186): Next
Locate 2, 13: Print Chr$(201): Locate 2, 80: Print Chr$(187)
Locate 24, 13: Print Chr$(200): Locate 24, 80: Print Chr$(188)
If VUORO = 1 Then Locate 3, 14: Print "ALKO  "; Chr$(219): Locate 4, 14: Print "      "; Chr$(219): For T = 14 To 20: KEN(T, 5) = 2: Locate 5, T: Print Chr$(219): Next: KEN(20, 4) = 2: KEN(20, 3) = 2: KEN(16, 5) = 18: Locate 5, 16: Print Chr$(177)
If VUORO = 2 Then Locate 3, 14: Print "K-RAUTA"; Chr$(219): Locate 4, 14: Print "       "; Chr$(219): For T = 14 To 21: Locate 5, T: Print Chr$(219): KEN(T, 5) = 19: Next: KEN(21, 3) = 19: KEN(21, 4) = 19: KEN(17, 5) = 20: Locate 5, 17: Print Chr$(176)
     
PIS = PIS + 1: If PIS > 20 Then PIS = 20
For T = 1 To PIS
    PIS: XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 11) + 6: If KEN(XXX, YYY) <> 0 Then GoTo PIS
    Locate YYY, XXX: Color 12: Print Chr$(248): KEN(XXX, YYY) = 13
Next
   
RR = Int(Rnd * 9) + 1: POL = RR: For T = 1 To POL
    PPP: XX = Int(Rnd * 64) + 14: YY = Int(Rnd * 15) + 6: If KEN(XX, YY) <> 0 Then GoTo PPP
    Locate YY, XX: Color 10: Print "C": KEN(XX, YY) = 14: XXXX(T) = XX: YYYY(T) = YY: ELO(T) = 1
Next: XX = 0: YY = 0
Locate 1, 1: Print Tab(80);

If LEV > 2 Then RAN = Int(Rnd * 100): If RAN < 60 Then XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 12) + 6: If KEN(XXX, YYY) = 0 Then VX = XXX: VY = YYY: VELO = 1: Locate VY, VX: Print "V"
MMUU: XXX = Int(Rnd * 64) + 14: YYY = Int(Rnd * 12) + 5: If KEN(XXX, YYY) <> 0 Then GoTo MMUU Else KEN(XXX, YYY) = 22: Color 10: Locate YYY, XXX: Print "M"
Rem *** DIVARI TAI ISKU ***
RAN = 0: Color 15
If LEV > 4 Then RAN = Int(Rnd * 100): If RAN < 50 Then RAN = 1 Else RAN = 2
If RAN = 1 Then DIVARI = 1: ISKU = 0
If RAN = 2 Then DIVARI = 0: ISKU = 1
If DIVARI = 1 Then For T = 14 To 25: Locate 20, T: KEN(T, 20) = 24: Print "Û": Next: For T = 20 To 23: Locate T, 25: Print "Û": KEN(25, T) = 24: Next: Locate 20, 20: Print "±": KEN(20, 20) = 25: Locate 23, 15: Print "DIVARI"
If ISKU = 1 Then For T = 14 To 25: Locate 20, T: KEN(T, 20) = 26: Print "Û": Next: For T = 20 To 23: Locate T, 25: Print "Û": KEN(25, T) = 26: Next: Locate 20, 20: Print "±": KEN(20, 20) = 27: Locate 23, 15: Print "ISKU"
Rem *** POLIISIASEMA+VANKILA ***
RAN = 0
RAN = Int(Rnd * 100): If RAN < 50 Then RAN = 0 Else RAN = 1
If LEV > 6 And RAN = 1 Then For T = 40 To 50: Locate 20, T: KEN(T, 20) = 29: Print "Û": Locate 20, T + 12: Print "Û": KEN(T + 12, 20) = 29: Next: For T = 20 To 23: KEN(40, T) = 29: KEN(50, T) = 29: KEN(52, T) = 29: KEN(62, T) = 29: Locate T, 62: Print "Û": Locate T, 52: Print "Û": Locate T, 50: Print "Û": Locate T, 40: Print "Û": Next: Locate 20, 45: Print "°": KEN(45, 20) = 30: Locate 23, 41: Print "POLIISI": Locate 23, 53: Print "PUTKA": PUTKA = 1: Locate 20, 57: Print "²": KEN(57, 20) = 31
RAN = Int(Rnd * 100): If RAN < 60 Then YKX = 18: YKY = 12: If KEN(YKX, YKY) = 0 Then KEN(YKX, YKY) = 32: Locate YKY, YKX: Print "˜": YKA = 1
     
Rem *** ASEMA ***
If LEV > 8 Then For T = 65 To 77: Locate 22, T: Print "Û": KEN(T, 22) = 29: Next: Locate 23, 70: Print "ÛASEMAÛÛ": KEN(70, 23) = 29: KEN(77, 23) = 29: ASEM = 1: SPOW = 7



Return

HEITA: Rem ****************** HEITTO ********************
Locate 1, 1: Input "ANNA NUMERO .."; AA
If AA = 1 Then BEER = BEER - 1: If BEER < 0 Then BEER = 0: Return Else GoTo HEIT
If AA = 2 Then LON = LON - 1: If LON < 0 Then LON = 0: Return Else GoTo HEIT
If AA = 3 Then MAK = MAK - 1: If MAK < 0 Then MAK = 0: Return Else GoTo HEIT
If AA = 4 Then KET = KET - 1: If KET < 0 Then KET = 0: Return Else GoTo HEIT
If AA = 5 Then KNI = KNI - 1: If KNI < 0 Then KNI = 0: Return Else GoTo HEIT
If AA = 6 Then KI = KI - 1: If KI < 0 Then KI = 0: Return Else GoTo HEIT
If AA = 7 Then PA = PA - 1: If PA < 0 Then PA = 0: Return Else GoTo HEIT
If AA = 8 Then KA = KA - 1: If KA < 0 Then KA = 0: Return Else GoTo HEIT
If AA = 9 Then PU = PU - 1: If PU < 0 Then PU = 1: Return Else GoTo HEIT
If AA = 10 Then OM = OM - 1: If OM < 0 Then OM = 0: Return Else GoTo HEIT
If AA = 11 Then BA = BA - 1: If BA < 0 Then BA = 0: Return Else GoTo HEIT
Return
HEIT: Locate 1, 1: Print Tab(80);
Locate 1, 1: Color 7: Print "Suunta ...??              ": XX = 0: YY = 0
HEHE: A$ = InKey$: If A$ = "8" Then YY = -1
If A$ = "6" Then XX = 1
If A$ = "5" Then YY = 1
If A$ = "4" Then XX = -1
If A$ <> "8" And A$ <> "6" And A$ <> "5" And A$ <> "4" Then GoTo HEHE
GoSub TULOSTUS: XXX = X + XX: YYY = Y + YY: Locate 1, 1: Print "                                               "
HEI:
If KEN(XXX, YYY) <> 0 Then XX = XX * -1: YY = YY * -1: GoTo HE
Locate YYY, XXX: Print "*": For T = 1 To 250: Next
Locate YYY, XXX: Print " "
XXX = XXX + XX: YYY = YYY + YY:
GoTo HEI
HE: If KEN(XXX, YYY) = 14 Then Locate 1, 1: Color 12: Print "POLLARI KUPSAHTI !!!": KEN(XXX, YYY) = 15: Locate YYY, XXX: Print "!": HY = 1: GoSub ELOTON: ALL = ALL + 1
If KEN(XXX, YYY) = 1 Or KEN(XXX, YYY) = 2 Then Locate 1, 1: Color 10: Print "Kops !!!": XX = XX * -1: YY = YY * -1: XXX = XXX + XX: YYY = YYY + YY: KEN(XXX, YYY) = AA: GoSub TARK
If KEN(XXX, YYY) = 21 Then RAN = Int(Rnd * 100): If RAN < 40 Then Locate 1, 1: Print "OHI !!!": Return Else Locate 1, 1: Print "Osuit varkaaseen, joka kupsahti.": KEN(XXX, YYY) = 23: Locate YYY, XXX: Print "$": VELO = 0: ALL = ALL + 1
If KEN(XXX, YYY) = 22 Then RAN = Int(Rnd * 100): If RAN < 50 Then Locate 1, 1: Print "Mummo v„ist„„ !!": Return Else Locate 1, 1: Print "Kolkkasit juuri mumumummurun": KEN(XXX, YYY) = 23: Locate YYY, XXX: Print "$": ALL = ALL + 1: KM = KM + 1
If KEN(XXX, YYY) = 33 Then RAN = Int(Rnd * 100): If RAN < 40 Then Locate 1, 1: Print "Vankikarkuri v„ist„„.": Return Else Locate 1, 1: Print "Heittosi osuu vankikarkuria p„„h„n. H„n...se n„ytt„„ vihastuvan.": KONP = KONP - 1: If KONP < 2 Then Locate 1, 1: Print "N„ytt„„ silt„,ett„ vankikarkuri p”kertyy heittosi ansiosta maahan...         ": CONMAN = 0: KEN(XXX, YYY) = 0: If KON < 1 Then Locate KONY, KONX: Print " ": ALL = ALL + 1:
If KEN(XXX, YYY) = 34 Then Locate 1, 1: Print "Heittosi ossuupi skinukkelia vy”n alle. Se k„rsii !!!": SPOW = SPOW - 2: GoSub TULOSTUS: If SPOW < 1 Then Locate SKIY, SKIX: Print " ": ALL = ALL + 1: Return Else Return

Return
TARK: Rem *********** ETSITŽŽN, MIKŽ TAVARA OSUI SEINŽŽN ***************
Locate YYY, XXX
If AA = 1 Then Print "%"
If AA = 2 Then Print "("
If AA = 3 Then Print "&"
If AA = 4 Then Print "á"
If AA = 5 Then Print "\"
If AA = 6 Then Print "!"
If AA = 7 Then Print "œ"
If AA = 8 Then Print ""
If AA = 9 Then Print "o"
If AA = 10 Then Print ")"
Return
     
ELOTON: Rem ***** ETSITŽŽN KUOLLUT POLLARI *****
For T = 1 To POL
    If XXXX(T) = XXX And YYYY(T) = YYY Then ELO(T) = 0
Next
Return

GLU: Rem ************** JUONTI ****************
If BEER < 1 And LON < 1 Then Locate 1, 1: Color 12: Print "Eip„ oo ehtaa tavaraa.": Return
If BEER > 0 And LON > 0 Then Locate 1, 1: Print "Kumpaakos horaat,kaljaa vai lonkkaa (1,2) ?": GoSub JOKU: Return
If BEER < 1 Then LON = LON - 1: GoSub TULOSTUS: Locate 1, 1: Color 10: Print "Glub glub .. gulb gulu... ”””rbbbs.": PROM = PROM + 4: Return
If LON < 1 Then BEER = BEER - 1: GoSub TULOSTUS: Locate 1, 1: Color 10: Print "<Tsuuuhss> .. gluuuub gluub glub ... ”””rrr””yyh hh hh": PROM = PROM + 3: Return
Stop
JOKU: A$ = InKey$: If A$ <> "1" And A$ <> "2" Then GoTo JOKU
If A$ = "1" Then BEER = BEER - 1: PROM = PROM + 2
If A$ = "2" Then LON = LON - 1: PROM = PROM + 4
Locate 1, 1: Color 10: Print " Glub glub glub glub ..... Burb !!!                     "
GoSub TULOSTUS
Return
     
ALKU: Rem ************* PELI ***************
GoSub TULOSTUS
GoSub KENT
X = 17: Y = 12
PELI: A$ = InKey$
NAP: Locate Y, X: Print " "
Locate 1, 1: Print Tab(80);
If A$ = "4" Then WALK = WALK + 1: X = X - 1: If KEN(X, Y) <> 0 Then XX = 1: GoSub REAGOINTI
If A$ = "6" Then WALK = WALK + 1: X = X + 1: If KEN(X, Y) <> 0 Then XX = -1: GoSub REAGOINTI
If A$ = "8" Then WALK = WALK + 1: Y = Y - 1: If KEN(X, Y) <> 0 Then YY = 1: GoSub REAGOINTI
If A$ = "5" Then WALK = WALK + 1: Y = Y + 1: If KEN(X, Y) <> 0 Then YY = -1: GoSub REAGOINTI
Color 14: Locate Y, X: Print Chr$(234)
If A$ = "H" Or A$ = "h" Then GoSub HEITA
If A$ = "Q" Or A$ = "q" Then GoSub GLU
If A$ = "E" Or A$ = "e" Then GoSub EAT
If PROM > 23 Then Locate 26, 1: Color 14: Print "Horjahtelet.": GoSub HORJUNTA Else Locate 26, 1: Print "              "
If PROM < 4 Then Locate 26, 30: Color 14: Print "Sua janottaa !!!" Else Locate 26, 30: Print "                 "
If PROM > 28 Then RAN = Int(Rnd * 100): If RAN < 30 Then Locate 25, 1: Print "Oksennat !!": VOIMA = VOIMA - 1: GoSub SONTA: GoSub TULOSTUS Else Locate 25, 1: Print "            "
If PROM < 28 Then Locate 25, 1: Print "               "
If PROM > 34 Then Locate 1, 1: Color 14: Print "HIC ! ... Kompastut omiin jalkoihisi...huomaat ettet el„ ennee...": GoTo GOVER

GoSub POLSKI
POV = POV + 1: If POV > 260 Then POV = 1: PROM = PROM - 1: GoSub TULOSTUS: If PROM < .1 Then Locate 1, 1: Color 14: Print "Yht„kkii tajuat kraapuliuksen tulevan. Et kest„ en„„...": GoTo GOVER
If PROM < 2 Then Color 14: Locate 27, 1: Print "Voit pahoin !!" Else Locate 27, 1: Print "              "
VOI = VOI + 1: If VOI > 50 Then VOI = 1: VOIMA = VOIMA + 1: GoSub TULOSTUS
If HY = 1 Then GoSub POLLAR
If VELO = 1 Then GoSub VARAS
If BEER > 0 And YKA = 1 Then GoSub YKA
If PUTKA = 1 And CONMAN = 0 Then PUTKIS = PUTKIS + 1: If PUTKIS > 50 Then PUTKIS = 0: CONMAN = 1: KONX = 57: KONY = 19: KEN(KONX, KONY) = 34: KONP = 4
If CONMAN = 1 Then GoSub KONNA
If SPOW < 1 Then SPOW = 7: SKIH = 0: KEN(SKIX, SKIY) = 0: SKIX = 0: SKIY = 0
If SKIH = 0 And ASEM = 1 Then ASE = ASE + 1: If ASE > 30 Then ASE = 0: SKIH = 1: SKI = 5: KEN(69, 23) = 34: SKIX = 69: SKIY = 23: Locate SKIY, SKIX: Print "O"
If SKIH = 1 Then GoSub SKINHEAD
If VANK = 1 And X = KONX Or VANK = 1 And Y = KONY Then GoSub VANHEITTO
      

NAAP: A$ = InKey$:
If VOIMA < 1 Then Locate 1, 1: Color 14: Print "Terveytesi pett„„... kemahdat tantereeseen pitk„ksesi .........": GoTo GOVER

If A$ <> "" Then GoTo NAP
TULO = 0
Locate 28, 40: Color 10: Print "KŽPPŽILY:"; WALK
If NEX = 1 Then NEX = 0: LEV = LEV + 1: SIP = 0: GoTo ALKU

GoTo NAAP:

EAT: Rem *********** SY™P™TTELY ***************
Color 10
If MAK < 1 And OM < 1 And BA < 1 And PU < 1 Then Locate 1, 1: Print "Ei sy”t„v„„.": Return
If MAK < 1 And OM < 1 And PU < 1 Then Locate 1, 1: BA = BA - 1: RAN = Int(Rnd * 100): If RAN < 50 Then Print "Ei vaikutusta.": Return Else Print "Bansku teki ter„„ !!!": VOIMA = VOIMA + 1: GoSub TULOSTUS: Return
If MAK < 1 And PU < 1 And BA < 1 Then Locate 1, 1: OM = OM - 1: RAN = Int(Rnd * 100): If RAN < 50 Then Print "Omppo oli pilaantunut,oksennat.": VOIMA = VOIMA - 1: Return Else Print "Omena oli herkullinen !!!": VOIMA = VOIMA + 2: GoSub TULOSTUS: Return
If MAK < 1 And OM < 1 And BA < 1 Then Locate 1, 1: PU = PU - 1: RAN = Int(Rnd * 100): If RAN < 70 Then Print "Sy”m„si kala sis„lsi matoja...": VOIMA = VOIMA - 2: GoSub TULOSTUS: Return Else Print "Kala antaa sinulle uutta puhtia !!!": VOIMA = VOIMA + 4: GoSub TULOSTUS: Return
If OM < 1 And BA < 1 And BA < 1 Then MAK = MAK - 1: Locate 1, 1: Print "Oispa v„h„n kossua t„n kaa.": VOIMA = VOIMA + 1: GoSub TULOSTUS: Return
Locate 1, 1: Print "Mit„s...1=makkara 2=kala 3=omppo 4=bansku ???"
AAA: A$ = InKey$: If A$ <> "1" And A$ <> "2" And A$ <> "3" And A$ <> "4" Then GoTo AAA
Locate 1, 1: Print Tab(80);
Locate 1, 1
If A$ = "1" Then If MAK < 1 Then Print "Ei ole niit„.": Return Else Print "Mamamamakkaraa.": VOIMA = VOIMA + 1: MAK = MAK - 1: GoSub TULOSTUS: Return
If A$ = "2" Then If PU < 1 Then Print "Kalat loppu.": Return Else RAN = Int(Rnd * 100): If RAN < 70 Then Print "Joku oli piilottanut papanoita kalaasi !": VOIMA = VOIMA - 2: GoSub TULOSTUS: Return Else Print "Kaloreja kalasta.": VOIMA = VOIMA + 4: PU = PU - 1: GoSub TULOSTUS: Return
If A$ = "3" Then If OM < 1 Then Print "EE OO OMMPPOJA": Return Else RAN = Int(Rnd * 100): If RAN < 50 Then VOIMA = VOIMA - 1: Print "Joku oli k„ytellyt keski keppi„„n omppoosi.": GoSub TULOSTUS: Return Else Print "™rb.": VOIMA = VOIMA + 2: OM = OM - 1: GoSub TULOSTUS: Return
If A$ = "4" Then If BA < 1 Then Print "Bailu ilman banskuu ?": Return Else RAN = Int(Rnd * 100): If RAN < 50 Then Print "Bansku ei vaikuttanut.": Return Else Print "Babababananiii": VOIMA = VOIMA + 1: BA = BA - 1: GoSub TULOSTUS: Return
Stop
REAGOINTI: Color 12: TULO = 1
If KEN(X, Y) = 1 Then Locate 1, 2: Print "Eip„s karata pelialueelta !!": WALK = WALK - 1: X = X + XX: XX = 0: Y = Y + YY: YY = 0: Locate Y, X: Print Chr$(234)
If KEN(X, Y) = 2 Then Locate 1, 2: Print "Isket p„„si alkon sein„„n. Mielenkiintoista.": WALK = WALK - 1: X = X + XX: XX = 0: Y = Y + YY: YY = 0
If KEN(X, Y) = 3 Then Locate 1, 2: Print "Nam, lenkki makkaraa.": MAK = MAK + 1: KEN(X, Y) = 0
If KEN(X, Y) = 4 Then Locate 1, 2: Print "Katohhan, miehen mittainen.": BEER = BEER + 1: KEN(X, Y) = 0
If KEN(X, Y) = 5 Then Locate 1, 2: Print "Kutkuta pollaria riimuilla.": KET = KET + 1: KEN(X, Y) = 0
If KEN(X, Y) = 6 Then Locate 1, 2: Print "Teroita veitsesi mummoihin.": KNI = KNI + 1: KEN(X, Y) = 0
If KEN(X, Y) = 7 Then Locate 1, 2: Print "L”ysit kiven.Onnitteluni.": KI = KI + 1: KEN(X, Y) = 0
If KEN(X, Y) = 8 Then Locate 1, 2: Print "Camel boots,miehille,jotka kusevat omille jaloileen.": KEN(X, Y) = 0: KA = KA + 1
If KEN(X, Y) = 9 Then Locate 1, 2: Print "Kalasi lemuaa.": PU = PU + 1: KEN(X, Y) = 0
If KEN(X, Y) = 10 Then Locate 1, 2: Print "Omppo.Sen voi sy”d„, jos uskaltaa.": OM = OM + 1: KEN(X, Y) = 0
If KEN(X, Y) = 11 Then Locate 1, 2: Print "L”ysit banaanin. Kokeileppas sy”d„ se.": BA = BA + 1: KEN(X, Y) = 0
If KEN(X, Y) = 12 Then If PIS > SIP Then Locate 1, 2: Print "Ker„„ pisteet ensin....": X = X + XX: Y = Y + YY
If KEN(X, Y) = 13 Then SIP = SIP + 1: KEN(X, Y) = 0: Locate 1, 1: Print "Bona"
If KEN(X, Y) = 15 Then Locate 1, 2: Print "Pamppu. Mahtaakohan t„m„ olla kadoksissa joltain poltsulta ?": PA = PA + 1: KEN(X, Y) = 0
If KEN(X, Y) = 14 Then Locate 1, 2: Print "Poltsu pamputtaa ,ja se v„s„hti.": KEN(X, Y) = 15: HY = 1: GoSub KUOL: X = X + XX: Y = Y + YY: ALL = ALL + 1
If KEN(X, Y) = 16 Then Locate 1, 2: Print "Kappas, lonkan mittainen !!!": KEN(X, Y) = 0: LON = LON + 1: GoSub TULOSTUS
If KEN(X, Y) = 17 Then Locate 1, 2: GoSub APU: KEN(X, Y) = 0:
If KEN(X, Y) = 18 Then GoSub ALKO: Return
If KEN(X, Y) = 19 Then Locate 1, 2: Print "K-RAUDAN  sein„ on vahvempaa tekoa,kuin p„„si.Et p„„ssyt sis„„n.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 20 Then Locate 1, 2: Print "K-RAUTAA ei ole viel„ ohjelmoitu loppuun. Kokeile uudempaa versiota.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 23 Then Locate 1, 2: RAN = Int(Rnd * 30): Print "Kukkarosi lihavoituu "; RAN; " markalla.": KEN(X, Y) = 0: MONEY = MONEY + RAN
If KEN(X, Y) = 22 Then Locate 1, 2: GoSub MUMMO: Return
If KEN(X, Y) = 24 Then Locate 1, 2: Print "Divarin sein„ on vankkumaton,vaikka kuinka yrit„t k„vell„ sit„ p„in.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 25 Then GoSub DIVARI
If KEN(X, Y) = 26 Then Locate 1, 2: Print "Iskun sein„ on iskunkest„v„,et p„„ssyt sis„„n.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 27 Then Locate 1, 2: Print "Emme myy juopoille.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 28 Then Locate 1, 2: Print "Huomaat sen liian my”h„„n... liukastut paskaan.": VOIMA = VOIMA - 1: KEN(X, Y) = 0
If KEN(X, Y) = 29 Then Locate 1, 2: Print "Ly”t p„„t„si sein„„n.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 30 Then Locate 1, 2: Print "Putkaan otetaan kyll„ juoppoja, tuletko (K/E) ?": GoSub PUTKA
If KEN(X, Y) = 32 Then Locate 1, 2: If BEER > 0 Then Print "Huitaiset yk„„,joka p”llii sulta yhen kaljan.": YKA = 0: KEN(YKX, YKY) = 0: Locate YKY, YKX: Print " ": BEER = BEER - 1 Else Print "M„t„t yk„„ sikana naamaan.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 31 Then Locate 1, 2: Print "Jos haluat putkaan,mene poliisiaseman ovesta.": X = X + XX: Y = Y + YY
If KEN(X, Y) = 33 Then Locate 1, 2: Print "Hakkaat hullun lailla vankikarkuria. ": X = X + XX: Y = Y + YY: KONP = KONP - 1: If KONP < 1 Then CONMAN = 0: Locate 1, 1: Print "Hakkasit vankikarkurin kuoliaaksi...                                 ": KEN(KONX, KONY) = 0: Locate KONY, KONX: Print " ": ALL = ALL + 1
If KEN(X, Y) = 34 Then Locate 1, 2: Print "Pusket SHITHEADia naamaan,seh„n murjoo sua.";: X = X + XX: Y = Y + YY: SPOW = SPOW - 1: RAN = Int(Rnd * 100): If RAN < 50 Then Print "Nakkasit „ij„„ munille !!!": SPOW = SPOW - 1: If SPOW < 1 Then Locate SKIY, SKIX: Print " ": ALL = ALL + 1




XX = 0: YY = 0: If PIS = SIP And KEN(X, Y) = 12 Then NEX = 1: HY = 0: Locate 1, 1: Print "Seuraavaan kentt„„n...": VELO = 0: CONMAN = 0: PUTKIS = 0: YKA = 0: PUTKA = 0
GoSub TULOSTUS
Return
KUOL:
For T = 1 To POL
    If XXXX(T) = X And YYYY(T) = Y Then ELO(T) = 0
Next: R = Int(Rnd * 3) + 1: VOIMA = VOIMA - R
Color 10: Locate Y, X: Print "!"
Return

POLLAR: For G = 1 To POL
    If ELO(G) > 0 Then GoSub POLLA
Next G
Return
        
POLLA:
      
Locate YYYY(G), XXXX(G): Print " ": KEN(XXXX(G), YYYY(G)) = 0
If X + 1 < XXXX(G) Then XXXX(G) = XXXX(G) - 1: If KEN(XXXX(G), YYYY(G)) <> 0 Then XXXX(G) = XXXX(G) + 1: Locate 26, 40
If X - 1 > XXXX(G) Then XXXX(G) = XXXX(G) + 1: If KEN(XXXX(G), YYYY(G)) <> 0 Then XXXX(G) = XXXX(G) - 1: Locate 26, 40
If Y - 1 > YYYY(G) Then YYYY(G) = YYYY(G) + 1: If KEN(XXXX(G), YYYY(G)) <> 0 Then YYYY(G) = YYYY(G) - 1: Locate 26, 40
If Y + 1 < YYYY(G) Then YYYY(G) = YYYY(G) - 1: If KEN(XXXX(G), YYYY(G)) <> 0 Then YYYY(G) = YYYY(G) + 1: Locate 26, 40
Locate YYYY(G), XXXX(G): Color 10: Print "C": KEN(XXXX(G), YYYY(G)) = 14
      
Return
POLL: Rem ********* POLLARIN HAKKAUS **********
Color 14
If HHH = 1 Then Locate 1, 1: If TULO < 1 Then Print "Pollari varotteleepi sua,ettei kantsi r”ttel”id„.": Return Else Return
If TULO < 1 Then Locate 1, 1: Print "Pollari pamputtaa !!!"
R = Int(Rnd * 3) + 1: VOIMA = VOIMA - R: GoSub TULOSTUS
Return
       
GOVER: A$ = ""
GOOVER: A$ = InKey$: If A$ = "" Then GoTo GOOVER
Cls
Print "Hakkaamasi mummelit  :"; KM
Print "Kaikki hakatut oliot :"; ALL
Print "-----------------------------"
Print "Yhteispistetilanteesi:"; KM + ALL
For T = 1 To 5: Print: Next
Print "Kiitos PULTSARIn pelaamisesta !"
Print "Uuvvestaan (k/e)?"
GOO: A$ = InKey$: If A$ = "K" Or A$ = "k" Then GoTo 1
If A$ = "E" Or A$ = "e" Then End
GoTo GOO

APU: Rem ************** TAIVAALLISET TEKSTIT *****************
Randomize Timer
RAN = Int(Rnd * 36) + 1
Restore
For T = 1 To RAN
    Read D$
Next
Print "Odottelet v„h„n aikaa,jonka j„lkeen kuulet taivaallisen „„nen :       <LISŽŽ>"
AP: AA$ = InKey$: If AA$ = "" Then GoTo AP
Locate 1, 1: Print Tab(80);: Locate 1, 1
Print D$
Return

Data "Mummoilla on rahaa."
Data "Poliisit on k„yny punttisalilla."
Data "Alcossa myyd„„n tavaraa."
Data "Varkaille kannattaa antaa pamppua."
Data "Sven Olof on niit„ miehi„ ."
Data "Vankikarkuri kupsahtaa vasta kolmannella pampulla"
Data "PULTSARI   -  Written By Mika Maaranen                          28.12.1991"
Data "Kalaa kannatta sy”d„ harkiten."
Data "Sinulla on 50 % mahdollisuus,siihen,ett„ omena on herkullinen"
Data "Poliisien kanssa ei ole leikkimist„. "
Data "Mummot saattavat huitaista k„silaukulla."
Data "T„ss„kin peliss„ alco ottaa ylihintaa."
Data "Varkaat voi nuijia vain heitt„m„ll„."
Data "Vauhtia siihen peliin !!"
Data "Mit„s oikein kuppaat ? Olet hakannut vasta pari mummoa !"
Data "Mummelit v„istelev„t heittoja."
Data "Skinhead on saattanut sekoontua el„imiin..."
Data "Kankkusta kannatta v„ltell„. Tai ainakin t„ss„ peliss„."
Data "K-RAUTAa ei ole ohjelmoitu loppuun."
Data "Tavatessasi kaljapullon etiketti„ , tavaat n„in L-A-H-D-E-N IIIIIIIIIIIIIIII"
Data "Varo paskaa.Siihen liukastuu."
Data "Oispa t„ss„ peliss„ vessa."
Data "Alco ei myy p„ihtyneille."
Data "Divari ei osta juopuneilta."
Data "PULTSARI:  -  Idea By Mika -,Jari -,Tero Maaranen and Kimmo Korhonen."
Data "Mummot on vaarallisia."
Data "AIDS tappaa."
Data "Jos otat,et aja."
Data "Poliisi asemalla on putka,kokeile sit„(vasta viidennen levelin j„lkeen)"
Data "Burb !!"
Data "T„m„ ohjelma on laadittu k„ytt„m„ll„ Qbasic 4.5 - ohjelmaa."
Data "Vankikarkurit ne heittelleepi ilkeesti kivill„ - jos ne l”yt„„ sellasen."
Data "By Miksu Soft."
Data "Tiesittek”,ett„ radiota kuunnellessa on hyv„ ohjelmoida ?"
Data "Konekirjoitustaidosta on hy”ty„."
Data "Mietit viime”ist„ untasi,ja sitten muistat,ettet nukkunut viime y”n„."
Data "kakakakalaa."
Data "Muistat KIROSANAN,jonka joku joskus sulle selitti : P E L I  K O N S O L I"
Data "Ultima 6 onnesi uus"
Data "Kysy uudempaa versiota,soita 12345... (hp vain vaivaiset 999999999999999 mk.)"
Data "We wish you a merry cristmas and a happy new year...1992."
Data "Zak McKracken voisi olla t„n pelin kunkku."
Data "Leisuresuit Larryss„ on puhtia."
Data "Putkassa mieli sekoaa."
Data "Punaista p„in !!!"
Data "Vinkki :poliisit vaarantavat terveytesi !!!"
Data "The Last Zone...sorry s””r,The Last Bone."
Data ""




ALKO: X = X + XX: Y = Y + YY: Color 10
If PROM > 14 Then Locate 1, 1: Print "Vartija heitti sinut pihalle. Olet juopunut.": Return
If PROM < 5 Then Locate 1, 1: Print "Teh„n olette selv„. Meill„ sen voi korjata.": GoSub NAPISKA
If MONEY < 15 Then Locate 1, 1: Color 12: Print "Putiikki on kiinni.": Return
Locate 1, 1: Print "Ja mit„s herra ostaa ,kalja vai lonkka (1/2)?"
ALK: A$ = InKey$: If A$ <> "1" And A$ <> "2" Then GoTo ALK
If A$ = "1" Then BEER = BEER + 1: MONEY = MONEY - 10: GoSub TULOSTUS: Locate 1, 1: Print "Kiitos,ja hyv„„ p„iv„n jatkoa.                                  ": Return
MONEY = MONEY - 15: LON = LON + 1: Locate 1, 1: Print "A wise choice.                                              ": GoSub TULOSTUS: Return
Stop
VARAS: Rem ******************* VARKAAN LIIKKEET ***********************
Locate VY, VX: Print " "
KEN(VX, VY) = 0
If X + 1 < VX Then VX = VX - 1: If KEN(VX, VY) <> 0 Then GoSub TESTI: VX = VX + 1
If X - 1 > VX Then VX = VX + 1: If KEN(VX, VY) <> 0 Then GoSub TESTI: VX = VX - 1
If Y - 1 > VY Then VY = VY + 1: If KEN(VX, VY) <> 0 Then GoSub TESTI: VY = VY - 1
If Y + 1 < VY Then VY = VY - 1: If KEN(VX, VY) <> 0 Then GoSub TESTI: VY = VY + 1
If VALI = 0 Then KEN(VX, VY) = 21: Locate VY, VX: Print "V"
If KEN(X, Y - 1) = 21 Or KEN(X + 1, Y - 1) = 21 Then GoSub EDD: GoTo SEUR
If KEN(X + 1, Y) = 21 Or KEN(X + 1, Y + 1) = 21 Or KEN(X, Y + 1) = 21 Or KEN(X, Y + 1) = 21 Or KEN(X - 1, Y + 1) = 21 Or KEN(X - 1, Y) = 21 Or KEN(X - 1, Y - 1) = 21 Then GoSub EDD
GoTo SEUR
EDD:
      
      
Locate 1, 1: Color 10: Print "K„teiskassasi kutistuu !!!":
RAN = Int(Rnd * 15) + 1: MONEY = MONEY - RAN:
Locate VY, VX: Print " ": KEN(VX, VY) = 0: VELO = 0:
If MONEY < 0 Then MONEY = 0
Return
SEUR:
      
If VALI = 1 Then VELO = 0: KEN(VX, VY) = 0: VALI = 0
Return
TESTI: Rem ********* TESTATAAN, MIHIN VARAS MENEE ************
If KEN(VX, VY) = 28 Then VALI = 1: KEN(VX, VY) = 0: Locate VY, VX: Print " ": Locate 1, 1: Print "Varas liukastui paskaan,ja kuoli.": Return
If KEN(VX, VY) = 22 Then KEN(VX, VY) = 0: Locate 1, 1: Print "Mummo j„i varkaan k„siin.": Locate VY, VX: Print " ": Return
If KEN(VX, VY) = 14 Then VALI = 1: Locate 1, 1: Print "Poliisi pid„tt„„ varkaan.": VELO = 0: Return
Return

MUMMO: Rem ***** LY™K™ MUMMELI ? ******
HY = 1
RAN = Int(Rnd * 100): If RAN < 50 Then Print "El„kell„inen hakkaa sua k„silaukullaan !!!": RAN = Int(Rnd * 2) + 1: VOIMA = VOIMA - RAN: GoSub TULOSTUS: X = X + XX: Y = Y + YY: Return
Print "Mukiloit mummon kumoon. Haa,lompsa !!!"
KEN(X, Y) = 23: Locate Y, X: Color 10: Print "$"
KM = KM + 1: ALL = ALL + 1: X = X + XX: Y = Y + YY: Return

POLSKI: Rem ***TARKISTELLAAN,ONKO KENTIES POLLARI LŽHELLŽ PAMPUTUS AIKEISSA***
If HY = 0 Then HHH = 1 Else HHH = 0
If KEN(X + 1, Y) = 14 Then GoSub POLL
If KEN(X + 1, Y + 1) = 14 Then GoSub POLL
If KEN(X, Y + 1) = 14 Then GoSub POLL
If KEN(X - 1, Y + 1) = 14 Then GoSub POLL
If KEN(X - 1, Y) = 14 Then GoSub POLL
If KEN(X - 1, Y - 1) = 14 Then GoSub POLL
If KEN(X, Y - 1) = 14 Then GoSub POLL
If KEN(X + 1, Y - 1) = 14 Then GoSub POLL
Return
NAPISKA: A$ = InKey$: If A$ = "" Then GoTo NAPISKA
Return
DIVARI: Rem ************  MYYNTI DIVARISSA  *************
Color 10
If PROM > 15 Then Locate 1, 1: Print "Teh„n olette r„k„k„nniss„. Ulos t„„lt„ !"
If KA < 1 And KNI < 1 And PA < 1 And KET < 1 Then Locate 1, 1: Print "Emme osta teilt„ mit„„n. Out."
Locate 1, 1: Print "Mit„s myisit ? Bootsit=1  Pamppu=2  Ketjut=3  Veitsi=4  ": X = X + XX: Y = Y + YY
DIV: A$ = InKey$: If A$ <> "1" And A$ <> "2" And A$ <> "3" And A$ <> "4" Then GoTo DIV
Locate 1, 1: Print "                                                               "
If A$ = "1" Then If KA < 1 Then Locate 1, 1: Print "Eih„n teill„ ole kenki„ edes jaloissanne.": Return Else Locate 1, 1: MONEY = MONEY + 100: Print "Bootsit meni satasella.": KA = KA - 1: GoSub TULOSTUS: Return
If A$ = "2" Then If PA < 1 Then Locate 1, 1: Print "Joo,montahan noita pamppuja sinulla onkin.": Return Else Locate 1, 1: RAN = Int(Rnd * 40) + 1: Print "Pamppu hurahti "; RAN; " markalla.": PA = PA - 1: MONEY = MONEY + RAN: GoSub TULOSTUS: Return
If A$ = "3" Then If KET < 1 Then Locate 1, 1: Print "Mutta , kun sinulla ei ole yhtik„s yht„„n ketjun p„tk„„k„„n.": Return Else Locate 1, 1: Print "Ketjut onnistuit myym„„n kolmella kympill„.": MONEY = MONEY + 30: KET = KET - 1: GoSub TULOSTUS: Return
If KNI < 1 Then Locate 1, 1: Print "Et omista veitsi„.": Return
KNI = KNI - 1
Locate 1, 1: RAN = Int(Rnd * 50) + 1: Print "Voitit veitell„s "; RAN; " markkaa.": MONEY = MONEY + RAN: GoSub TULOSTUS: Return

PUTKA: Rem ******** PUTKA *********
A$ = InKey$: If A$ <> "k" And A$ <> "K" And A$ <> "E" And A$ <> "e" Then GoTo PUTKA
If A$ = "E" Or A$ = "e" Then Locate 1, 1: Print "No h„ivy sitten !!!                                               ": Y = Y + YY: Return
Locate 1, 1: Print "Putkassa istuskellessasi huomaat parin tunnin p„„st„ miten          <LISŽŽ>   "
PUTK: A$ = InKey$: If A$ = "" Then GoTo PUTK
Locate 1, 1: Print "P„„si alkaa selvet„,ja miten kankkunen tulee. Game over !!!!!              "
PUTT: A$ = InKey$: If A$ = "" Then GoTo PUTT
Locate 1, 1: Print "Uudestaan (k/e)?"
PU: A$ = InKey$: If A$ = "E" Or A$ = "e" Then Cls: Screen 0: End
If A$ = "K" Or A$ = "k" Then GoTo 1
GoTo PU
    
YKA: Rem *********** YKA *************
Color 14
If KEN(X, Y - 1) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
If KEN(X + 1, Y - 1) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
If KEN(X + 1, Y) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
If KEN(X + 1, Y + 1) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
If KEN(X, Y + 1) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
If KEN(X - 1, Y + 1) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
If KEN(X - 1, Y) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
If KEN(X - 1, Y - 1) = 32 Then Locate 1, 1: Print "Yk„ pyyt„„ yht„ kaljaa, annatko(k/e)?": GoSub KYSY: Return
Color 15: KEN(YKX, YKY) = 0: Locate YKY, YKX: Print " "
If X + 1 < YKX Then YKX = YKX - 1: If KEN(YKX, YKY) <> 0 Then YKX = YKX + 1
If X - 1 > YKX Then YKX = YKX + 1: If KEN(YKX, YKY) <> 0 Then YKX = YKX - 1
If Y - 1 > YKY Then YKY = YKY + 1: If KEN(YKX, YKY) <> 0 Then YKY = YKY - 1
If Y + 1 < YKY Then YKY = YKY - 1: If KEN(YKX, YKY) <> 0 Then YKY = YKY + 1
KEN(YKX, YKY) = 32: Locate YKY, YKX: Print "˜"
Return


KYSY: A$ = InKey$: If A$ <> "K" And A$ <> "k" And A$ <> "E" And A$ <> "e" Then GoTo KYSY
If A$ = "E" Or A$ = "e" Then Locate 1, 1: Print "Yk„ taputteleepi palleaasi.                        ": VOIMA = VOIMA - 1: GoSub TULOSTUS: Return
Locate 1, 1: Print "Yk„ kiitteleepi,ja katoaa.                                        ": BEER = BEER - 1: GoSub TULOSTUS: Return

KONNA: Rem ******************** VANKIKARKURI ********************
Color 14
If KEN(X, Y - 1) = 33 Then Locate 1, 1: Print "Vankikarkuri l„ksytt„„ sua.": GoSub KONN: Return
If KEN(X + 1, Y - 1) = 33 Then Locate 1, 1: Print "Vankikarkuri muiluttaa sinua.": GoSub KONN: Return
If KEN(X + 1, Y) = 33 Then Locate 1, 1: Print "Vankikarkuri l„ksytt„„ sua.": GoSub KONN: Return
If KEN(X + 1, Y + 1) = 33 Then Locate 1, 1: Print "Vankikarkuri muiluttaa sinua.": GoSub KONN: Return
If KEN(X, Y + 1) = 33 Then Locate 1, 1: Print "Vankikarkuri l„ksytt„„ sua.": GoSub KONN: Return
If KEN(X - 1, Y + 1) = 33 Then Locate 1, 1: Print "Vankikarkuri muiluttaa sinua.": GoSub KONN: Return
If KEN(X - 1, Y) = 33 Then Locate 1, 1: Print "Vankikarkuri l„ksytt„„ sua.": GoSub KONN: Return
If KEN(X - 1, Y - 1) = 33 Then Locate 1, 1: Print "Vankikarkuri muiluttaa sinua.": GoSub KONN: Return
KEN(KONX, KONY) = 0: Locate KONY, KONX: Print " "
If X + 1 < KONX Then KONX = KONX - 1: If KEN(KONX, KONY) <> 0 Then GoSub TESTII: KONX = KONX + 1
If X - 1 > KONX Then KONX = KONX + 1: If KEN(KONX, KONY) <> 0 Then GoSub TESTII: KONX = KONX - 1
If Y - 1 > KONY Then KONY = KONY + 1: If KEN(KONX, KONY) <> 0 Then GoSub TESTII: KONY = KONY - 1
If Y + 1 < KONY Then KONY = KONY - 1: If KEN(KONX, KONY) <> 0 Then GoSub TESTII: KONY = KONY + 1
If VALI = 0 Then KEN(KONX, KONY) = 33: Locate KONY, KONX: Print "E"
If VALI = 1 Then VALI = 0: KEN(KONX, KONY) = 0
Return
TESTII: Rem **** TESTATAAN,MINNE KARKURI MENEE ****
If KEN(KONX, KONY) = 14 Then VALI = 1: Locate 1, 1: Print "Poliisi pid„tt„„ vankikarkurin.": CONMAN = 0: Return
If KEN(KONX, KONY) = 3 Then Locate 1, 1: Print "Vankikarkuri ahmii kiekuralenkin.": KEN(KONX, KONY) = 0: Return
If KEN(KONX, KONY) = 7 Then Locate 1, 1: Print "Vankikarkuri nappaa kiven.": VANK = 1: KEN(KONX, KONY) = 0: Return
Return
KONN: RAN = Int(Rnd * 3) + 1: VOIMA = VOIMA - RAN: GoSub TULOSTUS: Return
VANHEITTO: Rem ********** KARKURI HEITTŽŽ KIVEN *************
Color 10: Locate 1, 1: Print "Vankikarkuri heitt„„ kivell„ !!!": Color 15
VALX = KONX: VALY = KONY: If Y = KONY Then GoTo AX
If Y < KONY Then HEIY = -1 Else HEIY = 1
HHHH: If VALX = X And VALY = Y Then GoTo OKEI
Locate VALY, VALX: Print " ": Locate KONY, KONX: Print "E"
VALY = VALY + HEIY: Locate VALY, VALX: Print "*"
For T = 1 To 500: Next
GoTo HHHH
AX: If X < KONX Then HEIX = -1 Else HEIX = 1
H: If VALX = X And VALY = Y Then GoTo OKEI
Locate VALY, VALX: Print " ": Locate KONY, KONX: Print "E"
VALX = VALX + HEIX: Locate VALY, VALX: Print "*"
For T = 1 To 500: Next
GoTo H

OKEI: VANK = 0: RAN = Int(Rnd * 5) + 1: VOIMA = VOIMA - RAN: GoSub TULOSTUS
Return
     
SKINHEAD: Rem ***** KALJUN LIIKKEET *****

If KEN(X, Y - 1) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Skinhead murjoo sinuu apinan raivolla.": GoSub KONNN: Return Else GoSub KONNN: Return
If KEN(X + 1, Y - 1) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Skinhetula murjoo sua apinan raivolla.": GoSub KONNN: Return Else GoSub KONNN: Return
If KEN(X + 1, Y) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Kalju ukkeli murjoo sinua apinan raivolla.": GoSub KONNN: Return Else GoSub KONNN: Return
If KEN(X + 1, Y + 1) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Skinukulele murjoo sinua apinana.": GoSub KONNN: Return Else GoSub KONNN: Return
If KEN(X, Y + 1) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Skinhead murjoo sinua apinan raivolla.": GoSub KONNN: Return Else GoSub KONNN: Return
If KEN(X - 1, Y + 1) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Skinhead muiluttaa sinua.": GoSub KONNN: Return Else GoSub KONNN: Return
If KEN(X - 1, Y) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Skinhead l„ksytt„„ sua.": GoSub KONNN: Return Else GoSub KONNN: Return
If KEN(X - 1, Y - 1) = 34 Then Locate 1, 1: If TULO < 1 Then Print "Skinhead muiluttaa sinua apinan raivolla.": GoSub KONNN: Return Else GoSub KONNN: Return
KEN(SKIX, SKIY) = 0: Locate SKIY, SKIX: Print " "
If X + 1 < SKIX Then SKIX = SKIX - 1: If KEN(SKIX, SKIY) <> 0 Then SKIX = SKIX + 1
If X - 1 > SKIX Then SKIX = SKIX + 1: If KEN(SKIX, SKIY) <> 0 Then SKIX = SKIX - 1
If Y - 1 > SKIY Then SKIY = SKIY + 1: If KEN(SKIX, SKIY) <> 0 Then SKIY = SKIY - 1
If Y + 1 < SKIY Then SKIY = SKIY - 1: If KEN(SKIX, SKIY) <> 0 Then SKIY = SKIY + 1
KEN(SKIX, SKIY) = 34: Locate SKIY, SKIX: Print "O"
       
Return
  
KONNN: RAN = Int(Rnd * 3) + 1: VOIMA = VOIMA - RAN: GoSub TULOSTUS: Return

HORJUNTA: Rem ******** HORJUMINEN ***********
Locate Y, X: Print " "
RAN = Int(Rnd * 100): If RAN < 40 Then Color 14: Locate Y, X: Print "ê": Return
RAN = Int(Rnd * 100): If RAN < 50 Then XX = -1 Else XX = 1
X = X + XX: If KEN(X, Y) <> 0 Then XX = XX * -1: GoSub REAGOINTI
RAN = Int(Rnd * 100): If RAN < 40 Then Locate Y, X: Color 14: Print "ê": Return
RAN = Int(Rnd * 100): If RAN < 50 Then YY = -1 Else YY = 1
Y = Y + YY: If KEN(X, Y) <> 0 Then YY = YY * -1: GoSub REAGOINTI
Color 14: Locate Y, X: Print "ê"
Return

SONTA: Rem ************** MINNE TULEE YRJ™ *****************
RAN = Int(Rnd * 100): If RAN < 50 Then ZX = -1 Else ZX = 1
RAN = Int(Rnd * 100): If RAN < 50 Then ZY = -1 Else ZY = 1
If KEN(X + ZX, Y + ZY) = 22 Then Locate 1, 1: Color 14: Print "Yk„s meni mummon niskoille. Se kuoli."
If KEN(X + ZX, Y + ZY) = 14 Then Locate 1, 1: Color 14: Print "Polsu tukehtui yk„„s,muut pollarit ei taija tyk„t„ susta.": HY = 1
If KEN(X + ZX, Y + ZY) <> 0 And KEN(X + ZX, Y + ZY) <> 22 And KEN(X + ZX, Y + ZY) <> 14 Then GoTo JEPULIS
Return
JEPULIS:
KEN(X + ZX, Y + ZY) = 28: Locate Y + ZY, X + ZX: Color 14: Print "‘"
Return
