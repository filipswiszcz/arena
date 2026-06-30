# arena

A 2D battle arena (reinforcement learning).

![GIF](res/gameplay.gif)

## Dependencies
### Common
* `glfw3`
* `OpenGL`
### System-specific
#### Linux
* `build-essential` or equivalent (GCC, Make)
* `GLEW`
#### macOS
* `Xcode Command Line Tools`
* `Homebrew` (recommended for GLFW installation: `brew install glfw`)
> [!NOTE]
> macOS natively supports OpenGL up to version 4.1
#### Windows
* `Microsoft Visual Studio with C++ tools` (2019 or newer recommended)
* `vcpkg` (recommended for GLFW installation: `vcpkg install glfw3`)
* `GLEW`

## Compiling
#### Linux / macOS
```bash
git clone https://github.com/filipswiszcz/arena.git
cd arena
./build.sh [debug/dev/release]
```
#### Windows
```bash
git clone https://github.com/filipswiszcz/arena.git
cd arena
.\build.bat [debug/dev/release]
```
> [!NOTE]
> Run the executable from the `bin/` directory (e.g. `./bin/game` on Linux/macOS or `.\bin\game.exe` on Windows)

## Key bindings
|  |  |
| :--- | :--- |
| `Ctrl` + `Q` | Quit game
| `Ctrl` + `S` | Save game (current training state)
| `P` | Pause game
| `R` | Resume game
| `T` | Switch player control (manual (you) and auto (ML network))
| `ESC` | Reset game turn
| `W` | Jump (supports double jump)
| `S` | Crouch
| `A`/`D` | Move left/right
| `Left arrow`/`Right arrow` | Face left/right
| `Shift` | Dash
| `SPACE` | Shoot