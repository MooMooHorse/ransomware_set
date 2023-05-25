@echo off
setlocal enabledelayedexpansion

for %%F in (*) do (
  set "filename=%%~nF"
  set "newname=!filename:_=!"
  if not "!newname!"=="%%~nF" (
    set "extension=%%~xF"
    ren "%%F" "!newname!!extension!"
  )
)

endlocal
