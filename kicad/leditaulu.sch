EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:ESP8266
LIBS:leditaulu-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Snooker display"
Date "2017-04-09"
Rev "1.0"
Comp ""
Comment1 "Jaakko Salo / jaakkos@gmail.com"
Comment2 "Sakari Tanskanen / sakari.tanskanen@gmail.com"
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L ESP-12E U?
U 1 1 58EABDA7
P 3050 2325
F 0 "U?" H 3050 2225 50  0000 C CNN
F 1 "ESP-12E" H 3050 2425 50  0000 C CNN
F 2 "" H 3050 2325 50  0001 C CNN
F 3 "" H 3050 2325 50  0001 C CNN
	1    3050 2325
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X06 P?
U 1 1 58EA5906
P 4875 3350
F 0 "P?" H 4875 3700 50  0000 C CNN
F 1 "Programming header" V 4975 3350 50  0000 C CNN
F 2 "" H 4875 3350 50  0000 C CNN
F 3 "" H 4875 3350 50  0000 C CNN
	1    4875 3350
	1    0    0    -1  
$EndComp
Text GLabel 4625 3200 0    39   Input ~ 0
ESP_GPIO0
$Comp
L Earth #PWR?
U 1 1 58EA62EF
P 4675 3100
F 0 "#PWR?" H 4675 2850 50  0001 C CNN
F 1 "Earth" H 4675 2950 50  0001 C CNN
F 2 "" H 4675 3100 50  0000 C CNN
F 3 "" H 4675 3100 50  0000 C CNN
	1    4675 3100
	0    1    1    0   
$EndComp
$Comp
L Earth #PWR?
U 1 1 58EA6329
P 4675 3600
F 0 "#PWR?" H 4675 3350 50  0001 C CNN
F 1 "Earth" H 4675 3450 50  0001 C CNN
F 2 "" H 4675 3600 50  0000 C CNN
F 3 "" H 4675 3600 50  0000 C CNN
	1    4675 3600
	0    1    1    0   
$EndComp
Text GLabel 4700 2225 1    39   Input ~ 0
3V3
Text GLabel 4500 2225 1    39   Input ~ 0
ESP_GPIO0
Text GLabel 5050 2225 1    39   Input ~ 0
3V3
$Comp
L Earth #PWR?
U 1 1 58EA6BBD
P 3950 2725
F 0 "#PWR?" H 3950 2475 50  0001 C CNN
F 1 "Earth" H 3950 2575 50  0001 C CNN
F 2 "" H 3950 2725 50  0000 C CNN
F 3 "" H 3950 2725 50  0000 C CNN
	1    3950 2725
	1    0    0    -1  
$EndComp
$Comp
L C_Small C?
U 1 1 58EA6BF2
P 1625 2825
F 0 "C?" H 1450 2825 50  0000 L CNN
F 1 "10uF" H 1400 2750 50  0000 L CNN
F 2 "" H 1625 2825 50  0000 C CNN
F 3 "" H 1625 2825 50  0000 C CNN
	1    1625 2825
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR?
U 1 1 58EA6CA7
P 1625 2925
F 0 "#PWR?" H 1625 2675 50  0001 C CNN
F 1 "Earth" H 1625 2775 50  0001 C CNN
F 2 "" H 1625 2925 50  0000 C CNN
F 3 "" H 1625 2925 50  0000 C CNN
	1    1625 2925
	1    0    0    -1  
$EndComp
$Comp
L R_Small R?
U 1 1 58EA63CB
P 4700 2325
F 0 "R?" H 4550 2300 50  0000 L CNN
F 1 "10K" H 4500 2375 50  0000 L CNN
F 2 "" H 4700 2325 50  0000 C CNN
F 3 "" H 4700 2325 50  0000 C CNN
	1    4700 2325
	-1   0    0    1   
$EndComp
$Comp
L R_Small R?
U 1 1 58EA6517
P 5050 2325
F 0 "R?" H 4900 2300 50  0000 L CNN
F 1 "10K" H 4850 2375 50  0000 L CNN
F 2 "" H 5050 2325 50  0000 C CNN
F 3 "" H 5050 2325 50  0000 C CNN
	1    5050 2325
	-1   0    0    1   
$EndComp
Text GLabel 1625 2625 1    39   Input ~ 0
3V3
Connection ~ 4500 2425
Wire Wire Line
	4625 3200 4675 3200
Wire Wire Line
	3950 2425 4700 2425
Wire Wire Line
	4500 2225 4500 2425
Wire Wire Line
	5050 2525 5050 2425
Wire Wire Line
	3950 2525 5050 2525
Wire Wire Line
	1625 2725 1625 2625
Connection ~ 1625 2725
Text GLabel 4625 3300 0    39   Input ~ 0
ESP_RST
Wire Wire Line
	4625 3300 4675 3300
NoConn ~ 2800 3225
NoConn ~ 2900 3225
NoConn ~ 3000 3225
NoConn ~ 3100 3225
NoConn ~ 3200 3225
NoConn ~ 3300 3225
Text GLabel 3950 2025 2    39   Input ~ 0
UART_TX
Text GLabel 3950 2125 2    39   Input ~ 0
UART_RX
Text GLabel 4625 3400 0    39   Input ~ 0
UART_RX
Text GLabel 4625 3500 0    39   Input ~ 0
UART_TX
Wire Wire Line
	4625 3400 4675 3400
Wire Wire Line
	4625 3500 4675 3500
Text GLabel 2150 2025 0    39   Input ~ 0
ESP_RST
Text GLabel 1625 2175 1    39   Input ~ 0
3V3
Wire Wire Line
	1625 2225 2150 2225
Wire Wire Line
	1625 2225 1625 2175
NoConn ~ 2150 2125
Wire Wire Line
	1625 2725 2150 2725
Text GLabel 2150 2525 0    39   Input ~ 0
MISO
Text GLabel 2150 2625 0    39   Output ~ 0
MOSI
Text GLabel 3950 2225 2    39   Output ~ 0
SCL
Text GLabel 3950 2325 2    39   BiDi ~ 0
SDA
$EndSCHEMATC
