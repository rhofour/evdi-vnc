# evdi-vnc
A minimalist utility to start up a VNC server as a secondary screen using EVDI.

# Usage
Make sure the evdi kernel module is loaded with: `modprobe evdi`

Run: `sudo ./evdi-vnc`

On another device connect with any standard vnc client.

# Building
Simply run `make`

# Dependencies
evdi-vnc has only two dependencies: [libvncserver](https://github.com/LibVNC/libvncserver) and
[evdi](https://github.com/DisplayLink/evdi).

libvncserver can likely be installed from your package manager.

libevdi is currently statically linked with evdi-vnc, but you'll also need install the evdi
kernel module in order to use evdi-vnc.

# License
This code is licensed under the GNU Public License v2. See the LICENSE for the full license text.
