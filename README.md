# Traveling salesperson problems.

![tsp usa cities](/usa.png)

Task for ESC202 implemented using C. The only dependency is the cl.exe compiler on windows - we are just using [stb](https://github.com/nothings/stb) (header only) style libraries.

## Compilation

We are using the [nob.h](https://github.com/tsoding/musializer/blob/master/src/nob.h) build system and msvc compiler on windows for now.

```console
cl.exe nob.c
nob.exe      # use just this for rebuilds!
main.exe
```
