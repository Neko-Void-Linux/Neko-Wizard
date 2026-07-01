#include "mirror_manager.h"

static MirrorInfo mirrors[] = {
    // Tier 1
    {"repo-default.voidlinux.org", "https://repo-default.voidlinux.org/", "Globally Available", "Default (auto-selects best Tier 1)", 1},
    {"repo-fi.voidlinux.org", "https://repo-fi.voidlinux.org/", "Europe", "Helsinki, Finland", 1},
    {"repo-de.voidlinux.org", "https://repo-de.voidlinux.org/", "Europe", "Frankfurt, Germany", 1},
    {"repo-fastly.voidlinux.org", "https://repo-fastly.voidlinux.org/", "Globally Available", "Fastly Global CDN", 1},
    {"mirrors.summithq.com/voidlinux", "https://mirrors.summithq.com/voidlinux/", "North America", "Chicago, USA", 1},

    // Tier 2 - Global
    {"mirrors.cicku.me/voidlinux", "https://mirrors.cicku.me/voidlinux", "Globally Available", "Cloudflare Global CDN", 2},

    // Tier 2 - Asia
    {"mirror.nju.edu.cn/voidlinux", "https://mirror.nju.edu.cn/voidlinux/", "Asia", "China", 2},
    {"mirrors.bfsu.edu.cn/voidlinux", "https://mirrors.bfsu.edu.cn/voidlinux/", "Asia", "Beijing, China", 2},
    {"mirrors.tuna.tsinghua.edu.cn/voidlinux", "https://mirrors.tuna.tsinghua.edu.cn/voidlinux/", "Asia", "Beijing, China", 2},
    {"mirror.mmdbalkhi.ir/void", "https://mirror.mmdbalkhi.ir/void/", "Asia", "Tehran, Iran", 2},
    {"mirrors.kubarcloud.net/voidlinux", "https://mirrors.kubarcloud.net/voidlinux/", "Asia", "Tehran, Iran", 2},
    {"repo.jing.rocks/voidlinux", "https://repo.jing.rocks/voidlinux/", "Asia", "Tokyo, Japan", 2},
    {"mirror.freedif.org/voidlinux", "https://mirror.freedif.org/voidlinux/", "Asia", "Singapore", 2},
    {"mirror.heigou.pe.kr/voidlinux", "https://mirror.heigou.pe.kr/voidlinux/", "Asia", "South Korea", 2},
    {"mirror.meowsmp.net/voidlinux", "https://mirror.meowsmp.net/voidlinux/", "Asia", "Hanoi, Vietnam", 2},

    // Tier 2 - Europe
    {"ftp.dk.xemacs.org/voidlinux", "http://ftp.dk.xemacs.org/voidlinux/", "Europe", "Denmark", 2},
    {"mirrors.dotsrc.org/voidlinux", "https://mirrors.dotsrc.org/voidlinux/", "Europe", "Denmark", 2},
    {"ftp.cc.uoc.gr/mirrors/linux/voidlinux", "https://ftp.cc.uoc.gr/mirrors/linux/voidlinux/", "Europe", "Greece", 2},
    {"voidlinux.mirror.garr.it", "https://voidlinux.mirror.garr.it/", "Europe", "Italy", 2},
    {"void.sakamoto.pl", "https://void.sakamoto.pl/", "Europe", "Warsaw, Poland", 2},
    {"ftp.debian.ru/mirrors/voidlinux", "http://ftp.debian.ru/mirrors/voidlinux/", "Europe", "Russia", 2},
    {"mirror.yandex.ru/mirrors/voidlinux", "https://mirror.yandex.ru/mirrors/voidlinux/", "Europe", "Russia", 2},
    {"ftp.lysator.liu.se/pub/voidlinux", "https://ftp.lysator.liu.se/pub/voidlinux/", "Europe", "Sweden", 2},
    {"mirror.accum.se/mirror/voidlinux", "https://mirror.accum.se/mirror/voidlinux/", "Europe", "Sweden", 2},
    {"mirror.puzzle.ch/voidlinux", "https://mirror.puzzle.ch/voidlinux/", "Europe", "Bern, Switzerland", 2},

    // Tier 2 - North America
    {"mirror.vofr.net/voidlinux", "https://mirror.vofr.net/voidlinux/", "North America", "Virginia, USA", 2},
    {"mirrors.lug.mtu.edu/voidlinux", "https://mirrors.lug.mtu.edu/voidlinux/", "North America", "Michigan, USA", 2},

    // Tier 2 - Oceania
    {"mirror.aarnet.edu.au/pub/voidlinux", "https://mirror.aarnet.edu.au/pub/voidlinux/", "Oceania", "Canberra, Australia", 2},

    // Tier 2 - South and Central America
    {"void.voidbr.org/voidlinux", "https://void.voidbr.org/voidlinux/", "South and Central America", "Mirante do Paranapanema/SP, Brazil", 2},
    {"void.voidlinux.com.br/voidlinux", "https://void.voidlinux.com.br/voidlinux/", "South and Central America", "Cacoal, Brazil", 2},
    {"mirror.linux.ec/voidlinux", "https://mirror.linux.ec/voidlinux/", "South and Central America", "Quito, Ecuador", 2},
};

static int num_mirrors = sizeof(mirrors) / sizeof(mirrors[0]);

GList *get_all_mirrors(void) {
    GList *list = NULL;
    for (int i = 0; i < num_mirrors; i++) {
        list = g_list_append(list, &mirrors[i]);
    }
    return list;
}
