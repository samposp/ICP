## Setup

WINDOWS: 

Download from https://opencv.org/releases/ 

Extract opencv-4.10.0-windows.exe to \<opencv path>

Add/change user specific variable:
 Ovladaci panely -> uzivatelske ucty -> Student -> Zmenit promenne prostredi
(Control panel   -> User accounts    -> Student -> Change system variables)

Add: 
 * Name:  OPENCV_DIR
 * Value: \<opencv path>

Modify variable PATH
 * Add item:  %OPENCV_DIR%\x64\vc16\bin
