# Reference Clock Firmware

Simple firmware emitting messages in one second intervals. Currently, as soon as the MCU is
powered on, the firmware will start emitting reference signals.  In our deployment, we used a STM32F401CC
development board. Other microprocessors from the STM32F4XX product line should work without
problems as well.  Since the firmware currently is quite primitive and really only depends on a
single timer, it should be trivial to port the code to other STM32 microcontrollers supporting the
[Cube Framework](https://www.st.com/en/development-tools/stm32cubemx.html).

