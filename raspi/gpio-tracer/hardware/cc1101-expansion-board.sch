EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date "15 nov 2012"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L power:+5V #PWR01
U 1 1 580C1B61
P 3100 950
F 0 "#PWR01" H 3100 800 50  0001 C CNN
F 1 "+5V" H 3100 1090 50  0000 C CNN
F 2 "" H 3100 950 50  0000 C CNN
F 3 "" H 3100 950 50  0000 C CNN
	1    3100 950 
	1    0    0    -1  
$EndComp
Wire Wire Line
	3100 950  3100 1100
Wire Wire Line
	3100 1100 2900 1100
Wire Wire Line
	3100 1200 2900 1200
Connection ~ 3100 1100
$Comp
L power:GND #PWR02
U 1 1 580C1D11
P 3000 3150
F 0 "#PWR02" H 3000 2900 50  0001 C CNN
F 1 "GND" H 3000 3000 50  0000 C CNN
F 2 "" H 3000 3150 50  0000 C CNN
F 3 "" H 3000 3150 50  0000 C CNN
	1    3000 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	3000 1300 3000 1700
Wire Wire Line
	3000 2700 2900 2700
Wire Wire Line
	3000 2500 2900 2500
Connection ~ 3000 2700
Wire Wire Line
	3000 2000 2900 2000
Connection ~ 3000 2500
Wire Wire Line
	3000 1700 2900 1700
Connection ~ 3000 2000
$Comp
L power:GND #PWR03
U 1 1 580C1E01
P 2300 3150
F 0 "#PWR03" H 2300 2900 50  0001 C CNN
F 1 "GND" H 2300 3000 50  0000 C CNN
F 2 "" H 2300 3150 50  0000 C CNN
F 3 "" H 2300 3150 50  0000 C CNN
	1    2300 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	2300 3000 2400 3000
Wire Wire Line
	2300 1500 2300 2300
Wire Wire Line
	2300 2300 2400 2300
Connection ~ 2300 3000
Connection ~ 2200 1100
Wire Wire Line
	2200 1900 2400 1900
Wire Wire Line
	2200 1100 2400 1100
Wire Wire Line
	2200 950  2200 1100
$Comp
L power:+3.3V #PWR04
U 1 1 580C1BC1
P 2200 950
F 0 "#PWR04" H 2200 800 50  0001 C CNN
F 1 "+3.3V" H 2200 1090 50  0000 C CNN
F 2 "" H 2200 950 50  0000 C CNN
F 3 "" H 2200 950 50  0000 C CNN
	1    2200 950 
	1    0    0    -1  
$EndComp
Wire Wire Line
	2300 1500 2400 1500
Connection ~ 2300 2300
Wire Wire Line
	2400 1200 1250 1200
Wire Wire Line
	1250 1300 2400 1300
Wire Wire Line
	1250 1400 2400 1400
Wire Wire Line
	2400 1600 1250 1600
Wire Wire Line
	1250 1700 2400 1700
Wire Wire Line
	1250 1800 2400 1800
Wire Wire Line
	1250 2100 2400 2100
Wire Wire Line
	1250 2200 2400 2200
Wire Wire Line
	2400 2400 1250 2400
Wire Wire Line
	1250 2500 2400 2500
Wire Wire Line
	1250 2600 2400 2600
Wire Wire Line
	2400 2700 1250 2700
Wire Wire Line
	1250 2800 2400 2800
Wire Wire Line
	1250 2900 2400 2900
Wire Wire Line
	2900 2800 3950 2800
Wire Wire Line
	2900 2900 3950 2900
Wire Wire Line
	2900 2300 3950 2300
Wire Wire Line
	2900 2400 3950 2400
Wire Wire Line
	2900 2100 3950 2100
Wire Wire Line
	2900 2200 3950 2200
Wire Wire Line
	2900 1800 3950 1800
Wire Wire Line
	2900 1900 3950 1900
Wire Wire Line
	2900 1500 3950 1500
Wire Wire Line
	2900 1600 3950 1600
Wire Wire Line
	2900 1400 3950 1400
Wire Wire Line
	2900 2600 3950 2600
Text Label 1250 1200 0    50   ~ 0
GPIO2(SDA1)
Text Label 1250 1300 0    50   ~ 0
GPIO3(SCL1)
Text Label 1250 1400 0    50   ~ 0
GPIO4(GCLK)
Text Label 1250 1600 0    50   ~ 0
GPIO17(GEN0)
Text Label 1250 1700 0    50   ~ 0
GPIO27(GEN2)
Text Label 1250 1800 0    50   ~ 0
GPIO22(GEN3)
Text Label 1250 2000 0    50   ~ 0
SI
Text Label 1250 2100 0    50   ~ 0
SO
Text Label 1250 2200 0    50   ~ 0
SCK
Text Label 1250 2400 0    50   ~ 0
ID_SD
Text Label 1250 2500 0    50   ~ 0
GPIO5
Text Label 1250 2600 0    50   ~ 0
GPIO6
Text Label 1250 2700 0    50   ~ 0
GPIO13(PWM1)
Text Label 1250 2800 0    50   ~ 0
GPIO19(SPI1_MISO)
Text Label 1250 2900 0    50   ~ 0
GPIO26
Text Label 3950 2900 2    50   ~ 0
GPIO20(SPI1_MOSI)
Text Label 3950 2800 2    50   ~ 0
GPIO16
Text Label 3950 2600 2    50   ~ 0
GPIO12(PWM0)
Text Label 3950 2400 2    50   ~ 0
ID_SC
Text Label 3950 2300 2    50   ~ 0
GPIO7(SPI1_CE_N)
Text Label 3950 2200 2    50   ~ 0
CSN
Text Label 3950 2100 2    50   ~ 0
GPIO25(GEN6)
Text Label 3950 1900 2    50   ~ 0
GPIO24(GEN5)
Text Label 3950 1800 2    50   ~ 0
GPIO23(GEN4)
Text Label 3950 1600 2    50   ~ 0
GPIO18(GEN1)(PWM0)
Text Label 3950 1500 2    50   ~ 0
GPIO15(RXD0)
Text Label 3950 1400 2    50   ~ 0
GPIO14(TXD0)
Wire Wire Line
	3000 1300 2900 1300
Connection ~ 3000 1700
Text Notes 650  7600 0    50   ~ 0
ID_SD and ID_SC PINS:\nThese pins are reserved for HAT ID EEPROM.\n\nAt boot time this I2C interface will be\ninterrogated to look for an EEPROM\nthat identifes the attached board and\nallows automagic setup of the GPIOs\n(and optionally, Linux drivers).\n\nDO NOT USE these pins for anything other\nthan attaching an I2C ID EEPROM. Leave\nunconnected if ID EEPROM not required.
$Comp
L cc1101-expansion-board-rescue:Mounting_Hole-Mechanical MK1
U 1 1 5834FB2E
P 3000 7200
F 0 "MK1" H 3100 7246 50  0000 L CNN
F 1 "M2.5" H 3100 7155 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5" H 3000 7200 60  0001 C CNN
F 3 "" H 3000 7200 60  0001 C CNN
	1    3000 7200
	1    0    0    -1  
$EndComp
$Comp
L cc1101-expansion-board-rescue:Mounting_Hole-Mechanical MK3
U 1 1 5834FBEF
P 3450 7200
F 0 "MK3" H 3550 7246 50  0000 L CNN
F 1 "M2.5" H 3550 7155 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5" H 3450 7200 60  0001 C CNN
F 3 "" H 3450 7200 60  0001 C CNN
	1    3450 7200
	1    0    0    -1  
$EndComp
$Comp
L cc1101-expansion-board-rescue:Mounting_Hole-Mechanical MK2
U 1 1 5834FC19
P 3000 7400
F 0 "MK2" H 3100 7446 50  0000 L CNN
F 1 "M2.5" H 3100 7355 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5" H 3000 7400 60  0001 C CNN
F 3 "" H 3000 7400 60  0001 C CNN
	1    3000 7400
	1    0    0    -1  
$EndComp
$Comp
L cc1101-expansion-board-rescue:Mounting_Hole-Mechanical MK4
U 1 1 5834FC4F
P 3450 7400
F 0 "MK4" H 3550 7446 50  0000 L CNN
F 1 "M2.5" H 3550 7355 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5" H 3450 7400 60  0001 C CNN
F 3 "" H 3450 7400 60  0001 C CNN
	1    3450 7400
	1    0    0    -1  
$EndComp
Text Notes 3000 7050 0    50   ~ 0
Mounting Holes
$Comp
L Connector_Generic:Conn_02x20_Odd_Even P1
U 1 1 59AD464A
P 2600 2000
F 0 "P1" H 2650 3117 50  0000 C CNN
F 1 "Conn_02x20_Odd_Even" H 2650 3026 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x20_P2.54mm_Vertical" H -2250 1050 50  0001 C CNN
F 3 "" H -2250 1050 50  0001 C CNN
	1    2600 2000
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 3000 3950 3000
Text Label 3950 3000 2    50   ~ 0
GPIO21(SPI1_SCK)
Wire Wire Line
	3100 1100 3100 1200
Wire Wire Line
	3000 2700 3000 3150
Wire Wire Line
	3000 2500 3000 2700
Wire Wire Line
	3000 2000 3000 2500
Wire Wire Line
	2300 3000 2300 3150
Wire Wire Line
	2200 1100 2200 1900
Wire Wire Line
	2300 2300 2300 3000
Wire Wire Line
	3000 1700 3000 2000
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J1
U 1 1 60456020
P 8050 1350
F 0 "J1" H 8100 1767 50  0000 C CNN
F 1 "Conn_02x05_Odd_Even" H 8100 1676 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x05_P2.54mm_Vertical" H 8050 1350 50  0001 C CNN
F 3 "~" H 8050 1350 50  0001 C CNN
	1    8050 1350
	1    0    0    -1  
$EndComp
Wire Wire Line
	7850 1150 7550 1150
Wire Wire Line
	7850 1250 7550 1250
Wire Wire Line
	7850 1350 7550 1350
Wire Wire Line
	7850 1450 7550 1450
Wire Wire Line
	7850 1550 7550 1550
Wire Wire Line
	8350 1150 8650 1150
Wire Wire Line
	8350 1250 8650 1250
Wire Wire Line
	8350 1350 8650 1350
Wire Wire Line
	8350 1450 8650 1450
Wire Wire Line
	8350 1550 8650 1550
Text Label 7650 1150 0    50   ~ 0
CH1
Text Label 7650 1250 0    50   ~ 0
CH3
Text Label 7650 1350 0    50   ~ 0
CH5
Text Label 7650 1450 0    50   ~ 0
CH7
Text Label 8400 1150 0    50   ~ 0
CH2
Text Label 8400 1250 0    50   ~ 0
CH4
Text Label 8400 1350 0    50   ~ 0
CH6
Text Label 8400 1450 0    50   ~ 0
CH8
Text Label 8400 1550 0    50   ~ 0
GND
$Comp
L power:GND #PWR07
U 1 1 60463CFC
P 8650 1550
F 0 "#PWR07" H 8650 1300 50  0001 C CNN
F 1 "GND" V 8655 1422 50  0000 R CNN
F 2 "" H 8650 1550 50  0001 C CNN
F 3 "" H 8650 1550 50  0001 C CNN
	1    8650 1550
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J2
U 1 1 6046469D
P 8050 2300
F 0 "J2" H 8100 2717 50  0000 C CNN
F 1 "Conn_02x05_Odd_Even" H 8100 2626 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x05_P2.54mm_Vertical" H 8050 2300 50  0001 C CNN
F 3 "~" H 8050 2300 50  0001 C CNN
	1    8050 2300
	1    0    0    -1  
$EndComp
Wire Wire Line
	7850 2100 7550 2100
Wire Wire Line
	7850 2200 7550 2200
Wire Wire Line
	7850 2300 7550 2300
Wire Wire Line
	7850 2400 7550 2400
Wire Wire Line
	7850 2500 7550 2500
Wire Wire Line
	8350 2100 8650 2100
Wire Wire Line
	8350 2200 8650 2200
Wire Wire Line
	8350 2300 8650 2300
Wire Wire Line
	8350 2400 8650 2400
Wire Wire Line
	8350 2500 8650 2500
Text Label 7650 2100 0    50   ~ 0
VCC
Text Label 8400 2100 0    50   ~ 0
VCC
Text Label 7650 2500 0    50   ~ 0
GND
Text Label 8400 2500 0    50   ~ 0
GND
Text Label 7650 2200 0    50   ~ 0
SI
Text Label 7650 2300 0    50   ~ 0
SO
Text Label 7650 2400 0    50   ~ 0
CSN
Text Label 8400 2200 0    50   ~ 0
SCK
Text Label 8400 2400 0    50   ~ 0
CH8
$Comp
L power:GND #PWR09
U 1 1 60475E59
P 8650 2500
F 0 "#PWR09" H 8650 2250 50  0001 C CNN
F 1 "GND" V 8655 2372 50  0000 R CNN
F 2 "" H 8650 2500 50  0001 C CNN
F 3 "" H 8650 2500 50  0001 C CNN
	1    8650 2500
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR06
U 1 1 6047652B
P 7550 2500
F 0 "#PWR06" H 7550 2250 50  0001 C CNN
F 1 "GND" V 7555 2372 50  0000 R CNN
F 2 "" H 7550 2500 50  0001 C CNN
F 3 "" H 7550 2500 50  0001 C CNN
	1    7550 2500
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR08
U 1 1 60476C2F
P 8650 2100
F 0 "#PWR08" H 8650 1950 50  0001 C CNN
F 1 "+3.3V" V 8665 2228 50  0000 L CNN
F 2 "" H 8650 2100 50  0001 C CNN
F 3 "" H 8650 2100 50  0001 C CNN
	1    8650 2100
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR05
U 1 1 604774AC
P 7550 2100
F 0 "#PWR05" H 7550 1950 50  0001 C CNN
F 1 "+3.3V" V 7565 2228 50  0000 L CNN
F 2 "" H 7550 2100 50  0001 C CNN
F 3 "" H 7550 2100 50  0001 C CNN
	1    7550 2100
	0    -1   -1   0   
$EndComp
$Comp
L power:PWR_FLAG #FLG01
U 1 1 60477FD2
P 10100 1200
F 0 "#FLG01" H 10100 1275 50  0001 C CNN
F 1 "PWR_FLAG" H 10100 1373 50  0000 C CNN
F 2 "" H 10100 1200 50  0001 C CNN
F 3 "~" H 10100 1200 50  0001 C CNN
	1    10100 1200
	1    0    0    -1  
$EndComp
$Comp
L power:PWR_FLAG #FLG02
U 1 1 60478446
P 10550 1200
F 0 "#FLG02" H 10550 1275 50  0001 C CNN
F 1 "PWR_FLAG" H 10550 1373 50  0000 C CNN
F 2 "" H 10550 1200 50  0001 C CNN
F 3 "~" H 10550 1200 50  0001 C CNN
	1    10550 1200
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR010
U 1 1 604794FA
P 10100 1200
F 0 "#PWR010" H 10100 950 50  0001 C CNN
F 1 "GND" H 10105 1027 50  0000 C CNN
F 2 "" H 10100 1200 50  0001 C CNN
F 3 "" H 10100 1200 50  0001 C CNN
	1    10100 1200
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR011
U 1 1 60479C2D
P 10550 1200
F 0 "#PWR011" H 10550 1050 50  0001 C CNN
F 1 "+3.3V" H 10565 1373 50  0000 C CNN
F 2 "" H 10550 1200 50  0001 C CNN
F 3 "" H 10550 1200 50  0001 C CNN
	1    10550 1200
	-1   0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J3
U 1 1 6048B383
P 8100 3200
F 0 "J3" H 8180 3192 50  0000 L CNN
F 1 "Conn_01x08" H 8180 3101 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Horizontal" H 8100 3200 50  0001 C CNN
F 3 "~" H 8100 3200 50  0001 C CNN
	1    8100 3200
	1    0    0    -1  
$EndComp
Wire Wire Line
	7900 2900 7550 2900
Wire Wire Line
	7900 3000 7550 3000
Wire Wire Line
	7900 3100 7550 3100
Wire Wire Line
	7900 3200 7550 3200
Wire Wire Line
	7900 3300 7550 3300
Wire Wire Line
	7900 3400 7550 3400
Wire Wire Line
	7900 3500 7550 3500
Wire Wire Line
	7900 3600 7550 3600
Text Label 7650 2900 0    50   ~ 0
CH1
Text Label 7650 3000 0    50   ~ 0
CH2
Text Label 7650 3100 0    50   ~ 0
CH3
Text Label 7650 3200 0    50   ~ 0
CH4
Text Label 7650 3300 0    50   ~ 0
CH5
Text Label 7650 3400 0    50   ~ 0
CH6
Text Label 7650 3500 0    50   ~ 0
CH7
Text Label 7650 3600 0    50   ~ 0
GND
NoConn ~ 8650 2300
NoConn ~ 7550 1550
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 604A6EFD
P 10550 1750
F 0 "#FLG0101" H 10550 1825 50  0001 C CNN
F 1 "PWR_FLAG" H 10550 1923 50  0000 C CNN
F 2 "" H 10550 1750 50  0001 C CNN
F 3 "~" H 10550 1750 50  0001 C CNN
	1    10550 1750
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0101
U 1 1 604A7A13
P 10550 1750
F 0 "#PWR0101" H 10550 1600 50  0001 C CNN
F 1 "+5V" H 10565 1923 50  0000 C CNN
F 2 "" H 10550 1750 50  0001 C CNN
F 3 "" H 10550 1750 50  0001 C CNN
	1    10550 1750
	-1   0    0    1   
$EndComp
Wire Wire Line
	1250 2000 2400 2000
NoConn ~ 1250 1200
NoConn ~ 1250 1300
NoConn ~ 1250 1400
NoConn ~ 1250 1600
NoConn ~ 1250 1700
NoConn ~ 1250 1800
NoConn ~ 1250 2400
NoConn ~ 1250 2500
NoConn ~ 1250 2600
NoConn ~ 1250 2700
NoConn ~ 1250 2800
NoConn ~ 1250 2900
NoConn ~ 3950 1400
NoConn ~ 3950 1500
NoConn ~ 3950 1600
NoConn ~ 3950 1800
NoConn ~ 3950 1900
NoConn ~ 3950 2100
NoConn ~ 3950 2300
NoConn ~ 3950 2600
NoConn ~ 3950 2800
NoConn ~ 3950 2900
NoConn ~ 3950 3000
NoConn ~ 3950 2400
$EndSCHEMATC
