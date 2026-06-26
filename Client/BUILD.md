# How to build
## You will need:
  1) aarch64-linux-gnu-(gcc/glibc/binutils) cross compiler
  2) cmake + make
  3) v4l-utils for aarch64-linux (see [building v4l-utils for aarch64](#build-v4l-utils-for-aarch64-linux-cross))

## Clone and cd into the repository
`git@github.com:stigl-cc/robot.git`  
`cd robot`  

## Configure
`cmake -B build`  
Note: If you need compile_commands.json for development, they will be generated at this point  

## Compile
`cmake --build build`  
Your executable should now be visible in `build/client/`  

## Build v4l-utils for aarch64-linux cross
### 1) Clone v4l2-utils
`git clone --depth 1 https://git.linuxtv.org/v4l-utils.git`  
`cd v4l-utils`  

### 2) Set up a crossfile and configure
Create a file called `crossfile` and paste in something like (this will depend on what your prefix for the toolchain is):  
```
[binaries]
c = '/usr/bin/aarch64-linux-gnu-gcc'
cpp = '/usr/bin/aarch64-linux-gnu-g++'
ar = '/usr/bin/aarch64-linux-gnu-ar'
strip = '/usr/bin/aarch64-linux-gnu-strip'
pkgconfig = '/usr/bin/aarch64-linux-gnu-pkg-config'

[host_machine]
system='linux'
cpu_family='aarch64'
cpu='aarch64'
endian='little'
```

Then, configure it like so:  
`meson setup --prefix / --default-library=both --cross-file crossfile build --reconfigure`  
Note that the `--prefix /` here is for the location inside our toolchain path, not root of your host system.  

### 3) Compile and install v4l-utils
You can easilly compile them as so: `ninja -C build`  
And then install them: `sudo DESTDIR=/usr/aarch64-linux-gnu ninja -C build/ install`  
