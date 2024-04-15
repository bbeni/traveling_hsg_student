# Traveling salesperson problems.

![tsp usa cities](/usa.png)

Task for ESC202 implemented using C. The only dependency is the cl.exe compiler on windows - we are just using [stb](https://github.com/nothings/stb) (header only) style libraries.

## Results
### Burma14

Random inital tour on top (length: 7969) <br />
Optimized tour on bottom (length: 3454)

![burma 14](/burma_random.png)
![burma 14](/burma_optimized.png)

### Berlin52

Random inital tour on top (length: 29716) <br />
Optimized tour on bottom (length: 11160)

![berlin](/berlin_random.png)
![berlin](/berlin_optimized.png)

### USA13509

Too big.. <br />
Random inital tour on top (length: 216647526) <br />
Optimized tour on bottom (length: 55982547)

![berlin](/usa_random.png)
![berlin](/usa_optimized.png)


## Compilation

We are using the [nob.h](https://github.com/tsoding/musializer/blob/master/src/nob.h) build system and msvc compiler on windows. Run in a Developer PowerShell ([MSVC setup](https://code.visualstudio.com/docs/cpp/config-msvc))

```console
cl.exe nob.c
nob.exe             # use just this for rebuilds!
main.exe
```


### Compilation Linux

To compile on linux uncomment the macro `#define LINUX` and comment out the other 2. Then run in terminal

```console
cc -o nob nob.c
./nob                 # use just this for rebuilds!
./main
```

### Compilation Mac OS

To compile on linux uncomment the macro `#define MACOS` and comment out the other 2. Then run in terminal

```console
clang -o nob nob.c
./nob                 # use just this for rebuilds!
./main
```
