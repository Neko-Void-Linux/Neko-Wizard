# download/

This folder holds the installer script used by Neko-Wizard.

| File          | Description                                                        |
|---------------|--------------------------------------------------------------------|
| `install.sh`  | Master installer. One function per app; run with `bash install.sh <app-id>`. |

After a push to `main`, the script is available at:

```
https://raw.githubusercontent.com/Neko-Void-Linux/Neko-Wizard/main/download/install.sh
```

Neko-Wizard downloads it with `curl` and runs it with the app-id of the
application to install, so every install command lives in one place and can be
updated without rebuilding the app.

## Usage

```bash
bash install.sh steam     # install one application
bash install.sh list      # list every available app-id
bash install.sh help      # show help
```

## Adding a new app

1. Add an `install_<id>()` function to `install.sh`.
2. Map the id in the dispatcher `case` block.
3. Point to it from `src/apps_manager.c` with `INSTALL_APP("<id>")`.
