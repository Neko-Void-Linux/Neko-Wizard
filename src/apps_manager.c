#include "apps_manager.h"
#include <stddef.h>

/* Neko-Wizard installs everything through a single remote installer script
 * (download/install.sh in this repo). Same pattern used by the drivers:
 * download it and run it with the app-id of what we want to install.
 * It stays up to date after every push to main, no rebuild needed. */
#define NEKO_SCRIPT_URL "https://raw.githubusercontent.com/Neko-Void-Linux/Neko-Wizard/main/download/install.sh"
#define INSTALL_APP(id) "curl -fsSL -o /tmp/neko-install.sh " NEKO_SCRIPT_URL " && bash /tmp/neko-install.sh " id

static AppInfo apps[] = {
    // Gaming
    {"Steam", "steam.png", INSTALL_APP("steam"), GROUP_GAMING, FALSE, FALSE},
    {"PortProton", "portproton.png", INSTALL_APP("portproton"), GROUP_GAMING, FALSE, FALSE},
    {"Heroic Games Launcher", "heroic.png", INSTALL_APP("heroic"), GROUP_GAMING, FALSE, FALSE},
    {"Lutris", "lutris.png", INSTALL_APP("lutris"), GROUP_GAMING, FALSE, FALSE},
    {"Hytale", "hytale.png", INSTALL_APP("hytale"), GROUP_GAMING, FALSE, FALSE},
    {"Trinity Launcher", "trinity.png", INSTALL_APP("trinity"), GROUP_GAMING, FALSE, FALSE},
    {"PrismLauncher", "prismlauncher.png", INSTALL_APP("prismlauncher"), GROUP_GAMING, FALSE, FALSE},
    {"PineconeMC", "elyprismlauncher.png", INSTALL_APP("pineconemc"), GROUP_GAMING, FALSE, FALSE},
    {"ProtonUp-Qt", "protonup-qt.png", INSTALL_APP("protonup"), GROUP_GAMING, FALSE, FALSE},
    {"Faugus Launcher", "faugus.png", INSTALL_APP("faugus"), GROUP_GAMING, FALSE, FALSE},
    // Audio and Video editing
    {"Reaper", "reaper.png", INSTALL_APP("reaper"), GROUP_AUDIO_VIDEO, FALSE, FALSE},
    {"OBS Studio", "obs.png", INSTALL_APP("obs"), GROUP_AUDIO_VIDEO, FALSE, FALSE},
    {"Kdenlive", "kdenlive.png", INSTALL_APP("kdenlive"), GROUP_AUDIO_VIDEO, FALSE, FALSE},
    {"OpenShot", "org.openshot.OpenShot.png", INSTALL_APP("openshot"), GROUP_AUDIO_VIDEO, FALSE, FALSE},
    {"VLC", "vlc.png", INSTALL_APP("vlc"), GROUP_AUDIO_VIDEO, FALSE, FALSE},
    {"Audacity", "audacity-logo.png", INSTALL_APP("audacity"), GROUP_AUDIO_VIDEO, FALSE, FALSE},
    {"Ardour", "ardour.png", INSTALL_APP("ardour"), GROUP_AUDIO_VIDEO, FALSE, FALSE},
    {"Blender", "Blender.png", INSTALL_APP("blender"), GROUP_AUDIO_VIDEO, FALSE, FALSE},

    // Drawing and Image Editing
    {"Krita", "krita.png", INSTALL_APP("krita"), GROUP_DRAWING_IMAGE, FALSE, FALSE},
    {"GIMP", "gimp.png", INSTALL_APP("gimp"), GROUP_DRAWING_IMAGE, FALSE, FALSE},
    {"Inkscape", "Inkscape.png", INSTALL_APP("inkscape"), GROUP_DRAWING_IMAGE, FALSE, FALSE},

    // Social Apps
    {"Spotify", "Spotify_icon.svg.png", INSTALL_APP("spotify"), GROUP_SOCIAL, FALSE, FALSE},
    {"Vesktop", "vesktop.png", INSTALL_APP("vesktop"), GROUP_SOCIAL, FALSE, FALSE},
    {"Waterfox", "waterfox.png", INSTALL_APP("waterfox"), GROUP_SOCIAL, FALSE, FALSE},
    {"Brave", "brave.png", INSTALL_APP("brave"), GROUP_SOCIAL, FALSE, FALSE},
    {"Zerotierone", "zerotierone.png", INSTALL_APP("zerotierone"), GROUP_SOCIAL, FALSE, FALSE},
    {"Telegram", "telegram.png", INSTALL_APP("telegram"), GROUP_SOCIAL, FALSE, FALSE},
    {"Vivaldi", "vivaldi.png", INSTALL_APP("vivaldi"), GROUP_SOCIAL, FALSE, FALSE},
    {"Chromium", "chromium.png", INSTALL_APP("chromium"), GROUP_SOCIAL, FALSE, FALSE},

    // Text editing and documents
    {"OnlyOffice", "onlyoffice.png", INSTALL_APP("onlyoffice"), GROUP_TEXT_DOCUMENTS, FALSE, FALSE},
    {"Kate", "org.kde.kate.desktop.png", INSTALL_APP("kate"), GROUP_TEXT_DOCUMENTS, FALSE, FALSE},
    {"LibreOffice", "Libre-Office.png", INSTALL_APP("libreoffice"), GROUP_TEXT_DOCUMENTS, FALSE, FALSE},

    // Drivers (grouped - each slot installs all related packages)
    {"Bluetooth","bluetooth.png", INSTALL_APP("bluetooth"), GROUP_DRIVERS, FALSE, FALSE},
    {"Printer Support", "print.png", INSTALL_APP("printer"), GROUP_DRIVERS, FALSE, FALSE},
    {"AMD Drivers", "amd.png", INSTALL_APP("amd"), GROUP_DRIVERS, FALSE, FALSE},
    {"Intel Drivers", "intel.png", INSTALL_APP("intel"), GROUP_DRIVERS, FALSE, FALSE},
    {"Nvidia Open", "nvidia.png", INSTALL_APP("nvidia-open"), GROUP_DRIVERS, FALSE, FALSE},
    {"Nvidia Proprietary Lastest", "nvidia.png", INSTALL_APP("nvidia-latest"), GROUP_DRIVERS, FALSE, FALSE},
    {"Nvidia Proprietary 580", "nvidia.png", INSTALL_APP("nvidia-580"), GROUP_DRIVERS, FALSE, FALSE},
    {"Nvidia Proprietary 470", "nvidia.png", INSTALL_APP("nvidia-470"), GROUP_DRIVERS, FALSE, FALSE},
    {"Nvidia Proprietary 390", "nvidia.png", INSTALL_APP("nvidia-390"), GROUP_DRIVERS, FALSE, FALSE},

    //SECURITY SECTION
    {"GUFW (FIREWALL)", "firewall.png", INSTALL_APP("gufw"), GROUP_SECURITY, FALSE, FALSE},
    {"CLAMAV + CLAMUI", "clamav.png", INSTALL_APP("clamav"), GROUP_SECURITY, FALSE, FALSE}
};

GList *get_all_apps(void) {
    GList *list = NULL;
    int num_apps = sizeof(apps) / sizeof(apps[0]);
    for (int i = 0; i < num_apps; i++) {
        list = g_list_append(list, &apps[i]);
    }
    return list;
}

gchar *get_resource_path(const char *rel_path) {
    gchar *exe_path = g_file_read_link("/proc/self/exe", NULL);
    gchar *exe_dir = NULL;

    if (exe_path) {
        exe_dir = g_path_get_dirname(exe_path);
        g_free(exe_path);
    } else {
        exe_dir = g_get_current_dir();
    }

    gchar *path1 = g_build_filename(exe_dir, rel_path, NULL);
    gchar *path2 = g_build_filename(exe_dir, "..", rel_path, NULL);
    gchar *path3 = g_build_filename("/usr/share/neko-store", rel_path, NULL);
    gchar *path4 = g_build_filename("/opt/neko-store", rel_path, NULL);

    g_free(exe_dir);

    if (g_file_test(path1, G_FILE_TEST_EXISTS)) {
        g_free(path2); g_free(path3); g_free(path4);
        return path1;
    }
    if (g_file_test(path2, G_FILE_TEST_EXISTS)) {
        g_free(path1); g_free(path3); g_free(path4);
        return path2;
    }
    if (g_file_test(path3, G_FILE_TEST_EXISTS)) {
        g_free(path1); g_free(path2); g_free(path4);
        return path3;
    }
    if (g_file_test(path4, G_FILE_TEST_EXISTS)) {
        g_free(path1); g_free(path2); g_free(path3);
        return path4;
    }

    g_free(path2); g_free(path3); g_free(path4);

    gchar *path5 = g_build_filename("/home/javierc/Documentos/server/dev/Neko Store", rel_path, NULL);
    if (g_file_test(path5, G_FILE_TEST_EXISTS)) {
        g_free(path1);
        return path5;
    }
    g_free(path5);

    return path1;
}
