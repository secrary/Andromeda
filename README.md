![andromeda](https://user-images.githubusercontent.com/16405698/65393541-89490480-dd8a-11e9-92a3-727799c30b02.png)

`Andromeda` makes initial reverse engineering work of `Android` applications bit faster and easier.
Compared to other alternatives, it's written in `C/C++` and has a noticeable performance advantage.

## DEMO
[![demo](https://user-images.githubusercontent.com/16405698/65391224-5a716500-dd6f-11e9-9de3-b3dcbc5e27ad.png)](https://www.youtube.com/watch?v=doeg-tCX-sg)

`The tool is in the early development stage`

# Requirements
libZip: sudo apt-get install libz-dev 
libOpenSSL: sudo apt-get install libssl-dev

# Compilation Instructions (VS Code)
0. Make a /bin folder inside the Andromeda folder
1. Navigate to /Andromeda/Andromeda.cpp (open in VS Code)
2. Run C++ build active file (F1 -> Run Task -> clang++ build active file)


## Author
Lasha Khasaia ([@_qaz_qaz](https://twitter.com/_qaz_qaz))

## Note
I've very limited experience with Android, so if you have time please contribute.
## TODO

* Implement new features
    - get list of `lib` files and dump them
    - Differentiate external and internal libraries
    - List all classes (not only entry point ones)
* Document the tool