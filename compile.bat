set debug=false
if "%debug%"=="false" set window=-mwindows 
g++ -Wall -pthread src\*.cpp -O2 -o release\main.exe %window%-L lib -lglfw3 -lglfw3dll -lglew32 -lglew32s -lopengl32 -lglu32 -lgdi32 --static
cd release
main.exe
pause
