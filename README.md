![andromeda](https://user-images.githubusercontent.com/16405698/65393541-89490480-dd8a-11e9-92a3-727799c30b02.png)

`Andromeda` makes initial reverse engineering work of `Android` applications bit faster and easier.
Compared to other alternatives, it's written in `C/C++` and has a noticeable performance advantage.

## DEMO
[![demo](https://user-images.githubusercontent.com/16405698/65391224-5a716500-dd6f-11e9-9de3-b3dcbc5e27ad.png)](https://www.youtube.com/watch?v=doeg-tCX-sg)

`The tool is in the early development stage`

## Requirements
- libzip: `sudo apt-get install libz-dev`
- openssl: `sudo apt-get install libssl-dev`

## Compilation Instructions (VS Code)
0. Make a `/bin` folder inside the `Andromeda` folder
1. Open `Andromeda` folder from [Visual Studio Code](https://code.visualstudio.com/) and Navigate to `/Andromeda/Andromeda.cpp`
2. Run C++ build active file (F1 -> Run Task -> clang++ build active file) (`Ctrl + Shift + B`)


## Commands
![commands](https://user-images.githubusercontent.com/16405698/65551555-eaa7d980-df2a-11e9-99a1-d7f51fd12af4.png)

## Author
Lasha Khasaia ([@_qaz_qaz](https://twitter.com/_qaz_qaz))

## Note
I've very limited experience with Android, so if you have time please contribute.

## TODO
* Implement new features
    - Differentiate external and internal libraries
* Document the tool