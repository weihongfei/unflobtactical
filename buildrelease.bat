del /Q Xenowar\*.*
del /Q Xenowar\res\*.*

mkdir Xenowar
mkdir Xenowar\res
copy .\res\*.* Xenowar\res\*.*
copy c:\bin\SDL.dll Xenowar
copy .\win32\Release\UFOAttack.exe Xenowar\Xenowar.exe


