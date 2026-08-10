#!/usr/bin/env bash
# ==============================================================================
#  Neko-Wizard installer
#  download/install.sh
#
#  Single source of truth for every application / driver that Neko-Wizard can
#  install. Each app is a function; the dispatcher at the bottom maps an
#  app-id to its function.
#
#  After a push to `main` this file lives at:
#    https://raw.githubusercontent.com/Neko-Void-Linux/Neko-Wizard/main/download/install.sh
#
#  Usage:
#    bash install.sh <app-id>     install one application
#    bash install.sh list         print all available app-ids
#    bash install.sh help         show this help
#
#  Exit status is 0 on success, non-zero on failure so Neko-Wizard can report
#  the result. Every step logs a line prefixed with [neko] to stdout; the app
#  shows those lines in its progress label.
# ==============================================================================

set -u

log() { printf '[neko] %s\n' "$*"; }
die() { printf '[neko] ERROR: %s\n' "$*" >&2; exit 1; }

usage() {
    printf 'Neko-Wizard installer\n\n'
    printf 'Usage: bash %s <app-id>\n' "$0"
    printf '       bash %s list\n\n' "$0"
    printf 'Run "bash %s list" to see every available app-id.\n' "$0"
}

list_apps() {
    log "Available app-ids:"
    printf '  %s\n' \
        steam portproton heroic lutris hytale trinity prismlauncher pineconemc protonup faugus \
        reaper obs kdenlive openshot vlc audacity ardour blender \
        krita gimp inkscape \
        spotify vesktop waterfox brave zerotierone telegram vivaldi chromium \
        onlyoffice kate libreoffice \
        bluetooth printer amd intel nvidia-open nvidia-latest nvidia-580 nvidia-470 nvidia-390 \
        gufw
}

# ------------------------------------------------------------------------------
# Gaming
# ------------------------------------------------------------------------------

install_steam() {
    log "Installing Steam (void-repo-nonfree + multilib + 32bit libs)..."
    pkexec xbps-install -Sy void-repo-nonfree void-repo-multilib void-repo-multilib-nonfree \
        && pkexec xbps-install -Sy steam-udev-rules libGL-32bit libpulseaudio-32bit libtxc_dxtn-32bit mesa-dri mesa-dri-32bit libdrm-32bit steam-nk
}

install_portproton() {
    log "Installing PortProton..."
    pkexec xbps-install -Sy portproton
}

install_heroic() {
    log "Installing Heroic Games Launcher..."
    pkexec xbps-install -Sy heroic-games
}

install_lutris() {
    log "Installing Lutris..."
    pkexec xbps-install -Sy lutris
}

install_hytale() {
    log "Installing Hytale Launcher (Flatpak)..."
    wget -O /tmp/tmp.flatpak https://launcher.hytale.com/builds/release/linux/amd64/hytale-launcher-latest.flatpak \
        && flatpak install /tmp/tmp.flatpak -y
}

install_trinity() {
    log "Installing Trinity Launcher (Flatpak)..."
    flatpak install com.trench.trinity.launcher -y
}

install_prismlauncher() {
    log "Installing PrismLauncher..."
    pkexec xbps-install -Sy PrismLauncher
}

install_pineconemc() {
    # PineconeMC ships as an AppImage; AppImageLauncher Lite integrates it into
    # the desktop menu. Both are user-level (no pkexec needed).
    local dir="$HOME/apps"
    local launcher="$dir/appimagelauncher-lite.AppImage"
    local pinecone="$dir/PineconeMC-Linux-x86_64.AppImage"

    log "Downloading AppImageLauncher..."
    mkdir -p "$dir" || die "Could not create $dir"
    curl -fsSL -o "$launcher" \
        "https://github.com/TheAssassin/AppImageLauncher/releases/download/v3.0.0-beta-3/appimagelauncher-lite-3.0.0-beta-2-gha287-x86_64.AppImage" \
        || die "Failed to download AppImageLauncher"
    chmod +x "$launcher"

    log "Downloading PineconeMC..."
    curl -fsSL -o "$pinecone" \
        "https://github.com/ElyPrismLauncher/Launcher/releases/download/11.0.3/PineconeMC-Linux-x86_64.AppImage" \
        || die "Failed to download PineconeMC"
    chmod +x "$pinecone"

    log "Integrating PineconeMC into the desktop (AppImageLauncher)..."
    "$launcher" cli integrate "$pinecone"
}

install_protonup() {
    log "Installing ProtonUp-Qt..."
    pkexec xbps-install -Sy protonup-qt
}

install_faugus() {
    log "Installing Faugus Launcher..."
    pkexec xbps-install -Sy faugus-launcher
}

# ------------------------------------------------------------------------------
# Audio & Video editing
# ------------------------------------------------------------------------------

install_reaper() {
    log "Installing Reaper (Flatpak)..."
    flatpak install flathub fm.reaper.Reaper -y
}

install_obs() {
    log "Installing OBS Studio..."
    pkexec xbps-install -Sy obs
}

install_kdenlive() {
    log "Installing Kdenlive..."
    pkexec xbps-install -Sy kdenlive
}

install_openshot() {
    log "Installing OpenShot..."
    pkexec xbps-install -Sy openshot
}

install_vlc() {
    log "Installing VLC..."
    pkexec xbps-install -Sy vlc
}

install_audacity() {
    log "Installing Audacity..."
    pkexec xbps-install -Sy audacity
}

install_ardour() {
    log "Installing Ardour..."
    pkexec xbps-install -Sy ardour
}

install_blender() {
    log "Installing Blender..."
    pkexec xbps-install -Sy blender
}

# ------------------------------------------------------------------------------
# Drawing and Image editing
# ------------------------------------------------------------------------------

install_krita() {
    log "Installing Krita..."
    pkexec xbps-install -Sy krita
}

install_gimp() {
    log "Installing GIMP..."
    pkexec xbps-install -Sy gimp
}

install_inkscape() {
    log "Installing Inkscape..."
    pkexec xbps-install -Sy inkscape
}

# ------------------------------------------------------------------------------
# Social apps and Internet
# ------------------------------------------------------------------------------

install_spotify() {
    log "Installing Spotify (Flatpak)..."
    flatpak install flathub com.spotify.Client -y
}

install_vesktop() {
    log "Installing Vesktop..."
    pkexec xbps-install -Sy vesktop
}

install_waterfox() {
    log "Installing Waterfox..."
    pkexec xbps-install -Sy waterfox
}

install_brave() {
    log "Installing Brave Browser..."
    pkexec xbps-install -Sy brave-bin
}

install_zerotierone() {
    log "Installing ZeroTier One..."
    pkexec xbps-install -Sy zerotierone
}

install_telegram() {
    log "Installing Telegram..."
    pkexec xbps-install -Sy telegram-desktop
}

install_vivaldi() {
    log "Installing Vivaldi..."
    pkexec xbps-install -Sy vivaldi
}

install_chromium() {
    log "Installing Chromium..."
    pkexec xbps-install -Sy chromium
}

# ------------------------------------------------------------------------------
# Text editing and documents
# ------------------------------------------------------------------------------

install_onlyoffice() {
    # OnlyOffice ships as an AppImage; AppImageLauncher Lite integrates it into
    # the desktop menu. Both are user-level (no pkexec needed).
    # Note: "cli integrate" works without systemd (important on Void/runit).
    local dir="$HOME/apps"
    local launcher="$dir/appimagelauncher-lite.AppImage"
    local editors="$dir/DesktopEditors-x86_64.AppImage"

    log "Downloading AppImageLauncher..."
    mkdir -p "$dir" || die "Could not create $dir"
    curl -fsSL -o "$launcher" \
        "https://github.com/TheAssassin/AppImageLauncher/releases/download/v3.0.0-beta-3/appimagelauncher-lite-3.0.0-beta-2-gha287-x86_64.AppImage" \
        || die "Failed to download AppImageLauncher"
    chmod +x "$launcher"

    log "Downloading OnlyOffice..."
    curl -fsSL -o "$editors" \
        "https://github.com/ONLYOFFICE/appimage-desktopeditors/releases/download/v9.4.0/DesktopEditors-x86_64.AppImage" \
        || die "Failed to download OnlyOffice"
    chmod +x "$editors"

    log "Integrating OnlyOffice into the desktop (AppImageLauncher)..."
    "$launcher" cli integrate "$editors"
}

install_kate() {
    log "Installing Kate..."
    pkexec xbps-install -Sy kate
}

install_libreoffice() {
    log "Installing LibreOffice..."
    pkexec xbps-install -Sy libreoffice
}

# ------------------------------------------------------------------------------
# Drivers (each one pulls all its related packages)
# ------------------------------------------------------------------------------

install_bluetooth() {
    log "Enabling Bluetooth support..."
    curl -fsSL -o /tmp/bluetooth-enable.sh https://codeberg.org/Neko-Void/bluetooth-enabler/raw/branch/main/install.sh \
        && pkexec bash /tmp/bluetooth-enable.sh
}

install_printer() {
    log "Enabling Printer support..."
    curl -fsSL -o /tmp/printer-enable.sh https://codeberg.org/Neko-Void/printer-enable/raw/branch/main/enable.sh \
        && pkexec bash /tmp/printer-enable.sh
}

install_amd() {
    log "Installing AMD drivers..."
    pkexec xbps-install -Sy mesa-dri mesa-dri-32bit mesa-vulkan-radeon mesa-vulkan-radeon-32bit linux-firmware-amd
}

install_intel() {
    log "Installing Intel drivers..."
    pkexec xbps-install -Sy mesa-dri mesa-dri-32bit mesa-vulkan-intel mesa-vulkan-intel-32bit linux-firmware-intel libva-intel-driver intel-media-driver mesa-intel-dri-32bit mesa-intel-dri
}

install_nvidia_open() {
    log "Installing NVIDIA (open kernel modules)..."
    curl -fsSL -o /tmp/nvidia-install.sh https://codeberg.org/Neko-Void/nvidia-support/raw/branch/main/install.sh \
        && pkexec bash /tmp/nvidia-install.sh open
}

install_nvidia_latest() {
    log "Installing NVIDIA (proprietary, latest)..."
    curl -fsSL -o /tmp/nvidia-install.sh https://codeberg.org/Neko-Void/nvidia-support/raw/branch/main/install.sh \
        && pkexec bash /tmp/nvidia-install.sh latest
}

install_nvidia_580() {
    log "Installing NVIDIA (proprietary, 580 series)..."
    curl -fsSL -o /tmp/nvidia-install.sh https://codeberg.org/Neko-Void/nvidia-support/raw/branch/main/install.sh \
        && pkexec bash /tmp/nvidia-install.sh 580
}

install_nvidia_470() {
    log "Installing NVIDIA (proprietary, 470 series)..."
    curl -fsSL -o /tmp/nvidia-install.sh https://codeberg.org/Neko-Void/nvidia-support/raw/branch/main/install.sh \
        && pkexec bash /tmp/nvidia-install.sh 470
}

install_nvidia_390() {
    log "Installing NVIDIA (proprietary, 390 series)..."
    curl -fsSL -o /tmp/nvidia-install.sh https://codeberg.org/Neko-Void/nvidia-support/raw/branch/main/install.sh \
        && pkexec bash /tmp/nvidia-install.sh 390
}

# ------------------------------------------------------------------------------
# Security
# ------------------------------------------------------------------------------

install_gufw() {
    log "Installing UFW + GUFW firewall..."
    pkexec xbps-install -Sy ufw gufw \
        && pkexec ln -s /etc/sv/ufw /var/service/ \
        && pkexec ufw enable
}

# ------------------------------------------------------------------------------
# Dispatcher
# ------------------------------------------------------------------------------

APP="${1:-}"

case "$APP" in
    steam)         install_steam ;;
    portproton)    install_portproton ;;
    heroic)        install_heroic ;;
    lutris)        install_lutris ;;
    hytale)        install_hytale ;;
    trinity)       install_trinity ;;
    prismlauncher) install_prismlauncher ;;
    pineconemc)    install_pineconemc ;;
    protonup)      install_protonup ;;
    faugus)        install_faugus ;;

    reaper)        install_reaper ;;
    obs)           install_obs ;;
    kdenlive)      install_kdenlive ;;
    openshot)      install_openshot ;;
    vlc)           install_vlc ;;
    audacity)      install_audacity ;;
    ardour)        install_ardour ;;
    blender)       install_blender ;;

    krita)         install_krita ;;
    gimp)          install_gimp ;;
    inkscape)      install_inkscape ;;

    spotify)       install_spotify ;;
    vesktop)       install_vesktop ;;
    waterfox)      install_waterfox ;;
    brave)         install_brave ;;
    zerotierone)   install_zerotierone ;;
    telegram)      install_telegram ;;
    vivaldi)       install_vivaldi ;;
    chromium)      install_chromium ;;

    onlyoffice)    install_onlyoffice ;;
    kate)          install_kate ;;
    libreoffice)   install_libreoffice ;;

    bluetooth)     install_bluetooth ;;
    printer)       install_printer ;;
    amd)           install_amd ;;
    intel)         install_intel ;;
    nvidia-open)   install_nvidia_open ;;
    nvidia-latest) install_nvidia_latest ;;
    nvidia-580)    install_nvidia_580 ;;
    nvidia-470)    install_nvidia_470 ;;
    nvidia-390)    install_nvidia_390 ;;

    gufw)          install_gufw ;;

    list)          list_apps ;;
    help|-h|--help) usage ;;
    "")            die "Missing app-id. Usage: $0 <app-id> (or '$0 list' to see the available ids)" ;;
    *)             die "Unknown app-id: '$APP'. Run '$0 list' to see the available ids." ;;
esac
rc=$?

if [ "$rc" -ne 0 ]; then
    printf '[neko] Installation of "%s" failed (exit code %s).\n' "$APP" "$rc" >&2
fi
exit "$rc"
