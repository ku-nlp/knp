echo off

..\makemat
if errorlevel 1 goto error

for %%f in (*.dic) do ..\makeint %%f
if errorlevel 1 goto error

copy /b *.int jumandic.txt
if errorlevel 1 goto error

del *.int
if errorlevel 1 goto error

..\dicsort jumandic.txt > jumandic.dat
if errorlevel 1 goto error

del jumandic.txt
if errorlevel 1 goto error

..\makepat
if errorlevel 1 goto error

echo JUMAN‚Ì«‘‚ªì¬‚³‚ê‚Ü‚µ‚½B
goto end

:error
echo «‘‚Ìì¬‚É¸”s‚µ‚Ü‚µ‚½B
goto end

:end
