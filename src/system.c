/* Stellarium Web Engine - Copyright (c) 2022 - Stellarium Labs SRL
 *
 * This program is licensed under the terms of the GNU AGPL v3, or
 * alternatively under a commercial licence.
 *
 * The terms of the AGPL v3 license can be found in the main directory of this
 * repository.
 */

#include "swe.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

// Fix mkdir on MINGW
#ifdef WIN32
#   define mkdir(p, m) mkdir(p)
#endif


// The global system instance.
sys_callbacks_t sys_callbacks = {};

char translation[][2][64] = {
  {"Andromeda", "アンドロメダ座"},
  {"Monoceros", "いっかくじゅう座"},
  {"Sagittarius", "いて座"},
  {"Delphinus", "いるか座"},
  {"Indus", "インディアン座"},
  {"Pisces", "うお座"},
  {"Lepus", "うさぎ座"},
  {"Bootes", "うしかい座"},
  {"Hydra", "うみへび座"},
  {"Eridanus", "エリダヌス座"},
  {"Taurus", "おうし座"},
  {"Canis Major", "おおいぬ座"},
  {"Lupus", "おおかみ座"},
  {"Ursa Major", "おおぐま座"},
  {"Virgo", "おとめ座"},
  {"Aries", "おひつじ座"},
  {"Orion", "オリオン座"},
  {"Pictor", "がか座"},
  {"Cassiopeia", "カシオペヤ座"},
  {"Dorado", "かじき座"},
  {"Cancer", "かに座"},
  {"Coma Berenices", "かみのけ座"},
  {"Chamaeleon", "カメレオン座"},
  {"Corvus", "からす座"},
  {"Corona Borealis", "かんむり座"},
  {"Tucana", "きょしちょう座"},
  {"Auriga", "ぎょしゃ座"},
  {"Camelopardalis", "きりん座"},
  {"Pavo", "くじゃく座"},
  {"Cetus", "くじら座"},
  {"Cepheus", "ケフェウス座"},
  {"Centaurus", "ケンタウルス座"},
  {"Microscopium", "けんびきょう座"},
  {"Canis Minor", "こいぬ座"},
  {"Equuleus", "こうま座"},
  {"Vulpecula", "こぎつね座"},
  {"Ursa Minor", "こぐま座"},
  {"Leo Minor", "こじし座"},
  {"Crater", "コップ座"},
  {"Lyra", "こと座"},
  {"Circinus", "コンパス座"},
  {"Ara", "さいだん座"},
  {"Scorpius", "さそり座"},
  {"Triangulum", "さんかく座"},
  {"Leo", "しし座"},
  {"Norma", "じょうぎ座"},
  {"Scutum", "たて座"},
  {"Caelum", "ちょうこくぐ座"},
  {"Sculptor", "ちょうこくしつ座"},
  {"Grus", "つる座"},
  {"Mensa", "テーブルさん座"},
  {"Libra", "てんびん座"},
  {"Lacerta", "とかげ座"},
  {"Horologium", "とけい座"},
  {"Volans", "とびうお座"},
  {"Puppis", "とも座"},
  {"Musca", "はえ座"},
  {"Cygnus", "はくちょう座"},
  {"Octans", "はちぶんぎ座"},
  {"Columba", "はと座"},
  {"Apus", "ふうちょう座"},
  {"Gemini", "ふたご座"},
  {"Pegasus", "ペガスス座"},
  {"Serpens", "へび座"},
  {"Ophiuchus", "へびつかい座"},
  {"Hercules", "ヘルクレス座"},
  {"Perseus", "ペルセウス座"},
  {"Vela", "ほ座"},
  {"Telescopium", "ぼうえんきょう座"},
  {"Phoenix", "ほうおう座"},
  {"Antlia", "ポンプ座"},
  {"Aquarius", "みずがめ座"},
  {"Hydrus", "みずへび座"},
  {"Crux", "みなみじゅうじ座"},
  {"Piscis Austrinus", "みなみのうお座"},
  {"Corona Australis", "みなみのかんむり座"},
  {"Triangulum Australe", "みなみのさんかく座"},
  {"Sagitta", "や座"},
  {"Capricornus", "やぎ座"},
  {"Lynx", "やまねこ座"},
  {"Pyxis", "らしんばん座"},
  {"Draco", "りゅう座"},
  {"Carina", "りゅうこつ座"},
  {"Canes Venatici", "りょうけん座"},
  {"Reticulum", "レチクル座"},
  {"Fornax", "ろ座"},
  {"Sextans", "ろくぶんぎ座"},
  {"Aquila", "わし座"},
  {"Sun", "太陽"},
  {"Moon", "月"},
  {"Mercury", "水星"},
  {"Venus", "金星"},
  {"Earth", "地球"},
  {"Mars", "火星"},
  {"Jupiter", "木星"},
  {"Saturn", "土星"},
  {"Uranus", "天王星"},
  {"Neptune", "海王星"},
  {"Pluto", "冥王星"},
  {"Sirius", "シリウス"},
  {"Canopus", "カノープス"},
  {"Arcturus", "アークトゥルス"},
  {"Rigil", "リギル"},
  {"Kentaurus", "ケンタウルス"},
  {"Vega", "ベガ"},
  {"Capella", "カペラ"},
  {"Rigel", "リゲル"},
  {"Procyon", "プロキオン"},
  {"Betelgeuse", "ベテルギウス"},
  {"Achernar", "アケルナル"},
  {"Hadar", "ハダル"},
  {"Acrux", "アクルックス"},
  {"Altair", "アルタイル"},
  {"Aldebaran", "アルデバラン"},
  {"Capella", "カペラ"},
  {"Antares", "アンタレス"},
  {"Spica", "スピカ"},
  {"Pollux", "ポルックス"},
  {"Fomalhaut", "フォーマルハウト"},
  {"Deneb", "デネブ"},
  {"Mimosa", "ミモザ"},
  {"Toliman", "トリマン"},
  {"Regulus", "レグルス"},
  {"Adara", "アダーラ"},
  {"Castor", "カストル"},
  {"Shaula", "シャウラ"},
  {"Gacrux", "ガクルックス"},
  {"Bellatrix", "ベラトリックス"},
  {"Elnath", "エルナト"},
  {"Miaplacidus", "ミアプラキドゥス"},
  {"Alnilam", "アルニラム"},
  {"Alioth", "アリオト"},
  {"Alnitak", "アルニタク"},
  {"Dubhe", "ドゥーベ"},
  {"Mirfak", "ミルファク"},
  {"Wezen", "ウェズン"},
  {"Kaus", "カウス"},
  {"Australis", "アウストラリス"},
  {"Alkaid", "アルカイド"},
  {"Sargas", "サルガス"},
  {"Menkalinan", "メンカリナン"},
  {"Peacock", "ピーコック"},
  {"Alhena", "アルヘナ"},
  {"Atria", "アトリア"},
  {"Alsephina", "Alsephina"},
  {"Avior", "アヴィオール"},
  {"Murzim", "ミルザム"},
  {"Alphard", "アルファルド"},
  {"Hamal", "ハマル"},
  {"Diphda", "ディフダ"},
  {"Polaris", "ポラリス"},
  {"Menkent", "メンケント"},
  {"Mirach", "ミラク"},
  {"Saiph", "サイフ"},
  {"Alpheratz", "アルフェラッツ"},
  {"Nunki", "ヌンキ"},
  {"Rasalhague", "ラス・アルハゲ"},
  {"Kochab", "コカブ"},
  {"Almach", "アルマク"},
  {"Tiaki", "Tiaki"},
  {"Algol", "アルゴル"},
  {"Denebola", "デネボラ"},
  {"Muhlifain", "ムリファイン"},
  {"Suhail", "スハイル"},
  {"Etamin", "エルタニン"},
  {"Schedar", "シェダル"},
  {"Sadr", "サドル"},
  {"Alphecca", "アルフェッカ"},
  {"Naos", "ナオス"},
  {"Aspidiske", "アスピディスケ"},
  {"Mizar", "ミザール"},
  {"Caph", "カフ"},
  {"Larawag", "Larawag"},
  {"Dschubba", "ジュバ"},
  {"Merak", "メラク"},
  {"Algieba", "アルギエバ"},
  {"Ankaa", "アンカア"},
  {"Girtab", "ギルタブ"},
  {"Izar", "イザール"},
  {"Enif", "エニフ"},
  {"Mintaka", "ミンタカ"},
  {"Scheat", "シェアト"},
  {"Sabik", "サビク"},
  {"Phecda", "フェクダ"},
  {"Aludra", "アルドラ"},
  {"Alderamin", "アルデラミン"},
  {"Adhara", "アダラ"},
  {"Markab", "マルカブ"},
  {"Aljanah", "アルジェナー"},
  {"Markab", "マルカブ"},
  {"Nebula", "星雲"},
  {"Pleiades", "プレアデス星団"}
};


void sys_log(const char *msg)
{
    if (sys_callbacks.log) {
        sys_callbacks.log(sys_callbacks.user, msg);
    } else {
        printf("%s\n", msg);
        fflush(stdout);
    }
}

double sys_get_unix_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000. / 1000.;
}

int sys_get_utc_offset(void)
{
#ifndef WIN32
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);
    return lt.tm_gmtoff;
#else
    // Not implemented yet.
    return 0;
#endif
}

const char *sys_get_user_dir(void)
{
    if (sys_callbacks.get_user_dir) {
        return sys_callbacks.get_user_dir(sys_callbacks.user);
    } else {
        return ".";
    }
}

int sys_make_dir(const char *path)
{
    char tmp[PATH_MAX];
    char *p;
    strcpy(tmp, path);
    for (p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if ((mkdir(tmp, S_IRWXU) != 0) && (errno != EEXIST)) return -1;
        *p = '/';
    }
    return 0;
}

int sys_device_sensors(bool enable_accelero, bool enable_magneto,
                       double acc[3], double mag[3], int *rot,
                       double *calibration_level)
{
    if (!sys_callbacks.device_sensors) return -1;
    return sys_callbacks.device_sensors(
            sys_callbacks.user, enable_accelero, enable_magneto, acc, mag, rot,
            calibration_level);
}

int sys_get_position(double *lat, double *lon, double *alt, double *accuracy)
{
    if (!sys_callbacks.get_position) return -1;
    return sys_callbacks.get_position(
                        sys_callbacks.user, lat, lon, alt, accuracy);
}

const char *sys_translate(const char *domain, const char *str)
{
    assert(domain);
    assert(strcmp(domain, "gui") == 0 ||
           strcmp(domain, "sky") == 0 ||
           strcmp(domain, "skyculture") == 0);
    if (!sys_callbacks.translate) return str;
    return sys_callbacks.translate(sys_callbacks.user, domain, str);
}

const char *translate_jp(const char *str) {
    int i;
    for (i = 0; i<sizeof(translation); i++) {
      if (strcmp(str, translation[i][0]) == 0) {
        return translation[i][1];
      }
    }
    return str;
}

const char *sys_get_lang()
{
    if (!sys_callbacks.get_lang) return "en";
    return sys_callbacks.get_lang();
}

bool sys_lang_supports_spacing()
{
    static int lang_has_spacing = -1;
    if (lang_has_spacing != -1)
        return lang_has_spacing;
    lang_has_spacing = strncmp(sys_get_lang(), "ar", 2) != 0 &&
                       strncmp(sys_get_lang(), "zh", 2) != 0 &&
                       strncmp(sys_get_lang(), "ja", 2) != 0 &&
                       strncmp(sys_get_lang(), "ko", 2) != 0;
    lang_has_spacing = lang_has_spacing ? 1 : 0;
    return lang_has_spacing;
}

char *sys_render_text(const char *txt, float size, int effects, int align,
                      int *w, int *h, int* xoffset, int* yoffset)
{
    assert(sys_callbacks.render_text);
    return sys_callbacks.render_text(sys_callbacks.user, txt, size, effects,
                                     align, w, h, xoffset, yoffset);
}

EMSCRIPTEN_KEEPALIVE
void sys_set_translate_function(
    const char *(*callback)(void *user, const char *domain, const char *str))
{
    sys_callbacks.translate = callback;
}

