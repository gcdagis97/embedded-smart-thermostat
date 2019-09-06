# embedded-smart-thermostat
A system was modeled to emulate modern HVAC engineering on an elementary level and is included in a report. The software and hardware needed to simulate a basic thermostat temperature-sensing system is laid out for replication purposes. This report also contains details of how the hardware and software interact, the theory and research behind the project, possible improvements of future iterations, fully commented and comprehensible code, and finally the conclusions made as a result.

The system controls physical heating / cooling apparatus by communicating from the Tiva board to a Raspberry Pi. A user controls the desired temperature in the interrupt driven system through the use of 2 switch buttons on the physical circuit. The appropriate heating / cooling apparatus is then activated in real-time, aided by fuzzy logic. Analog to digital conversion (ADC) with temperature sensor was used to realize full-scale resolution .  Periodic interrupts were required for sampling of ADC values. Live feedback from temperature sensor is indicated through the use of a dummy terminal; TeraTerm 4.98 was used in this case. The Pi also actively polled the temperatures and updated a Node-red module via a wifi connection.

Platform: Tiva™ C Series TM4C123GH6PM, Raspberry Pi 3 B+

IDE: KEIL uVision4 / KEIL uVision 5, Mu

Language: C, Python 3
