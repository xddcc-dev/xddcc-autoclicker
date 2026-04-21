# REQUIREMENTS
Vs2022
Qt

# how to build?

1. search for "Developer Command Prompt for VS 2022"
2. open it
3. Run the commands below (make sure to edit it to have the correct stuff)

cd C:\Users\<your user here>\path\to\XDDCCClicker
cmake -B build -DCMAKE_PREFIX_PATH="path/to/Qt/6.11.0/msvc2022_64" 
cmake --build build --config Release


now go to build folder and release, then add qt files. done!
