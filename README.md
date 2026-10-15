# CPT205 2D Coursework

An interactive 2D greeting card celebrating XJTLU's 20th anniversary, built with
C++, OpenGL and FreeGLUT for CPT205. Find the kitty with a lantern, guide it to
the switch, and light up the campus with fireworks.

## Project Files

- `task1.cpp` — application source code.
- `XJTLU.exe` — prebuilt Windows executable.
- `CPT205_Greeting_Card_Report.pdf` — coursework report.

## Run the Included Executable

On Windows, open a terminal in the project root and run:

```powershell
.\XJTLU.exe
```

## Controls

- **Left-click the kitty on the cover** after the prompt appears to start.
- **Move the mouse** to control the lantern and find the kitty.
- **W/A/S/D or arrow keys** — move the kitty toward the switch after finding it.
- **Space** — activate the switch when prompted and start the celebration.
- **Q** — quit the application.

## Build from Source

Compile `task1.cpp` with a Windows C++ compiler and the OpenGL/FreeGLUT headers
and libraries configured. The scene is drawn in code and does not require
external texture files.
