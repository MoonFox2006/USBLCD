@echo off
if '%1'=='' goto error
echo \(0,16)>COM%1
:loop
echo %time:~0,8%>COM%1
echo %date%>COM%1
timeout 1 >NUL
goto loop
:error
echo Missing COM port number!
