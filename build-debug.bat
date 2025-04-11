@echo OFF
setlocal EnableDelayedExpansion

REM Capture start time
set "startTime=%TIME%"

REM Run your command
make all

REM wow thanks a lot chat gpt :)
REM Capture end time
set "endTime=%TIME%"

REM Show start and end time
echo Start Time: %startTime%
echo End Time:   %endTime%

REM Calculate elapsed time
call :GetDuration "%startTime%" "%endTime%"
exit /b

:GetDuration
setlocal EnableDelayedExpansion

set "start=%~1"
set "end=%~2"

REM Parse start time
for /F "tokens=1-4 delims=:., " %%a in ("%start%") do (
    set /a "startH=1%%a %% 100, startM=1%%b %% 100, startS=1%%c %% 100, startMS=1%%d %% 100"
)

REM Parse end time
for /F "tokens=1-4 delims=:., " %%a in ("%end%") do (
    set /a "endH=1%%a %% 100, endM=1%%b %% 100, endS=1%%c %% 100, endMS=1%%d %% 100"
)

REM Convert to milliseconds
set /a startTotal=(startH*3600000 + startM*60000 + startS*1000 + startMS)
set /a endTotal=(endH*3600000 + endM*60000 + endS*1000 + endMS)

REM Handle rollover at midnight
if !endTotal! lss !startTotal! set /a endTotal+=86400000

set /a duration=endTotal - startTotal

REM Show result in seconds and milliseconds
set /a seconds=duration / 1000
set /a millis=duration %% 1000

echo Elapsed Time: !seconds!.!millis! seconds

endlocal
goto :eof

@echo OFF



