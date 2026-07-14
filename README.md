# Neko-Wizard

Neko-Wizard is a straightforward wizard application and driver installer designed specifically for Void Linux.


## Build and Run

To compile and launch the application, ensure you have Meson installed. Execute the following command from the root of the cloned repository:

```bash
meson setup build --reconfigure && meson compile -C build && ./build/neko-store

```