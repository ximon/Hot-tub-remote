EESchema Schematic File Version 4
LIBS:Interface-cache
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L power:VCC #PWR?
U 1 1 5D70837B
P 3650 1750
F 0 "#PWR?" H 3650 1600 50  0001 C CNN
F 1 "VCC" H 3667 1923 50  0000 C CNN
F 2 "" H 3650 1750 50  0001 C CNN
F 3 "" H 3650 1750 50  0001 C CNN
	1    3650 1750
	1    0    0    -1  
$EndComp
$Comp
L Logic_LevelTranslator:SN74LVC1T45DCK U1
U 1 1 5E9CDB6D
P 5200 1950
F 0 "U1" H 5644 1996 50  0000 L CNN
F 1 "SN74LVC1T45DCK" H 5644 1905 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-363_SC-70-6" H 5200 1500 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/sn74lvc1t45.pdf" H 4300 1300 50  0001 C CNN
	1    5200 1950
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 2150 4800 2150
Wire Wire Line
	5300 1550 5300 1500
Wire Wire Line
	5100 1550 5100 1500
Text GLabel 4400 1850 0    50   Input ~ 0
RX
Text GLabel 4400 2000 0    50   Input ~ 0
D1
Text GLabel 5800 1850 2    50   Input ~ 0
DATA
Text GLabel 5800 2350 2    50   Input ~ 0
GND
Text GLabel 5900 1500 2    50   Input ~ 0
5V
Text GLabel 4450 1500 0    50   Input ~ 0
3V3
Text GLabel 4400 2150 0    50   Input ~ 0
D2
Wire Wire Line
	4650 1850 4650 1950
Wire Wire Line
	4650 1950 4800 1950
Wire Wire Line
	4650 2000 4650 1950
Wire Wire Line
	4400 2000 4650 2000
Connection ~ 4650 1950
Wire Wire Line
	4400 1850 4650 1850
Wire Wire Line
	5100 1500 4450 1500
Wire Wire Line
	5200 2350 5800 2350
Wire Wire Line
	5300 1500 5900 1500
Wire Wire Line
	5600 1950 5650 1950
Wire Wire Line
	5650 1950 5650 1850
Wire Wire Line
	5650 1850 5800 1850
$Comp
L Device:CP C1
U 1 1 5D708E3A
P 3650 1900
F 0 "C1" H 3768 1946 50  0000 L CNN
F 1 "1000UF" H 3768 1855 50  0000 L CNN
F 2 "" H 3688 1750 50  0001 C CNN
F 3 "~" H 3650 1900 50  0001 C CNN
	1    3650 1900
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 5D708717
P 3650 2050
F 0 "#PWR?" H 3650 1800 50  0001 C CNN
F 1 "GND" H 3655 1877 50  0000 C CNN
F 2 "" H 3650 2050 50  0001 C CNN
F 3 "" H 3650 2050 50  0001 C CNN
	1    3650 2050
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 5E9D72B6
P 5200 2450
F 0 "#PWR?" H 5200 2200 50  0001 C CNN
F 1 "GND" H 5205 2277 50  0000 C CNN
F 2 "" H 5200 2450 50  0001 C CNN
F 3 "" H 5200 2450 50  0001 C CNN
	1    5200 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	5200 2450 5200 2350
Connection ~ 5200 2350
$EndSCHEMATC
