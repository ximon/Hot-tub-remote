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
L Logic_LevelTranslator:SN74LVC1T45DCK U1
U 1 1 5E9CDB6D
P 5200 1950
F 0 "U1" H 5644 1996 50  0000 L CNN
F 1 "SN74LVC1T45" H 5644 1905 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-363_SC-70-6" H 5200 1500 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/sn74lvc1t45.pdf" H 4300 1300 50  0001 C CNN
	1    5200 1950
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 2150 4800 2150
Wire Wire Line
	5100 1550 5100 1500
Text GLabel 4400 1850 0    50   Input ~ 0
RX
Text GLabel 4400 1950 0    50   Input ~ 0
D1
Text GLabel 5800 1850 2    50   Input ~ 0
DATA
Text GLabel 5800 2350 2    50   Input ~ 0
GND
Text GLabel 4450 1500 0    50   Input ~ 0
3V3
Text GLabel 4400 2150 0    50   Input ~ 0
D2
Wire Wire Line
	5100 1500 4450 1500
Wire Wire Line
	5200 2350 5800 2350
Wire Wire Line
	5600 1950 5650 1950
Wire Wire Line
	5650 1950 5650 1850
Wire Wire Line
	5650 1850 5800 1850
$Comp
L Device:CP C1
U 1 1 5D708E3A
P 3800 1850
F 0 "C1" H 3918 1896 50  0000 L CNN
F 1 "1000UF" H 3918 1805 50  0000 L CNN
F 2 "" H 3838 1700 50  0001 C CNN
F 3 "~" H 3800 1850 50  0001 C CNN
	1    3800 1850
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
$Comp
L Diode:1N4448W D1
U 1 1 5EA5CF14
P 4600 1850
F 0 "D1" H 4600 1633 50  0000 C CNN
F 1 "1N4448" H 4600 1724 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123" H 4600 1675 50  0001 C CNN
F 3 "https://www.vishay.com/docs/85722/1n4448w.pdf" H 4600 1850 50  0001 C CNN
	1    4600 1850
	-1   0    0    1   
$EndComp
Wire Wire Line
	4750 1850 4750 1950
Wire Wire Line
	4750 1950 4800 1950
Wire Wire Line
	4400 1950 4750 1950
Connection ~ 4750 1950
Wire Wire Line
	4450 1850 4400 1850
Text GLabel 5850 1350 2    50   Input ~ 0
5V
Text GLabel 4450 2350 0    50   Input ~ 0
GND
Wire Wire Line
	4450 2350 5200 2350
Text Notes 4250 1250 0    50   ~ 0
ESP8266
Text Notes 5450 1250 0    50   ~ 0
Hot Tub 3-pin
Text GLabel 4400 1350 0    50   Input ~ 0
5V
Wire Wire Line
	4400 1350 5300 1350
Connection ~ 5300 1350
Wire Wire Line
	5300 1350 5900 1350
Wire Wire Line
	5300 1350 5300 1550
$Comp
L power:GND #PWR?
U 1 1 5D708717
P 3800 2000
F 0 "#PWR?" H 3800 1750 50  0001 C CNN
F 1 "GND" H 3805 1827 50  0000 C CNN
F 2 "" H 3800 2000 50  0001 C CNN
F 3 "" H 3800 2000 50  0001 C CNN
	1    3800 2000
	1    0    0    -1  
$EndComp
Text GLabel 3800 1550 1    50   Input ~ 0
5V
Wire Wire Line
	3750 1550 3800 1550
Wire Wire Line
	3800 1550 3800 1700
$EndSCHEMATC
