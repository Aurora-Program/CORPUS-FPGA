@echo off
call C:\oss-cad-suite\environment.bat
openFPGALoader.exe -b tangnano9k -f impl\pnr\corpus.fs
exit /b %ERRORLEVEL%
