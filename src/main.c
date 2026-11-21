/*
 * CITY LIFE 3D — ULTIMATE WORLD V7.2.1
 * Expanded Android open-world city-life game build.
 * C99 + raylib 5.5 + KarvinJ/raylib-mobile Android template.
 * This project is not affiliated with Rockstar Games or GTA.
 *
 * V7 upgrade: persistent progression, invulnerable player mode, weapon presentation, combat audio and expanded feedback.
 * V7 gameplay: 50x50 procedural city grid, zombie and gang outbreak waves,
 * contextual interiors/shops, wallet and purchases, survival stats, and five user tracks only.
 * World geometry remains procedural low-poly; realistic external GLB/PBR asset packs are not included.
 */

#include "raymob.h"
#include "rlgl.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PI_F                 3.14159265358979323846f
#define RAD_TO_DEG           57.29577951308232f
#define CITY_HALF            1200.0f
#define WORLD_HALF           1900.0f
#define CITY_BLOCK           48.0f
#define CITY_ROAD_COUNT      51
#define MAX_BUILDINGS        10600
#define CITY_GRID_SIDE       50
#define BUILDINGS_PER_BLOCK  4
#define WALKER_COUNT         128
#define TRAFFIC_COUNT        72
#define TREE_CAPACITY        1300
#define MUSIC_CAPACITY       32
#define MISSION_COUNT        12
#define DRAW_DISTANCE        220.0f
#define SAND_X_MIN           1280.0f
#define SAND_X_MAX           1780.0f
#define SAND_Z_MIN            360.0f
#define SAND_Z_MAX            900.0f
#define SAND_CENTER_X        1530.0f
#define SAND_CENTER_Z         630.0f
#define HOSTILE_CAPACITY      48
#define INITIAL_ZOMBIES      12
#define INITIAL_GANG_MEMBERS  6
#define SMOKE_COUNT          64
#define SKID_COUNT           220
#define PLAYER_RADIUS        0.62f
#define CAR_RADIUS           1.55f
#define TARGET_COUNT         8
#define SFX_COUNT            8
#define SAVE_MAGIC           0x434C5637
#define SAVE_SCHEMA_VERSION  2
#define SAVE_DATA_SLOT_COUNT 33
#define SAVE_FILE_NAME       "citylife_profile.dat"

/* ---- Palette ----------------------------------------------------------- */
static const Color C_SKY          = { 126, 196, 235, 255 };
static const Color C_GRASS        = { 91, 174, 91, 255 };
static const Color C_ROAD         = { 73, 78, 86, 255 };
static const Color C_SIDEWALK     = { 184, 185, 180, 255 };
static const Color C_CURB         = { 224, 222, 210, 255 };
static const Color C_LANE         = { 238, 239, 232, 255 };
static const Color C_WINDOW       = { 112, 190, 226, 255 };
static const Color C_WINDOW_LIT   = { 246, 205, 131, 255 };
static const Color C_GLASS_DARK   = { 54, 111, 150, 255 };
static const Color C_HUD          = { 12, 20, 31, 232 };
static const Color C_GOLD         = { 255, 194, 66, 255 };
static const Color C_MINT         = { 91, 220, 174, 255 };
static const Color C_WHITE        = { 245, 247, 249, 255 };
static const Color C_RED          = { 225, 84, 74, 255 };
static const Color C_BLUE         = { 64, 143, 222, 255 };
static const Color C_DARK         = { 22, 28, 39, 255 };

/* ---- Data -------------------------------------------------------------- */
typedef struct {
    Vector3 position;
    float width, depth, height;
    int style;
    Color wall;
    Color trim;
    Color sign;
    unsigned int seed;
} Building;

typedef struct {
    Vector3 position;
    float yaw;
    float speed;
    float steering;
    Color paint;
    int model;
} Vehicle;

typedef struct {
    Vector3 position;
    float yaw;
    float phase;
    float speed;
    float routeMin;
    float routeMax;
    bool alongX;
    bool isStatic;
    int direction;
    Color shirt;
    Color trousers;
    Color skin;
} Pedestrian;

typedef struct {
    float x, z, scale;
    int species;
} WorldTree;

typedef struct {
    float x;
    float y;
} MoveInput;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float life;
    float maxLife;
    float size;
} SmokeParticle;

typedef struct {
    Vector3 a;
    Vector3 b;
    float life;
} SkidMark;

typedef enum {
    GAME_TITLE = 0,
    GAME_PLAYING,
    GAME_PAUSED
} GameState;

typedef enum {
    HOSTILE_ZOMBIE = 0,
    HOSTILE_GANG
} HostileType;

typedef struct {
    Vector3 position;
    float yaw;
    float phase;
    float health;
    float attackCooldown;
    float moveSpeed;
    int faction;
    bool active;
    HostileType type;
} Hostile;

typedef enum {
    INTERIOR_NONE = 0,
    INTERIOR_WEAPON,
    INTERIOR_MARKET,
    INTERIOR_DINER,
    INTERIOR_HOME
} InteriorType;

/* ---- Global city data -------------------------------------------------- */
static Building gBuildings[MAX_BUILDINGS];
static int gBuildingCount = 0;
static unsigned short gCellBuildingIndices[CITY_GRID_SIDE][CITY_GRID_SIDE][BUILDINGS_PER_BLOCK];
static unsigned char gCellBuildingCounts[CITY_GRID_SIDE][CITY_GRID_SIDE];
static Pedestrian gPedestrians[WALKER_COUNT];
static Vehicle gTraffic[TRAFFIC_COUNT];
static WorldTree gTrees[TREE_CAPACITY];
static int gTreeCount = 0;
static Music gMusicTracks[MUSIC_CAPACITY];
static char gMusicTitles[MUSIC_CAPACITY][64];
static int gMusicCount = 0;
static SmokeParticle gSmoke[SMOKE_COUNT];
static SkidMark gSkids[SKID_COUNT];
static int gSmokeCursor = 0;
static int gSkidCursor = 0;
static float gSmokeSpawnTimer = 0.0f;
static float gNightAmount = 0.0f;
static float gDriftScore = 0.0f;
static int gBestDrift = 0;
static bool gRadioEnabled = true;
static bool gAudioReady = false;
static bool gAudioDeviceReady = false;
static int gRadioIndex = 0;
static Sound gEngineSound = {0};
static Sound gGameSfx[SFX_COUNT] = {0};
static Texture2D gArabicAtlas = {0};
static float gMuzzleFlashTimer = 0.0f;
static float gFootstepTimer = 0.0f;
typedef struct { Vector3 start, end; float life; } ShotEffect;
static ShotEffect gShotEffect = {0};
static bool gPlayerInvulnerable = true;
static int gWeaponIndex = 0;
static int gWeaponAmmo[3] = {45, 0, 0};
static int gWeaponReserve[3] = {135, 0, 0};
static float gWeaponCooldown = 0.0f;
static float gMeleeCooldown = 0.0f;

typedef struct {
    Vector3 position;
    bool active;
    float respawnTimer;
} TrainingTarget;
static TrainingTarget gTargets[TARGET_COUNT];

/* Zombie outbreak, rival street crews and life-simulation state. */
static Hostile gHostiles[HOSTILE_CAPACITY];
static int gHostileActive = 0;
static int gWaveNumber = 1;
static int gChapter = 1;
static int gZombieKills = 0;
static int gGangKills = 0;
static int gPlayerHealth = 100;
static float gPlayerHunger = 100.0f;
static float gHungerDamageTimer = 0.0f;
static int gPlayerArmor = 0;
static bool gIndoors = false;
static InteriorType gInteriorType = INTERIOR_NONE;
static Vector3 gExteriorPosition = {0};
static float gClockHourOffset = 8.0f;
static bool gWeaponOwned[3] = {true, false, false};
static bool gInteriorExitWasDown = false;
static bool gInteriorBuy1WasDown = false;
static bool gInteriorBuy2WasDown = false;
static bool gInteriorBuy3WasDown = false;

/* Arabic UI labels are pre-shaped into an atlas to guarantee correct RTL text
   on raylib's small Android default font. The generated PNG contains no font. */
enum {
    AR_CITY=0, AR_CASH, AR_MISSIONS, AR_SPEED, AR_DRIFT, AR_BEST, AR_RADIO, AR_GARAGE,
    AR_PICKUP, AR_DELIVER, AR_ALL_DONE, AR_ENTER_CAR, AR_EXIT, AR_INTERACT, AR_JUMP, AR_GAS,
    AR_BRAKE, AR_NITRO, AR_HANDBRAKE, AR_FIRE, AR_SWITCH_WEAPON, AR_AMMO, AR_POINTS, AR_LEVEL,
    AR_PULSE, AR_RIFLE, AR_SHOCK, AR_RANGE, AR_DOWNTOWN, AR_OLDTOWN, AR_AIRPORT, AR_HARBOR,
    AR_DESERT, AR_COAST, AR_INDUSTRIAL, AR_WEATHER, AR_RAIN, AR_NIGHT, AR_CLEAR, AR_START,
    AR_RESUME, AR_RESTART, AR_PAUSED, AR_MISSION_OK, AR_PICKED, AR_DELIVERED, AR_HIT, AR_NO_AMMO,
    AR_MISS, AR_MOVE, AR_CAMERA, AR_RADIO_NAME, AR_SAVE, AR_SCORE, AR_EASY, AR_JOB,
    AR_SAND, AR_COAST_ROAD, AR_DRIFT_POINTS, AR_MUSIC, AR_SWITCHED, AR_READY, AR_CLOSE,
    AR_FOOT, AR_VEHICLE, AR_KMH,
    AR_ZOMBIES, AR_GANGS, AR_HEALTH, AR_HUNGER, AR_TIME, AR_CHAPTER, AR_WAVE,
    AR_KILLS, AR_GUNSHOP, AR_MARKET, AR_DINER, AR_HOME, AR_BUY_PISTOL,
    AR_BUY_RIFLE, AR_BUY_SHOTGUN, AR_BUY_AMMO, AR_BUY_FOOD, AR_EAT_MEAL,
    AR_SLEEP, AR_EXIT_STORE, AR_OUTBREAK, AR_NEW_WAVE, AR_DEFEATED,
    AR_NOT_ENOUGH, AR_OWNED, AR_DAY, AR_HOUR, AR_ARMOR, AR_PUNCH, AR_RELOAD,
    AR_DEV_CREDIT, AR_CREATOR_NAME, AR_COPYRIGHT_LABEL, AR_ULTIMATE_WORLD
};
#define AR_LABEL_COUNT 100


static int GetRadioTrackCount(void);
static bool IsInSandArena(Vector3 p);

typedef enum {
    SFX_PISTOL = 0, SFX_RIFLE, SFX_SHOTGUN, SFX_RELOAD,
    SFX_JUMP, SFX_FOOTSTEP, SFX_MELEE, SFX_IMPACT
} GameSfx;

/* ---- Small helpers ----------------------------------------------------- */
static float ClampF(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float LerpF(float a, float b, float t) {
    return a + (b - a) * t;
}

static float Distance2D(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2;
    float dz = z1 - z2;
    return sqrtf(dx*dx + dz*dz);
}

static float NormalizeAngle(float a) {
    while (a > PI_F) a -= 2.0f*PI_F;
    while (a < -PI_F) a += 2.0f*PI_F;
    return a;
}

static Color ColorScale(Color c, float factor) {
    Color out = c;
    out.r = (unsigned char)ClampF((float)c.r*factor, 0.0f, 255.0f);
    out.g = (unsigned char)ClampF((float)c.g*factor, 0.0f, 255.0f);
    out.b = (unsigned char)ClampF((float)c.b*factor, 0.0f, 255.0f);
    return out;
}

static Color LerpColor(Color a, Color b, float t) {
    t = ClampF(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)LerpF((float)a.r, (float)b.r, t),
        (unsigned char)LerpF((float)a.g, (float)b.g, t),
        (unsigned char)LerpF((float)a.b, (float)b.b, t), 255
    };
}

static unsigned int Hash2(int a, int b, int c) {
    unsigned int h = (unsigned int)(a*73856093u) ^
                     (unsigned int)(b*19349663u) ^
                     (unsigned int)(c*83492791u);
    h ^= h >> 13;
    h *= 1274126177u;
    h ^= h >> 16;
    return h;
}

static bool PointInRect(Vector2 p, Rectangle r) {
    return p.x >= r.x && p.x <= r.x + r.width &&
           p.y >= r.y && p.y <= r.y + r.height;
}

static bool TouchHeld(Rectangle r) {
    int count = GetTouchPointCount();
    for (int i = 0; i < count; i++) {
        if (PointInRect(GetTouchPosition(i), r)) return true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), r)) return true;
    return false;
}

static bool TouchPressed(Rectangle r, bool *wasDown) {
    bool down = TouchHeld(r);
    bool pressed = down && !(*wasDown);
    *wasDown = down;
    return pressed;
}

static void DrawPanel(Rectangle r, Color fill, Color outline) {
    DrawRectangleRounded(r, 0.12f, 8, fill);
    DrawRectangleRoundedLines(r, 0.12f, 8, outline);
}

static void DrawArabicLabel(int label, Rectangle dest, Color tint) {
    if (gArabicAtlas.id == 0 || label < 0 || label >= AR_LABEL_COUNT) return;
    Rectangle source = {(float)(label%4)*256.0f, (float)(label/4)*64.0f, 256.0f, 64.0f};
    DrawTexturePro(gArabicAtlas, source, dest, (Vector2){0.0f, 0.0f}, 0.0f, tint);
}

static void DrawArabicButton(Rectangle r, int label, Color fill, bool active) {
    Color face = active ? ColorScale(fill, 1.23f) : fill;
    DrawRectangleRounded(r, 0.20f, 9, Fade(face, 0.93f));
    DrawRectangleRoundedLines(r, 0.20f, 9, Fade(C_WHITE, 0.82f));
    DrawArabicLabel(label, (Rectangle){r.x + 5.0f, r.y + 5.0f, r.width - 10.0f, r.height - 10.0f}, C_WHITE);
}

static int GetArabicDistrictLabel(Vector3 p) {
    if (IsInSandArena(p)) return AR_DESERT;
    if (p.x < -960.0f && p.z > -180.0f && p.z < 180.0f) return AR_AIRPORT;
    if (p.x > 900.0f && p.z < -900.0f) return AR_HARBOR;
    if (p.z < -820.0f) return AR_COAST;
    if (fabsf(p.x) < 120.0f && fabsf(p.z) < 120.0f) return AR_DOWNTOWN;
    if (p.x > 720.0f) return AR_INDUSTRIAL;
    if (p.z > 760.0f) return AR_OLDTOWN;
    if (p.x < -720.0f) return AR_OLDTOWN;
    return AR_CITY;
}

static int GetArabicMessageLabel(const char *message) {
    if (strstr(message, "NEW OUTBREAK WAVE") != NULL) return AR_NEW_WAVE;
    if (strstr(message, "ZOMBIE") != NULL || strstr(message, "OUTBREAK") != NULL) return AR_ZOMBIES;
    if (strstr(message, "GANG") != NULL) return AR_GANGS;
    if (strstr(message, "PUNCH") != NULL || strstr(message, "MELEE") != NULL) return AR_PUNCH;
    if (strstr(message, "NOT ENOUGH") != NULL) return AR_NOT_ENOUGH;
    if (strstr(message, "PURCHASE") != NULL) return AR_OWNED;
    if (strstr(message, "RESTED") != NULL) return AR_SLEEP;
    if (strstr(message, "TARGET HIT") != NULL) return AR_HIT;
    if (strstr(message, "NO AMMO") != NULL) return AR_NO_AMMO;
    if (strstr(message, "MAGAZINE FULL") != NULL) return AR_AMMO;
    if (strstr(message, "PRACTICE RANGE") != NULL) return AR_MISS;
    if (strstr(message, "DELIVERY COMPLETE") != NULL) return AR_DELIVERED;
    if (strstr(message, "PACKAGE COLLECTED") != NULL) return AR_PICKED;
    if (strstr(message, "ALL JOBS") != NULL) return AR_ALL_DONE;
    if (strstr(message, "VEHICLE READY") != NULL) return AR_READY;
    if (strstr(message, "RADIO:") != NULL) return AR_RADIO_NAME;
    if (strstr(message, "RELOADED") != NULL) return AR_RELOAD;
    if (strstr(message, "WEAPON") != NULL) return AR_SWITCH_WEAPON;
    if (strstr(message, "YOU ARE ON FOOT") != NULL) return AR_MOVE;
    if (strstr(message, "CAMERA MODE") != NULL) return AR_CAMERA;
    if (strstr(message, "COLLISION") != NULL) return AR_MISS;
    return -1;
}

static void DrawButton(Rectangle r, const char *label, Color fill, bool active) {
    Color face = active ? ColorScale(fill, 1.23f) : fill;
    DrawRectangleRounded(r, 0.20f, 9, Fade(face, 0.91f));
    DrawRectangleRoundedLines(r, 0.20f, 9, Fade(C_WHITE, 0.82f));
    int size = (r.width < 82.0f) ? 15 : 17;
    int tw = MeasureText(label, size);
    DrawText(label, (int)(r.x + (r.width - (float)tw)*0.5f),
             (int)(r.y + (r.height - (float)size)*0.5f), size, C_WHITE);
}

/* ---- City generation --------------------------------------------------- */
static void AddBuilding(float x, float z, float w, float d, float h,
                        int style, unsigned int seed) {
    if (gBuildingCount >= MAX_BUILDINGS) return;

    static const Color walls[] = {
        { 175, 213, 232, 255 }, { 232, 222, 203, 255 },
        { 225, 177, 158, 255 }, { 176, 211, 181, 255 },
        { 224, 184, 216, 255 }, { 235, 220, 170, 255 },
        { 169, 195, 218, 255 }, { 222, 211, 204, 255 },
        { 190, 224, 226, 255 }, { 242, 225, 211, 255 }
    };
    static const Color trims[] = {
        { 242, 243, 235, 255 }, { 220, 229, 232, 255 },
        { 184, 197, 208, 255 }, { 236, 226, 211, 255 }
    };
    static const Color signs[] = {
        { 42, 126, 190, 255 }, { 211, 83, 75, 255 },
        { 52, 151, 105, 255 }, { 151, 91, 176, 255 }
    };

    int buildingIndex = gBuildingCount++;
    Building *b = &gBuildings[buildingIndex];
    int cellX = (int)floorf((x + CITY_HALF)/CITY_BLOCK);
    int cellZ = (int)floorf((z + CITY_HALF)/CITY_BLOCK);
    if (cellX >= 0 && cellX < CITY_GRID_SIDE &&
        cellZ >= 0 && cellZ < CITY_GRID_SIDE) {
        unsigned char *count = &gCellBuildingCounts[cellX][cellZ];
        if (*count < BUILDINGS_PER_BLOCK)
            gCellBuildingIndices[cellX][cellZ][(*count)++] = (unsigned short)buildingIndex;
    }
    b->position = (Vector3){ x, 0.0f, z };
    b->width = w;
    b->depth = d;
    b->height = h;
    b->style = style;
    b->seed = seed;
    b->wall = walls[seed % (sizeof(walls)/sizeof(walls[0]))];
    b->trim = trims[(seed >> 4) % (sizeof(trims)/sizeof(trims[0]))];
    b->sign = signs[(seed >> 7) % (sizeof(signs)/sizeof(signs[0]))];
}

static void BuildCity(void) {
    gBuildingCount = 0;
    memset(gCellBuildingCounts, 0, sizeof(gCellBuildingCounts));

    /* Fifty by fifty blocks. Airport and harbour districts are reserved.
       The large procedural grid is distance-culled during rendering. */
    for (int bx = -25; bx < 25; bx++) {
        for (int bz = -25; bz < 25; bz++) {
            if (abs(bx) <= 1 && abs(bz) <= 1) continue; /* central civic plaza, shops and home */
            if (bx <= -22 && bz >= -3 && bz <= 3) continue;
            if (bx >= 21 && bz <= -21) continue; /* container harbour district */

            float cx = (float)bx*CITY_BLOCK + CITY_BLOCK*0.5f;
            float cz = (float)bz*CITY_BLOCK + CITY_BLOCK*0.5f;

            for (int sx = -1; sx <= 1; sx += 2) {
                for (int sz = -1; sz <= 1; sz += 2) {
                    unsigned int seed = Hash2(bx, bz, (sx+2)*5 + (sz+2));
                    int style = (int)(seed % 4u);
                    float h = 15.0f + (float)((seed >> 5) % 8u)*3.0f;
                    /* Downtown towers; residential blocks and shops use a stepped skyline. */
                    if (abs(bx) <= 2 && abs(bz) <= 2) h += 30.0f;
                    else if (bx >= 4) h = 9.0f + (float)((seed >> 8) % 5u)*1.9f;
                    else if (bz >= 4) h = 11.0f + (float)((seed >> 9) % 6u)*2.2f;
                    else if (bx <= -8) h = 13.0f + (float)((seed >> 10) % 5u)*2.4f;
                    if (style == 2) h += 15.0f;
                    if (style == 3) h = 10.0f + (float)((seed >> 9) % 4u)*2.0f;
                    if (h > 92.0f) h = 92.0f;
                    float width = 12.0f + (float)((seed >> 12) % 4u)*0.65f;
                    float depth = 12.0f + (float)((seed >> 15) % 4u)*0.65f;
                    AddBuilding(cx + (float)sx*9.0f,
                                cz + (float)sz*9.0f,
                                width, depth, h, style, seed);
                }
            }
        }
    }
}

static bool HitsBuilding(float x, float z, float radius) {
    /* Building footprints are at most 6.9 units from their centre. */
    const float margin = 10.0f + radius;
    int minCellX = (int)floorf((x - margin + CITY_HALF)/CITY_BLOCK);
    int maxCellX = (int)floorf((x + margin + CITY_HALF)/CITY_BLOCK);
    int minCellZ = (int)floorf((z - margin + CITY_HALF)/CITY_BLOCK);
    int maxCellZ = (int)floorf((z + margin + CITY_HALF)/CITY_BLOCK);
    if (maxCellX < 0 || maxCellZ < 0 ||
        minCellX >= CITY_GRID_SIDE || minCellZ >= CITY_GRID_SIDE) return false;
    if (minCellX < 0) minCellX = 0;
    if (minCellZ < 0) minCellZ = 0;
    if (maxCellX >= CITY_GRID_SIDE) maxCellX = CITY_GRID_SIDE - 1;
    if (maxCellZ >= CITY_GRID_SIDE) maxCellZ = CITY_GRID_SIDE - 1;

    for (int cx = minCellX; cx <= maxCellX; cx++) {
        for (int cz = minCellZ; cz <= maxCellZ; cz++) {
            unsigned char count = gCellBuildingCounts[cx][cz];
            for (int slot = 0; slot < count; slot++) {
                const Building *b = &gBuildings[gCellBuildingIndices[cx][cz][slot]];
                float minX = b->position.x - b->width*0.5f;
                float maxX = b->position.x + b->width*0.5f;
                float minZ = b->position.z - b->depth*0.5f;
                float maxZ = b->position.z + b->depth*0.5f;
                float nearX = ClampF(x, minX, maxX);
                float nearZ = ClampF(z, minZ, maxZ);
                if (Distance2D(x, z, nearX, nearZ) < radius) return true;
            }
        }
    }
    return false;
}

static bool IsInSandArena(Vector3 p) {
    return p.x >= SAND_X_MIN && p.x <= SAND_X_MAX &&
           p.z >= SAND_Z_MIN && p.z <= SAND_Z_MAX;
}

static void MoveWithCollision(Vector3 *p, float dx, float dz, float radius) {
    float nx = ClampF(p->x + dx, -WORLD_HALF + 4.0f, WORLD_HALF - 4.0f);
    if (!HitsBuilding(nx, p->z, radius)) p->x = nx;

    float nz = ClampF(p->z + dz, -WORLD_HALF + 4.0f, WORLD_HALF - 4.0f);
    if (!HitsBuilding(p->x, nz, radius)) p->z = nz;
}

/* Prevent hits through buildings: the shot line is sampled in the horizontal
   plane because all city buildings rise from ground level. */
static bool HasClearShot(Vector3 from, Vector3 to) {
    float dx = to.x - from.x;
    float dz = to.z - from.z;
    float distance = sqrtf(dx*dx + dz*dz);
    if (distance <= 2.0f) return true;
    int steps = (int)ceilf(distance/1.25f);
    if (steps < 2) steps = 2;
    for (int i = 2; i < steps; i++) {
        float t = (float)i/(float)steps;
        float x = from.x + dx*t;
        float z = from.z + dz*t;
        if (HitsBuilding(x, z, 0.18f)) return false;
    }
    return true;
}

/* Keep the follow camera outside building footprints in narrow streets. */
static Vector3 ResolveCameraCollision(Vector3 target, Vector3 desired) {
    float dx = desired.x - target.x;
    float dz = desired.z - target.z;
    float distance = sqrtf(dx*dx + dz*dz);
    int steps = (int)ceilf(distance/0.75f);
    if (steps < 1) return desired;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i/(float)steps;
        float x = target.x + dx*t;
        float z = target.z + dz*t;
        if (HitsBuilding(x, z, 0.42f)) {
            float safeT = (float)(i - 1)/(float)steps;
            desired.x = target.x + dx*safeT;
            desired.z = target.z + dz*safeT;
            desired.y = LerpF(target.y + 1.0f, desired.y, safeT);
            break;
        }
    }
    return desired;
}

/* ---- World rendering --------------------------------------------------- */
static void DrawRoadNetwork(Vector3 focus) {
    /* City ground. */
    /* A wider ground plane connects the old city to the desert drift peninsula. */
    DrawPlane((Vector3){0.0f, -0.12f, 0.0f},
              (Vector2){2460.0f, 2460.0f}, C_GRASS);

    /* Ocean, beaches and a long coastal promenade north of the city. */
    DrawCube((Vector3){0.0f, -0.08f, -1218.0f},
             2380.0f, 0.12f, 122.0f, (Color){ 41, 126, 169, 255 });
    DrawCube((Vector3){0.0f, -0.01f, -1158.0f},
             2380.0f, 0.14f, 8.0f, (Color){ 224, 198, 153, 255 });
    DrawCube((Vector3){0.0f, 0.025f, -1147.0f},
             2380.0f, 0.11f, 3.0f, C_SIDEWALK);

    for (int i = 0; i < CITY_ROAD_COUNT; i++) {
        float c = -CITY_HALF + (float)i*CITY_BLOCK;
        float width = (i == 4) ? 17.0f : 14.0f;
        float sidewalk = 3.0f;

        /* Road surfaces. */
        DrawCube((Vector3){c, 0.0f, 0.0f}, width, 0.12f, 2460.0f, C_ROAD);
        DrawCube((Vector3){0.0f, 0.0f, c}, 2460.0f, 0.12f, width, C_ROAD);

        /* Road shoulders and sidewalks on both sides. */
        for (int side = -1; side <= 1; side += 2) {
            float offset = (width*0.5f) + sidewalk*0.5f;
            DrawCube((Vector3){c + (float)side*offset, 0.015f, 0.0f},
                     sidewalk, 0.15f, 2460.0f, C_SIDEWALK);
            DrawCube((Vector3){0.0f, 0.015f, c + (float)side*offset},
                     2460.0f, 0.15f, sidewalk, C_SIDEWALK);
            DrawCube((Vector3){c + (float)side*(width*0.5f + sidewalk), 0.10f, 0.0f},
                     0.22f, 0.18f, 2460.0f, C_CURB);
            DrawCube((Vector3){0.0f, 0.10f, c + (float)side*(width*0.5f + sidewalk)},
                     2460.0f, 0.18f, 0.22f, C_CURB);
        }

        /* White dashed lane separators, matching the bright stylized city reference. */
        for (int m = -1200; m <= 1200; m += 32) {
            if (fabsf(c - focus.x) < DRAW_DISTANCE &&
                fabsf((float)m - focus.z) < DRAW_DISTANCE) {
                DrawCube((Vector3){c, 0.078f, (float)m}, 0.22f, 0.035f, 5.8f, C_LANE);
            }
            if (fabsf(c - focus.z) < DRAW_DISTANCE &&
                fabsf((float)m - focus.x) < DRAW_DISTANCE) {
                DrawCube((Vector3){(float)m, 0.078f, c}, 5.8f, 0.035f, 0.22f, C_LANE);
            }
        }
    }

    /* Painted zebra crossings at nearby grid intersections. */
    int nearRoadX = (int)floorf(focus.x/CITY_BLOCK + 0.5f);
    int nearRoadZ = (int)floorf(focus.z/CITY_BLOCK + 0.5f);
    for (int ix = nearRoadX - 1; ix <= nearRoadX + 1; ix++) {
        for (int iz = nearRoadZ - 1; iz <= nearRoadZ + 1; iz++) {
            float rx = (float)ix*CITY_BLOCK;
            float rz = (float)iz*CITY_BLOCK;
            if (Distance2D(focus.x, focus.z, rx, rz) > DRAW_DISTANCE) continue;
            for (int bar = -3; bar <= 3; bar++) {
                float offset = (float)bar*0.95f;
                DrawCube((Vector3){rx + offset, 0.092f, rz - 5.25f},
                         0.52f, 0.035f, 3.2f, C_WHITE);
                DrawCube((Vector3){rx + offset, 0.092f, rz + 5.25f},
                         0.52f, 0.035f, 3.2f, C_WHITE);
                DrawCube((Vector3){rx - 5.25f, 0.092f, rz + offset},
                         3.2f, 0.035f, 0.52f, C_WHITE);
                DrawCube((Vector3){rx + 5.25f, 0.092f, rz + offset},
                         3.2f, 0.035f, 0.52f, C_WHITE);
            }
        }
    }

    /* Two-lane access road from the city ring road to the desert drift zone. */
    if (focus.x > 1120.0f || (focus.x > 1000.0f && focus.z > 300.0f)) {
        DrawCube((Vector3){1370.0f, 0.0f, 630.0f}, 390.0f, 0.13f, 17.0f, C_ROAD);
        DrawCube((Vector3){1370.0f, 0.09f, 630.0f}, 390.0f, 0.025f, 0.22f, C_LANE);
        DrawCube((Vector3){1370.0f, 0.10f, 621.0f}, 390.0f, 0.16f, 0.35f, C_CURB);
        DrawCube((Vector3){1370.0f, 0.10f, 639.0f}, 390.0f, 0.16f, 0.35f, C_CURB);
    }

    /* Crosswalk bars at the central civic plaza. */
    for (int i = -4; i <= 4; i++) {
        float p = (float)i*1.55f;
        DrawCube((Vector3){p, 0.09f, 8.8f}, 0.72f, 0.025f, 3.4f, C_WHITE);
        DrawCube((Vector3){8.8f, 0.09f, p}, 3.4f, 0.025f, 0.72f, C_WHITE);
        DrawCube((Vector3){p, 0.09f, -8.8f}, 0.72f, 0.025f, 3.4f, C_WHITE);
        DrawCube((Vector3){-8.8f, 0.09f, p}, 3.4f, 0.025f, 0.72f, C_WHITE);
    }
}

static void DrawBuildingWindows(const Building *b, float distance, Vector3 focus) {
    /* Repeated framed windows create the clean toy-city facade seen in the reference. */
    if (distance > 158.0f) return;

    int floors = (int)(b->height/3.15f);
    if (floors < 2) floors = 2;
    int floorLimit = (distance > 118.0f) ? 3 : ((distance > 76.0f) ? 5 : 8);
    if (floors > floorLimit) floors = floorLimit;

    float windowW = (b->style == 2) ? 1.85f : 1.72f;
    float windowH = (b->style == 2) ? 1.75f : 1.58f;
    float sideOffset = b->width*0.5f + 0.07f;
    float frontOffset = b->depth*0.5f + 0.07f;
    float frontSign = (focus.z >= b->position.z) ? 1.0f : -1.0f;
    float sideSign = (focus.x >= b->position.x) ? 1.0f : -1.0f;
    int columns = (distance > 112.0f) ? 2 : 3;
    float spacing = (b->width > 13.0f) ? 3.65f : 3.25f;

    for (int f = 0; f < floors; f++) {
        float y = 2.05f + (float)f*3.12f;
        if (y + 1.05f > b->height - 0.60f) break;

        for (int col = 0; col < columns; col++) {
            float x = b->position.x + ((float)col - (float)(columns-1)*0.5f)*spacing;
            unsigned int lit = Hash2((int)b->position.x + col, (int)b->position.z + f, b->style);
            Color glass = (gNightAmount > 0.34f || (lit % 9u) == 0u) ?
                          C_WINDOW_LIT : C_WINDOW;
            /* Main street-facing elevation: dark frame and inset blue glass. */
            float z = b->position.z + frontSign*frontOffset;
            DrawCube((Vector3){x, y, z}, windowW + 0.20f, windowH + 0.20f,
                     0.11f, ColorScale(b->trim, 0.76f));
            DrawCube((Vector3){x, y, z + frontSign*0.065f}, windowW, windowH, 0.055f, glass);
            DrawCube((Vector3){x, y, z + frontSign*0.10f}, 0.075f, windowH, 0.035f, C_WHITE);
            DrawCube((Vector3){x, y, z + frontSign*0.10f}, windowW, 0.075f, 0.035f, C_WHITE);
        }

        /* Side elevations are detailed on close buildings and modern towers. */
        if (distance < 118.0f) {
            for (int col = -1; col <= 1; col++) {
                float z = b->position.z + (float)col*2.55f;
                float x = b->position.x + sideSign*sideOffset;
                DrawCube((Vector3){x, y, z}, 0.11f, windowH + 0.18f,
                         windowW + 0.18f, ColorScale(b->trim, 0.76f));
                DrawCube((Vector3){x + sideSign*0.065f, y, z},
                         0.055f, windowH, windowW, C_WINDOW);
            }
        }

        /* Thin floor slab/band makes each story readable from a distance. */
        if (b->height > 18.0f && f > 0) {
            float bandY = 0.58f + (float)f*3.12f;
            DrawCube((Vector3){b->position.x, bandY,
                               b->position.z + frontSign*(b->depth*0.5f + 0.035f)},
                     b->width*0.94f, 0.095f, 0.075f, b->trim);
        }
    }
}

static void DrawBuilding(const Building *b, Vector3 focus) {
    float dx = focus.x - b->position.x;
    float dz = focus.z - b->position.z;
    float distanceSquared = dx*dx + dz*dz;
    if (distanceSquared > DRAW_DISTANCE*DRAW_DISTANCE) return;
    float distance = sqrtf(distanceSquared);

    Vector3 center = {b->position.x, b->height*0.5f, b->position.z};
    Color shadowTrim = ColorScale(b->trim, 0.78f);
    float frontSign = (focus.z >= b->position.z) ? 1.0f : -1.0f;
    DrawCube(center, b->width, b->height, b->depth, b->wall);

    /* Crisp light roof rim and a raised parapet around the top. */
    DrawCube((Vector3){b->position.x, b->height + 0.20f, b->position.z},
             b->width + 0.55f, 0.40f, b->depth + 0.55f, b->trim);
    DrawCube((Vector3){b->position.x, b->height + 0.53f,
                       b->position.z - b->depth*0.5f + 0.10f},
             b->width + 0.28f, 0.42f, 0.18f, shadowTrim);
    DrawCube((Vector3){b->position.x, b->height + 0.53f,
                       b->position.z + b->depth*0.5f - 0.10f},
             b->width + 0.28f, 0.42f, 0.18f, shadowTrim);
    DrawCube((Vector3){b->position.x - b->width*0.5f + 0.10f,
                       b->height + 0.53f, b->position.z},
             0.18f, 0.42f, b->depth, shadowTrim);
    DrawCube((Vector3){b->position.x + b->width*0.5f - 0.10f,
                       b->height + 0.53f, b->position.z},
             0.18f, 0.42f, b->depth, shadowTrim);

    /* Corner pilasters and street-level plinth add architectural depth. */
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawCube((Vector3){b->position.x + (float)sx*(b->width*0.5f - 0.23f),
                           b->height*0.5f, b->position.z + b->depth*0.5f + 0.035f},
                 0.30f, b->height, 0.10f, ColorScale(b->trim, 0.92f));
    }
    DrawCube((Vector3){b->position.x, 0.32f, b->position.z},
             b->width + 0.22f, 0.42f, b->depth + 0.22f, shadowTrim);

    DrawBuildingWindows(b, distance, focus);

    /* Front entrance and glass shopfront. */
    DrawCube((Vector3){b->position.x, 1.35f,
                       b->position.z + frontSign*(b->depth*0.5f + 0.10f)},
             b->width*0.70f, 1.42f, 0.13f, C_GLASS_DARK);
    DrawCube((Vector3){b->position.x, 1.35f,
                       b->position.z + frontSign*(b->depth*0.5f + 0.18f)},
             b->width*0.63f, 1.23f, 0.045f, C_WINDOW);
    DrawCube((Vector3){b->position.x, 0.43f,
                       b->position.z + frontSign*(b->depth*0.5f + 0.24f)},
             b->width*0.78f, 0.13f, 0.42f, b->trim);

    if (b->style == 3) {
        /* Bright shop sign and striped canopy. */
        DrawCube((Vector3){b->position.x, 2.72f,
                           b->position.z + frontSign*(b->depth*0.5f + 0.48f)},
                 b->width*0.91f, 0.55f, 0.68f, b->sign);
        for (int stripe = -2; stripe <= 2; stripe++) {
            DrawCube((Vector3){b->position.x + (float)stripe*b->width*0.16f,
                               3.08f, b->position.z + frontSign*(b->depth*0.5f + 0.80f)},
                     b->width*0.12f, 0.12f, 0.06f,
                     (stripe % 2 == 0) ? C_WHITE : ColorScale(b->sign, 0.78f));
        }
        DrawCube((Vector3){b->position.x, 3.43f,
                           b->position.z + frontSign*(b->depth*0.5f + 0.48f)},
                 b->width*0.95f, 0.12f, 0.72f, b->trim);
    }

    /* Residential balconies in a repeating floor rhythm. */
    if (distance < 138.0f && (b->style == 0 || b->style == 1)) {
        int balconyFloors = (int)(b->height/6.0f);
        if (balconyFloors > 8) balconyFloors = 8;
        for (int f = 1; f < balconyFloors; f++) {
            if (((b->seed >> (unsigned int)(f % 16)) & 1u) == 0u) continue;
            float y = 2.85f + (float)f*5.9f;
            if (y > b->height - 1.4f) break;
            float balconyW = b->width*0.55f;
            float balconyZ = b->position.z + frontSign*(b->depth*0.5f + 0.66f);
            DrawCube((Vector3){b->position.x, y, balconyZ}, balconyW, 0.13f,
                     1.10f, b->trim);
            DrawCube((Vector3){b->position.x, y + 0.48f, balconyZ + frontSign*0.48f},
                     balconyW, 0.10f, 0.10f, C_WHITE);
            DrawCube((Vector3){b->position.x - balconyW*0.5f, y + 0.25f, balconyZ + frontSign*0.48f},
                     0.10f, 0.48f, 0.10f, C_WHITE);
            DrawCube((Vector3){b->position.x + balconyW*0.5f, y + 0.25f, balconyZ + frontSign*0.48f},
                     0.10f, 0.48f, 0.10f, C_WHITE);
        }
    }

    /* Rooftop service room, water tank or antenna. */
    if ((b->seed % 3u) == 0u) {
        DrawCube((Vector3){b->position.x - b->width*0.20f,
                           b->height + 1.20f,
                           b->position.z - b->depth*0.16f},
                 b->width*0.34f, 1.55f, b->depth*0.30f,
                 ColorScale(b->wall, 0.90f));
        DrawCylinder((Vector3){b->position.x + b->width*0.22f,
                               b->height + 1.02f, b->position.z - b->depth*0.18f},
                     0.72f, 0.72f, 1.30f, 10, (Color){ 105, 139, 158, 255 });
    } else if ((b->seed % 4u) == 0u) {
        DrawCube((Vector3){b->position.x, b->height + 1.55f, b->position.z},
                 0.18f, 2.8f, 0.18f, b->trim);
        DrawSphere((Vector3){b->position.x, b->height + 3.05f, b->position.z},
                   0.23f, C_RED);
    }
}

/* Query only nearby grid cells instead of scanning all 10,000 buildings each frame. */
static void DrawNearbyBuildings(Vector3 focus) {
    float extent = DRAW_DISTANCE + CITY_BLOCK;
    int minX = (int)floorf((focus.x - extent + CITY_HALF)/CITY_BLOCK);
    int maxX = (int)floorf((focus.x + extent + CITY_HALF)/CITY_BLOCK);
    int minZ = (int)floorf((focus.z - extent + CITY_HALF)/CITY_BLOCK);
    int maxZ = (int)floorf((focus.z + extent + CITY_HALF)/CITY_BLOCK);
    if (minX < 0) minX = 0;
    if (minZ < 0) minZ = 0;
    if (maxX >= CITY_GRID_SIDE) maxX = CITY_GRID_SIDE - 1;
    if (maxZ >= CITY_GRID_SIDE) maxZ = CITY_GRID_SIDE - 1;
    if (minX > maxX || minZ > maxZ) return;

    for (int cx = minX; cx <= maxX; cx++) {
        for (int cz = minZ; cz <= maxZ; cz++) {
            unsigned char count = gCellBuildingCounts[cx][cz];
            for (int slot = 0; slot < count; slot++) {
                unsigned int index = gCellBuildingIndices[cx][cz][slot];
                if (index < (unsigned int)gBuildingCount)
                    DrawBuilding(&gBuildings[index], focus);
            }
        }
    }
}

static void DrawTree(float x, float z, float scale, int species) {
    DrawCylinder((Vector3){x, 0.95f*scale, z}, 0.22f*scale, 0.28f*scale,
                 1.9f*scale, 8, (Color){119, 82, 53, 255});
    /* Species 0: broadleaf, 1: pine, 2: palm-like crown. */
    if (species == 2) {
        DrawCylinder((Vector3){x, 2.95f*scale, z}, 0.05f*scale, 0.05f*scale,
                     4.0f*scale, 7, (Color){121, 89, 57, 255});
        for (int k = 0; k < 4; k++) {
            float y = (1.55f + (float)k*0.64f)*scale;
            DrawCylinder((Vector3){x, y, z}, (1.08f - (float)k*0.17f)*scale,
                         0.04f*scale, 1.05f*scale, 7,
                         (Color){42, (unsigned char)(105 + k*8), 64, 255});
        }
    } else if (species == 1) {
        DrawSphere((Vector3){x, 2.20f*scale, z}, 1.05f*scale,
                   (Color){58, 130, 73, 255});
        for (int k = 0; k < 6; k++) {
            float a = (float)k*PI_F/3.0f;
            DrawLine3D((Vector3){x, 2.95f*scale, z},
                       (Vector3){x + cosf(a)*1.8f*scale, 2.75f*scale,
                                 z + sinf(a)*1.8f*scale},
                       (Color){62, 137, 76, 255});
        }
    } else {
        DrawSphere((Vector3){x, 2.15f*scale, z}, 1.18f*scale,
                   (Color){51, 133, 76, 255});
        DrawSphere((Vector3){x - 0.58f*scale, 2.0f*scale, z + 0.12f*scale},
                   0.70f*scale, (Color){66, 151, 84, 255});
        DrawSphere((Vector3){x + 0.55f*scale, 2.08f*scale, z - 0.18f*scale},
                   0.72f*scale, (Color){43, 119, 69, 255});
    }
}

static void AddWorldTree(float x, float z, float scale, int species) {
    if (gTreeCount >= TREE_CAPACITY) return;
    gTrees[gTreeCount++] = (WorldTree){x, z, scale, species};
}

static void InitWorldTrees(void) {
    gTreeCount = 0;
    for (int bx = -25; bx < 25; bx++) {
        for (int bz = -25; bz < 25; bz++) {
            unsigned int seed = Hash2(bx, bz, 991);
            float cx = (float)bx*CITY_BLOCK + CITY_BLOCK*0.5f;
            float cz = (float)bz*CITY_BLOCK + CITY_BLOCK*0.5f;
            /* Keep the civic plaza, shops, runway and harbour clear. */
            if (abs(bx) <= 1 && abs(bz) <= 1) continue;
            if (bx <= -22 && bz >= -3 && bz <= 3) continue;
            if (bx >= 21 && bz <= -21) continue;
            if ((seed % 3u) != 0u) continue;
            AddWorldTree(cx - 3.8f, cz - 2.2f, 0.72f + (float)(seed%40u)*0.01f,
                         (int)(seed%3u));
            if ((seed % 6u) == 0u)
                AddWorldTree(cx + 4.0f, cz + 3.1f, 0.65f + (float)((seed>>4)%35u)*0.01f,
                             (int)((seed>>3)%3u));
        }
    }
    /* Coastal promenade parks and neighbourhood green belts. */
    for (int i = 0; i < 26; i++) {
        float x = -330.0f + (float)(i%13)*55.0f;
        float z = (i < 13) ? -356.0f : 342.0f;
        AddWorldTree(x, z, 0.80f + (float)(i%4)*0.10f, i%3);
    }
}

static void DrawStreetLamp(float x, float z) {
    DrawCylinder((Vector3){x, 3.2f, z}, 0.10f, 0.10f, 6.4f, 8,
                 (Color){ 63, 72, 82, 255 });
    DrawCube((Vector3){x + 0.68f, 6.25f, z}, 1.45f, 0.12f, 0.12f,
             (Color){ 63, 72, 82, 255 });
    DrawCube((Vector3){x + 1.23f, 6.05f, z}, 0.52f, 0.18f, 0.52f,
             (Color){ 244, 218, 151, 255 });
    if (gNightAmount > 0.25f) {
        DrawSphere((Vector3){x + 1.23f, 5.95f, z}, 0.48f,
                   Fade((Color){255, 220, 145, 255}, gNightAmount*0.30f));
    }
}

static void DrawCentralPlaza(float time) {
    DrawCube((Vector3){24.0f, 0.04f, 24.0f}, 31.0f, 0.18f, 31.0f,
             (Color){ 176, 176, 162, 255 });
    DrawCube((Vector3){24.0f, 0.15f, 24.0f}, 19.0f, 0.12f, 19.0f,
             (Color){ 202, 193, 170, 255 });

    /* Fountain with a softly moving water column. */
    DrawCylinder((Vector3){24.0f, 0.60f, 24.0f}, 4.8f, 4.8f, 0.65f,
                 32, (Color){ 91, 114, 126, 255 });
    DrawCylinder((Vector3){24.0f, 1.02f, 24.0f}, 3.9f, 3.9f, 0.18f,
                 32, (Color){ 92, 173, 196, 255 });
    float waterHeight = 1.5f + 0.18f*sinf(time*2.4f);
    DrawCylinder((Vector3){24.0f, 1.65f + waterHeight*0.5f, 24.0f},
                 0.52f, 0.08f, waterHeight, 16,
                 (Color){ 126, 202, 221, 255 });
    DrawSphere((Vector3){24.0f, 1.55f + waterHeight, 24.0f}, 0.28f,
               (Color){ 176, 230, 237, 255 });

    for (int i = 0; i < 4; i++) {
        float a = (float)i*PI_F*0.5f;
        float x = 24.0f + cosf(a)*11.2f;
        float z = 24.0f + sinf(a)*11.2f;
        DrawCube((Vector3){x, 0.65f, z}, 4.2f, 0.55f, 0.70f,
                 (Color){ 110, 79, 57, 255 });
        DrawCube((Vector3){x, 0.98f, z}, 4.2f, 0.12f, 0.76f,
                 (Color){ 183, 143, 101, 255 });
    }

    DrawTree(10.0f, 10.0f, 0.92f, 0);
    DrawTree(38.0f, 10.0f, 0.92f, 2);
    DrawTree(10.0f, 38.0f, 0.92f, 1);
    DrawTree(38.0f, 38.0f, 0.92f, 0);
}

/* ---- Large-world landmarks -------------------------------------------- */
static void DrawAirport(Vector3 focus) {
    if (Distance2D(focus.x, focus.z, -1080.0f, 0.0f) > DRAW_DISTANCE + 80.0f) return;
    DrawCube((Vector3){-1080.0f, 0.025f, 0.0f}, 25.0f, 0.18f, 224.0f,
             (Color){ 48, 53, 62, 255 });
    DrawCube((Vector3){-1080.0f, 0.13f, 0.0f}, 0.55f, 0.035f, 198.0f, C_WHITE);
    for (int z = -92; z <= 92; z += 24) {
        DrawCube((Vector3){-1080.0f, 0.15f, (float)z}, 10.0f, 0.035f, 1.1f, C_WHITE);
    }
    /* Terminal and control tower. */
    DrawCube((Vector3){-1042.0f, 5.0f, -82.0f}, 44.0f, 10.0f, 25.0f,
             (Color){ 174, 190, 197, 255 });
    DrawCube((Vector3){-1042.0f, 9.0f, -68.8f}, 38.0f, 2.0f, 1.0f, C_WINDOW);
    DrawCube((Vector3){-1020.0f, 17.0f, -76.0f}, 7.0f, 24.0f, 7.0f,
             (Color){ 125, 145, 155, 255 });
    DrawCube((Vector3){-1020.0f, 30.0f, -76.0f}, 9.0f, 4.0f, 9.0f, C_GLASS_DARK);
    DrawCube((Vector3){-1020.0f, 33.0f, -76.0f}, 4.0f, 0.35f, 4.0f, C_RED);
    /* Parked passenger aircraft, stylised from primitives. */
    DrawCylinderEx((Vector3){-1080.0f, 3.1f, 0.0f},
                   (Vector3){-1080.0f, 3.1f, -20.0f}, 1.7f, 1.7f, 12, C_WHITE);
    DrawCube((Vector3){-1080.0f, 3.3f, -6.0f}, 30.0f, 0.28f, 7.5f, C_WHITE);
    DrawCube((Vector3){-1080.0f, 3.3f, 1.0f}, 9.0f, 0.18f, 5.0f, C_WHITE);
    DrawCube((Vector3){-1080.0f, 3.4f, -12.0f}, 4.8f, 0.18f, 8.5f, C_WHITE);
    DrawCube((Vector3){-1080.0f, 4.3f, -9.0f}, 4.2f, 1.0f, 4.6f, C_GLASS_DARK);
}

static void DrawHarbor(Vector3 focus, float time) {
    if (Distance2D(focus.x, focus.z, 1020.0f, -1060.0f) > DRAW_DISTANCE + 100.0f) return;
    DrawCube((Vector3){1020.0f, 0.02f, -1080.0f}, 940.0f, 0.28f, 38.0f,
             (Color){ 112, 117, 118, 255 });
    for (int i = 0; i < 12; i++) {
        float x = 940.0f + (float)(i%4)*23.0f;
        float z = -1066.0f - (float)(i/4)*12.0f;
        Color c = (i%3 == 0) ? (Color){190, 91, 61, 255} :
                  ((i%3 == 1) ? (Color){58, 112, 145, 255} : (Color){190, 157, 73, 255});
        DrawCube((Vector3){x, 3.0f, z}, 19.0f, 6.0f, 10.0f, c);
        DrawCube((Vector3){x, 6.2f, z}, 19.2f, 0.16f, 10.2f, ColorScale(c, 0.72f));
    }
    DrawCube((Vector3){1020.0f, 0.25f, -1120.0f}, 180.0f, 0.55f, 50.0f,
             (Color){ 100, 103, 101, 255 });
    for (int i = 0; i < 5; i++) {
        float x = 955.0f + (float)i*32.0f;
        DrawCylinder((Vector3){x, 6.0f, -1100.0f}, 0.45f, 0.45f, 12.0f, 8,
                     (Color){ 53, 64, 73, 255 });
        DrawLine3D((Vector3){x, 12.0f, -1100.0f},
                   (Vector3){x + 5.0f*sinf(time*0.2f), 20.0f, -1095.0f}, C_GOLD);
    }
}

static void DrawDriftDesert(Vector3 focus, float time) {
    if (Distance2D(focus.x, focus.z, SAND_CENTER_X, SAND_CENTER_Z) > DRAW_DISTANCE + 240.0f) return;
    Color sand = (Color){210, 177, 116, 255};
    Color darkSand = (Color){177, 137, 83, 255};
    DrawCube((Vector3){SAND_CENTER_X, -0.045f, SAND_CENTER_Z}, 520.0f, 0.20f, 560.0f, sand);
    /* Rolling dune ridges around the playable drift basin. */
    for (int i = 0; i < 10; i++) {
        float x = SAND_CENTER_X - 220.0f + (float)(i%5)*110.0f;
        float z = (i < 5) ? (SAND_CENTER_Z - 260.0f) : (SAND_CENTER_Z + 260.0f);
        rlPushMatrix();
        rlTranslatef(x, 1.0f, z);
        rlScalef(1.75f, 0.48f, 1.30f);
        DrawSphere((Vector3){0.0f, 7.0f + (float)(i%3)*1.1f, 0.0f},
                   21.0f, (i%2) ? darkSand : (Color){220, 190, 135, 255});
        rlPopMatrix();
    }
    /* A large circular drift bowl and a second linked donut track. */
    for (int ring = 0; ring < 2; ring++) {
        float radius = 46.0f + (float)ring*1.35f;
        for (int i = 0; i < 72; i++) {
            float a0 = (float)i/72.0f*2.0f*PI_F;
            float a1 = (float)(i+1)/72.0f*2.0f*PI_F;
            DrawLine3D((Vector3){SAND_CENTER_X + cosf(a0)*radius, 0.08f,
                                  SAND_CENTER_Z + sinf(a0)*radius},
                       (Vector3){SAND_CENTER_X + cosf(a1)*radius, 0.08f,
                                  SAND_CENTER_Z + sinf(a1)*radius},
                       Fade(darkSand, 0.72f));
        }
    }
    for (int loop = 0; loop < 2; loop++) {
        float cx = SAND_CENTER_X + (loop == 0 ? -94.0f : 94.0f);
        for (int i = 0; i < 64; i++) {
            float a0 = (float)i/64.0f*2.0f*PI_F;
            float a1 = (float)(i+1)/64.0f*2.0f*PI_F;
            DrawLine3D((Vector3){cx + cosf(a0)*31.0f, 0.09f,
                                  SAND_CENTER_Z + sinf(a0)*42.0f},
                       (Vector3){cx + cosf(a1)*31.0f, 0.09f,
                                  SAND_CENTER_Z + sinf(a1)*42.0f},
                       Fade((Color){142, 105, 64, 255}, 0.82f));
        }
    }
    /* Start/finish gate, flag poles, floodlights and viewing platforms. */
    DrawCube((Vector3){SAND_CENTER_X, 6.0f, SAND_CENTER_Z - 72.0f}, 25.0f, 0.5f, 0.7f,
             (Color){34, 42, 49, 255});
    DrawCube((Vector3){SAND_CENTER_X - 12.0f, 3.2f, SAND_CENTER_Z - 72.0f}, 0.45f, 6.4f, 0.45f, C_DARK);
    DrawCube((Vector3){SAND_CENTER_X + 12.0f, 3.2f, SAND_CENTER_Z - 72.0f}, 0.45f, 6.4f, 0.45f, C_DARK);
    DrawCube((Vector3){SAND_CENTER_X, 6.45f, SAND_CENTER_Z - 72.0f}, 24.0f, 0.65f, 0.35f,
             (Color){197, 66, 54, 255});
    DrawCube((Vector3){SAND_CENTER_X, 4.2f, SAND_CENTER_Z - 94.0f}, 32.0f, 1.0f, 0.7f,
             (Color){30, 39, 49, 255});
    DrawCube((Vector3){SAND_CENTER_X, 4.25f, SAND_CENTER_Z - 93.55f}, 23.0f, 0.22f, 0.06f,
             C_GOLD);
    for (int i = 0; i < 8; i++) {
        float x = SAND_CENTER_X - 220.0f + (float)i*62.0f;
        DrawCylinder((Vector3){x, 5.0f, (SAND_CENTER_Z - 172.0f)}, 0.20f, 0.20f, 10.0f, 8,
                     (Color){57, 64, 69, 255});
        if (gNightAmount > 0.25f)
            DrawSphere((Vector3){x, 9.8f, (SAND_CENTER_Z - 172.0f)}, 1.15f,
                       Fade((Color){255, 223, 162, 255}, 0.55f));
    }
    /* Static spectator stands on the southern edge. */
    DrawCube((Vector3){SAND_CENTER_X, 1.2f, (SAND_CENTER_Z + 175.0f)}, 150.0f, 2.4f, 14.0f,
             (Color){86, 91, 89, 255});
    for (int row = 0; row < 3; row++)
        DrawCube((Vector3){SAND_CENTER_X, 2.2f + (float)row*0.55f,
                            (SAND_CENTER_Z + 167.0f) + (float)row*2.0f},
                 138.0f, 0.20f, 1.1f, (Color){145, 149, 143, 255});
    if (gNightAmount > 0.35f) {
        float pulse = 0.8f + 0.2f*sinf(time*2.0f);
        DrawSphere((Vector3){SAND_CENTER_X - 82.0f, 10.0f, SAND_CENTER_Z - 42.0f},
                   1.4f*pulse, Fade(C_GOLD, 0.55f));
        DrawSphere((Vector3){SAND_CENTER_X + 82.0f, 10.0f, SAND_CENTER_Z - 42.0f},
                   1.4f*pulse, Fade(C_GOLD, 0.55f));
    }
}

static void DrawWorldLandmarks(Vector3 focus, float time) {
    DrawAirport(focus);
    DrawHarbor(focus, time);
    DrawDriftDesert(focus, time);
    /* Distant low-poly hills give the outer districts a defined skyline. */
    if (focus.z > 900.0f) {
        for (int i = 0; i < 9; i++) {
            float x = -1050.0f + (float)i*260.0f;
            float h = 38.0f + (float)((i*17)%43);
            DrawCylinder((Vector3){x, h*0.40f, 1300.0f}, 38.0f, 10.0f, h, 7,
                         (Color){ 92, 112, 91, 255 });
        }
    }
}

/* ---- Vehicles ---------------------------------------------------------- */
static void DrawVehicleModel(const Vehicle *v, bool playerVehicle) {
    rlPushMatrix();
    rlTranslatef(v->position.x, v->position.y, v->position.z);
    rlRotatef(v->yaw*RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

    float bodyY = (v->model == 1 || v->model == 6) ? 0.72f :
                  ((v->model == 2 || v->model == 4 || v->model == 5) ? 0.96f : 0.80f);
    float bodyW = (v->model == 1 || v->model == 3 || v->model == 4 || v->model == 5) ? 2.18f : 1.94f;
    float bodyL = (v->model == 1 || v->model == 3 || v->model == 4 || v->model == 5) ? 4.95f : 4.22f;
    Color paint = v->paint;
    Color glass = (Color){ 65, 121, 151, 255 };

    /* Main body, hood and trunk. */
    DrawCube((Vector3){0.0f, bodyY, 0.0f}, bodyW, 0.72f, bodyL, paint);
    DrawCubeWires((Vector3){0.0f, bodyY, 0.0f}, bodyW, 0.72f, bodyL,
                  ColorScale(paint, 0.62f));
    DrawCube((Vector3){0.0f, 1.18f, -1.12f}, bodyW*0.83f, 0.28f, 1.1f,
             ColorScale(paint, 1.12f));
    DrawCube((Vector3){0.0f, 1.17f, 1.38f}, bodyW*0.82f, 0.25f, 0.76f,
             ColorScale(paint, 0.88f));

    /* Cabin and windows. */
    float cabinHeight = (v->model == 1 || v->model == 6) ? 0.56f :
                        ((v->model == 2 || v->model == 4 || v->model == 5) ? 1.02f : 0.82f);
    float cabinY = (v->model == 1 || v->model == 6) ? 1.34f :
                   ((v->model == 2 || v->model == 4 || v->model == 5) ? 1.55f : 1.47f);
    DrawCube((Vector3){0.0f, cabinY, 0.24f}, bodyW*0.78f, cabinHeight,
             (v->model == 3 || v->model == 5) ? 1.42f : 2.0f, ColorScale(paint, 0.94f));
    DrawCube((Vector3){0.0f, 1.52f, -0.79f}, bodyW*0.72f, 0.58f, 0.08f, glass);
    DrawCube((Vector3){0.0f, 1.52f, 1.27f}, bodyW*0.70f, 0.55f, 0.08f, glass);
    DrawCube((Vector3){-bodyW*0.405f, 1.51f, 0.20f}, 0.08f, 0.58f, 1.45f, glass);
    DrawCube((Vector3){ bodyW*0.405f, 1.51f, 0.20f}, 0.08f, 0.58f, 1.45f, glass);

    /* Headlights, tail lamps and bumpers. */
    DrawCube((Vector3){-0.66f, 0.91f, -bodyL*0.5f - 0.035f},
             0.42f, 0.22f, 0.09f, (Color){ 255, 240, 190, 255 });
    DrawCube((Vector3){ 0.66f, 0.91f, -bodyL*0.5f - 0.035f},
             0.42f, 0.22f, 0.09f, (Color){ 255, 240, 190, 255 });
    DrawCube((Vector3){-0.67f, 0.91f, bodyL*0.5f + 0.035f},
             0.38f, 0.24f, 0.09f, C_RED);
    DrawCube((Vector3){ 0.67f, 0.91f, bodyL*0.5f + 0.035f},
             0.38f, 0.24f, 0.09f, C_RED);
    DrawCube((Vector3){0.0f, 0.47f, -bodyL*0.5f - 0.08f}, bodyW*0.94f, 0.18f, 0.18f,
             (Color){ 64, 68, 75, 255 });
    DrawCube((Vector3){0.0f, 0.47f, bodyL*0.5f + 0.08f}, bodyW*0.94f, 0.18f, 0.18f,
             (Color){ 64, 68, 75, 255 });

    /* Four low-poly tyres and silver hubs. */
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            float x = (float)sx*(bodyW*0.51f);
            float z = (float)sz*(bodyL*0.31f);
            DrawSphere((Vector3){x, 0.50f, z}, 0.43f,
                       (Color){ 24, 28, 34, 255 });
            DrawSphere((Vector3){x*1.015f, 0.50f, z}, 0.20f,
                       (Color){ 157, 166, 174, 255 });
        }
    }

    /* Model-specific silhouettes: sport rear wing, SUV roof rack, pickup bed. */
    if (v->model == 1) {
        DrawCube((Vector3){0.0f, 1.48f, 1.72f}, 1.75f, 0.10f, 0.22f, C_DARK);
        DrawCube((Vector3){0.0f, 1.30f, 1.70f}, 0.10f, 0.36f, 0.14f, C_DARK);
    } else if (v->model == 2) {
        DrawCube((Vector3){0.0f, 2.12f, 0.20f}, 1.55f, 0.08f, 1.60f, C_DARK);
    } else if (v->model == 3) {
        DrawCube((Vector3){0.0f, 1.18f, 1.16f}, bodyW*0.78f, 0.48f, 1.02f,
                 ColorScale(paint, 0.76f));
    } else if (v->model == 4) {
        DrawCube((Vector3){0.0f, 2.12f, 0.10f}, 0.70f, 0.12f, 0.42f, C_WHITE);
        DrawCube((Vector3){-0.34f, 2.24f, 0.10f}, 0.22f, 0.12f, 0.32f, C_BLUE);
        DrawCube((Vector3){0.34f, 2.24f, 0.10f}, 0.22f, 0.12f, 0.32f, C_RED);
    } else if (v->model == 5) {
        DrawCube((Vector3){0.0f, 1.28f, 1.23f}, 1.72f, 0.82f, 1.28f, ColorScale(paint, 0.88f));
        DrawCube((Vector3){0.0f, 1.88f, 0.22f}, 1.50f, 0.20f, 1.55f, glass);
    } else if (v->model == 6) {
        DrawCube((Vector3){0.0f, 1.10f, 0.22f}, bodyW*0.72f, 0.12f, 2.00f, C_GOLD);
        DrawCube((Vector3){0.0f, 1.72f, 0.10f}, bodyW*0.74f, 0.12f, 1.65f, C_DARK);
    } else if (v->model == 7) {
        DrawCube((Vector3){0.0f, 2.05f, 0.12f}, 0.44f, 0.14f, 0.34f, C_GOLD);
        DrawCube((Vector3){0.0f, 0.91f, 0.0f}, bodyW*0.97f, 0.07f, bodyL*0.82f, C_WHITE);
    } else if (v->model == 8) {
        DrawCube((Vector3){0.0f, 1.44f, 1.70f}, 1.55f, 0.10f, 0.22f, C_DARK);
        DrawCube((Vector3){0.0f, 0.88f, -0.20f}, 0.24f, 0.08f, 3.60f, C_WHITE);
    } else if (v->model == 9) {
        DrawCube((Vector3){0.0f, 1.20f, -0.72f}, 0.62f, 0.17f, 0.80f, C_DARK);
        DrawCube((Vector3){0.0f, 1.50f, 1.60f}, 1.85f, 0.10f, 0.18f, C_DARK);
    } else if (v->model == 10) {
        DrawCube((Vector3){0.0f, 1.85f, -0.10f}, 1.45f, 0.10f, 1.90f, glass);
        DrawCube((Vector3){0.0f, 0.48f, -2.10f}, 1.70f, 0.12f, 0.10f, C_GOLD);
    } else if (v->model == 11) {
        DrawCube((Vector3){-0.78f, 1.90f, 0.10f}, 0.10f, 1.00f, 1.95f, C_DARK);
        DrawCube((Vector3){0.78f, 1.90f, 0.10f}, 0.10f, 1.00f, 1.95f, C_DARK);
        DrawCube((Vector3){0.0f, 2.40f, 0.10f}, 1.65f, 0.10f, 1.95f, C_DARK);
    }
    if (playerVehicle) {
        DrawCube((Vector3){0.0f, 1.91f, -0.15f}, 0.18f, 0.08f, 0.32f,
                 (Color){ 235, 235, 235, 255 });
    }
    rlPopMatrix();
}

static void DrawSandMeetCars(Vector3 focus) {
    if (Distance2D(focus.x, focus.z, SAND_CENTER_X, SAND_CENTER_Z) > 230.0f) return;
    static const Color paints[8] = {
        {215, 62, 55, 255}, {51, 112, 188, 255}, {236, 221, 181, 255},
        {42, 46, 55, 255}, {208, 150, 55, 255}, {83, 149, 107, 255},
        {139, 81, 161, 255}, {221, 222, 228, 255}
    };
    for (int i = 0; i < 8; i++) {
        Vehicle display = {0};
        display.position = (Vector3){SAND_CENTER_X - 84.0f + (float)(i%4)*56.0f,
                                    0.0f, SAND_CENTER_Z + 170.0f + (float)(i/4)*15.0f};
        display.yaw = PI_F;
        display.paint = paints[i];
        display.model = (i*3 + 1)%12;
        DrawVehicleModel(&display, false);
    }
}

static void InitTraffic(void) {
    static const Color paints[12] = {
        {219, 87, 70, 255}, {54, 120, 177, 255}, {224, 221, 208, 255},
        {51, 62, 73, 255}, {205, 165, 75, 255}, {93, 156, 119, 255},
        {123, 83, 160, 255}, {229, 229, 220, 255}, {231, 123, 52, 255},
        {39, 42, 51, 255}, {102, 187, 191, 255}, {170, 178, 187, 255}
    };

    for (int i = 0; i < TRAFFIC_COUNT; i++) {
        Vehicle *v = &gTraffic[i];
        int road = (i%47) - 23;
        float coord = (float)road*48.0f;
        v->speed = 6.0f + (float)(i%5)*1.55f;
        v->paint = paints[i%12];
        v->model = i%12;
        v->position.y = 0.0f;
        if ((i%2) == 0) {
            v->position.x = -CITY_HALF + 16.0f + (float)((i*73)%((int)(CITY_HALF*2.0f - 32.0f)));
            v->position.z = coord + ((i%4 < 2) ? -3.2f : 3.2f);
            v->yaw = (i%4 < 2) ? -PI_F*0.5f : PI_F*0.5f;
        } else {
            v->position.x = coord + ((i%4 < 2) ? -3.2f : 3.2f);
            v->position.z = -CITY_HALF + 16.0f + (float)((i*59)%((int)(CITY_HALF*2.0f - 32.0f)));
            v->yaw = (i%4 < 2) ? 0.0f : PI_F;
        }
    }
}

static void UpdateTraffic(float dt) {
    for (int i = 0; i < TRAFFIC_COUNT; i++) {
        Vehicle *v = &gTraffic[i];
        float dx = -sinf(v->yaw)*v->speed*dt;
        float dz = -cosf(v->yaw)*v->speed*dt;
        v->position.x += dx;
        v->position.z += dz;
        if (v->position.x > CITY_HALF + 8.0f) v->position.x = -CITY_HALF - 8.0f;
        if (v->position.x < -CITY_HALF - 8.0f) v->position.x = CITY_HALF + 8.0f;
        if (v->position.z > CITY_HALF + 8.0f) v->position.z = -CITY_HALF - 8.0f;
        if (v->position.z < -CITY_HALF - 8.0f) v->position.z = CITY_HALF + 8.0f;
    }
}

/* ---- People ------------------------------------------------------------ */
static void DrawHuman(Vector3 p, float yaw, float phase,
                      Color shirt, Color trousers, Color skin,
                      bool isPlayer, bool running) {
    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(yaw*RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

    float swing = (running ? 32.0f : 20.0f)*sinf(phase);
    float bob = (running ? 0.075f : 0.035f)*fabsf(sinf(phase));
    Color shoe = (Color){ 34, 39, 46, 255 };
    Color hair = (Color){ 47, 39, 35, 255 };

    /* Torso, collar and belt. */
    DrawCube((Vector3){0.0f, 1.12f + bob, 0.0f}, 0.66f, 0.86f, 0.38f, shirt);
    DrawCube((Vector3){0.0f, 0.68f + bob, 0.0f}, 0.62f, 0.12f, 0.40f,
             (Color){ 54, 48, 47, 255 });
    DrawCube((Vector3){0.0f, 1.57f + bob, -0.01f}, 0.34f, 0.14f, 0.34f, skin);

    /* Head, hair and face. */
    DrawSphere((Vector3){0.0f, 1.86f + bob, 0.0f}, 0.31f, skin);
    DrawSphere((Vector3){0.0f, 2.04f + bob, 0.035f}, 0.285f, hair);
    DrawCube((Vector3){0.0f, 1.86f + bob, -0.274f}, 0.12f, 0.055f, 0.035f,
             (Color){ 44, 43, 43, 255 });

    /* Arms pivot from shoulders and swing while walking. */
    rlPushMatrix();
    rlTranslatef(-0.40f, 1.43f + bob, 0.0f);
    rlRotatef(swing, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.34f, 0.0f}, 0.20f, 0.68f, 0.23f, shirt);
    DrawSphere((Vector3){0.0f, -0.72f, -0.01f}, 0.12f, skin);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.40f, 1.43f + bob, 0.0f);
    rlRotatef(-swing, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.34f, 0.0f}, 0.20f, 0.68f, 0.23f, shirt);
    DrawSphere((Vector3){0.0f, -0.72f, -0.01f}, 0.12f, skin);
    rlPopMatrix();

    /* Legs and shoes. */
    rlPushMatrix();
    rlTranslatef(-0.18f, 0.67f + bob, 0.0f);
    rlRotatef(-swing*0.72f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.30f, 0.0f}, 0.25f, 0.60f, 0.29f, trousers);
    DrawCube((Vector3){0.0f, -0.59f, -0.10f}, 0.28f, 0.16f, 0.42f, shoe);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.18f, 0.67f + bob, 0.0f);
    rlRotatef(swing*0.72f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.30f, 0.0f}, 0.25f, 0.60f, 0.29f, trousers);
    DrawCube((Vector3){0.0f, -0.59f, -0.10f}, 0.28f, 0.16f, 0.42f, shoe);
    rlPopMatrix();

    if (isPlayer) {
        DrawCube((Vector3){0.0f, 1.12f + bob, 0.25f}, 0.42f, 0.48f, 0.20f,
                 (Color){ 42, 57, 80, 255 });
    }
    rlPopMatrix();
}

static void DrawPlayerWeapon(Vector3 player, float yaw, int weaponIndex, float bob) {
    Color metal = (Color){48, 55, 64, 255};
    Color darkMetal = (Color){24, 29, 36, 255};
    Color grip = (Color){91, 65, 47, 255};
    rlPushMatrix();
    rlTranslatef(player.x, player.y, player.z);
    rlRotatef(yaw*RAD_TO_DEG, 0.0f, 1.0f, 0.0f);
    float y = 1.08f + bob;
    if (weaponIndex == 1) {
        /* Long rifle: receiver, barrel, stock, handguard and sight. */
        DrawCube((Vector3){0.39f, y, -0.51f}, 0.18f, 0.16f, 0.66f, metal);
        DrawCube((Vector3){0.39f, y + 0.015f, -1.02f}, 0.075f, 0.075f, 0.55f, darkMetal);
        DrawCube((Vector3){0.39f, y - 0.04f, -0.04f}, 0.17f, 0.13f, 0.38f, grip);
        DrawCube((Vector3){0.39f, y + 0.14f, -0.60f}, 0.07f, 0.07f, 0.20f, darkMetal);
        DrawCube((Vector3){0.39f, y - 0.16f, -0.52f}, 0.12f, 0.19f, 0.16f, darkMetal);
        if (gMuzzleFlashTimer > 0.0f) DrawSphere((Vector3){0.39f, y, -1.34f}, 0.19f, (Color){255, 215, 102, 230});
    } else if (weaponIndex == 2) {
        /* Heavy shotgun: wide receiver, twin-thick barrel and foregrip. */
        DrawCube((Vector3){0.39f, y, -0.49f}, 0.22f, 0.19f, 0.58f, metal);
        DrawCube((Vector3){0.35f, y + 0.015f, -1.02f}, 0.075f, 0.075f, 0.48f, darkMetal);
        DrawCube((Vector3){0.44f, y + 0.015f, -1.02f}, 0.075f, 0.075f, 0.48f, darkMetal);
        DrawCube((Vector3){0.39f, y - 0.03f, -0.01f}, 0.18f, 0.15f, 0.42f, grip);
        DrawCube((Vector3){0.39f, y - 0.16f, -0.47f}, 0.15f, 0.20f, 0.18f, darkMetal);
        if (gMuzzleFlashTimer > 0.0f) DrawSphere((Vector3){0.39f, y, -1.29f}, 0.23f, (Color){255, 203, 89, 230});
    } else {
        /* Compact pistol with slide, barrel and grip. */
        DrawCube((Vector3){0.40f, y + 0.02f, -0.48f}, 0.16f, 0.13f, 0.43f, metal);
        DrawCube((Vector3){0.40f, y + 0.025f, -0.77f}, 0.065f, 0.065f, 0.20f, darkMetal);
        DrawCube((Vector3){0.40f, y - 0.13f, -0.34f}, 0.12f, 0.24f, 0.14f, grip);
        DrawCube((Vector3){0.40f, y + 0.115f, -0.50f}, 0.05f, 0.035f, 0.12f, darkMetal);
        if (gMuzzleFlashTimer > 0.0f) DrawSphere((Vector3){0.40f, y + 0.02f, -0.91f}, 0.15f, (Color){255, 220, 120, 230});
    }
    /* A visible glove/hand joins the weapon to the character silhouette. */
    DrawSphere((Vector3){0.40f, y - 0.20f, -0.24f}, 0.13f, (Color){190, 145, 111, 255});
    rlPopMatrix();
}

static void DrawShotEffect(void) {
    if (gShotEffect.life <= 0.0f) return;
    float alpha = ClampF(gShotEffect.life/0.11f, 0.0f, 1.0f);
    DrawLine3D(gShotEffect.start, gShotEffect.end, Fade((Color){255, 223, 126, 255}, alpha));
    DrawSphere(gShotEffect.end, 0.055f + alpha*0.035f, Fade((Color){255, 180, 74, 255}, alpha));
}

static void InitPedestrians(void) {
    static const Color shirts[] = {
        { 42, 116, 185, 255 }, { 195, 82, 73, 255 },
        { 68, 143, 104, 255 }, { 192, 146, 63, 255 },
        { 120, 91, 165, 255 }, { 220, 220, 213, 255 },
        { 56, 82, 105, 255 }
    };
    static const Color pants[] = {
        { 44, 55, 73, 255 }, { 84, 84, 80, 255 },
        { 90, 102, 116, 255 }, { 70, 64, 58, 255 }
    };

    for (int i = 0; i < WALKER_COUNT; i++) {
        Pedestrian *p = &gPedestrians[i];
        p->isStatic = (i >= 108);
        p->alongX = ((i%2) == 0);
        p->direction = ((i%3) == 0) ? -1 : 1;
        p->routeMin = -CITY_HALF + 24.0f;
        p->routeMax = CITY_HALF - 24.0f;
        p->speed = 1.0f + (float)(i%5)*0.27f;
        p->phase = (float)i*0.83f;
        p->shirt = shirts[i%7];
        p->trousers = pants[i%4];
        p->skin = (Color){
            (unsigned char)(183 + (i%4)*11),
            (unsigned char)(132 + (i%3)*12),
            (unsigned char)(101 + (i%4)*9), 255
        };

        if (p->isStatic) {
            int guest = i - 108;
            float a = (float)guest/20.0f*2.0f*PI_F;
            float radius = (guest%2 == 0) ? 91.0f : 99.0f;
            p->position = (Vector3){SAND_CENTER_X + cosf(a)*radius, 0.0f,
                                     SAND_CENTER_Z + sinf(a)*radius};
            p->yaw = -a + PI_F*0.5f;
            p->speed = 0.0f;
        } else if (p->alongX) {
            int road = (i/2)%47 - 23;
            float start = -CITY_HALF + 30.0f + (float)((i*41)%((int)(CITY_HALF*2.0f - 60.0f)));
            p->position = (Vector3){ start, 0.0f,
                                     (float)road*48.0f + ((i%4 < 2) ? -10.2f : 10.2f) };
            p->yaw = (p->direction > 0) ? -PI_F*0.5f : PI_F*0.5f;
        } else {
            int road = (i/2)%47 - 23;
            float start = -CITY_HALF + 30.0f + (float)((i*37)%((int)(CITY_HALF*2.0f - 60.0f)));
            p->position = (Vector3){ (float)road*48.0f + ((i%4 < 2) ? -10.2f : 10.2f),
                                     0.0f, start };
            p->yaw = (p->direction > 0) ? PI_F : 0.0f;
        }
    }
}

static void UpdatePedestrians(float dt) {
    for (int i = 0; i < WALKER_COUNT; i++) {
        Pedestrian *p = &gPedestrians[i];
        float movement = p->speed*(float)p->direction*dt;
        if (!p->isStatic) {
            if (p->alongX) p->position.x += movement;
            else p->position.z += movement;
        }
        p->phase += dt*7.0f;
        if (p->isStatic) continue;

        float value = p->alongX ? p->position.x : p->position.z;
        if (value > p->routeMax) {
            if (p->alongX) p->position.x = p->routeMin;
            else p->position.z = p->routeMin;
        } else if (value < p->routeMin) {
            if (p->alongX) p->position.x = p->routeMax;
            else p->position.z = p->routeMax;
        }
    }
}

static void DrawRainAround(Vector3 focus, float time, bool raining) {
    if (!raining) return;
    for (int i = 0; i < 72; i++) {
        float x = focus.x + (float)((i*37)%74) - 37.0f;
        float z = focus.z + (float)((i*53)%74) - 37.0f;
        float y = 5.0f + (float)((i*19)%17) + fmodf(time*29.0f + (float)i*1.7f, 8.0f);
        DrawLine3D((Vector3){x, y, z}, (Vector3){x - 0.30f, y - 1.25f, z + 0.10f},
                   Fade((Color){190, 212, 225, 255}, 0.48f));
    }
}

static const char *GetCarName(int model) {
    static const char *names[12] = {
        "CITY SEDAN", "SPORT COUPE", "OFFROAD SUV", "PICKUP",
        "POLICE UNIT", "CARGO VAN", "CLASSIC GT", "CITY TAXI",
        "RALLY HATCH", "MUSCLE GT", "ELECTRIC LUX", "DESERT BUGGY"
    };
    return names[model%12];
}

/* ---- Missions ---------------------------------------------------------- */
static const Vector3 gMissionPoints[MISSION_COUNT] = {
    {   45.0f, 0.0f,   11.0f }, {   98.0f, 0.0f,  -48.0f },
    {  -44.0f, 0.0f,   60.0f }, {  -96.0f, 0.0f,  -52.0f },
    {  145.0f, 0.0f, -105.0f }, { -145.0f, 0.0f,  144.0f },
    {  285.0f, 0.0f,  -90.0f }, { -286.0f, 0.0f, -150.0f },
    {  250.0f, 0.0f,  240.0f }, { -240.0f, 0.0f,  285.0f },
    {  565.0f, 0.0f,  110.0f }, {  690.0f, 0.0f,  230.0f }
};

static void DrawMissionMarker(Vector3 p, float time, bool pickup) {
    float pulse = 1.0f + 0.16f*sinf(time*3.0f);
    Color c = pickup ? C_MINT : C_GOLD;
    DrawCylinder((Vector3){p.x, 0.13f, p.z}, 1.35f*pulse, 1.35f*pulse,
                 0.18f, 24, Fade(c, 0.55f));
    DrawCylinderWires((Vector3){p.x, 0.15f, p.z}, 1.5f*pulse, 1.5f*pulse,
                      0.22f, 24, c);
    DrawCylinder((Vector3){p.x, 1.35f, p.z}, 0.12f, 0.12f, 2.2f,
                 12, Fade(c, 0.36f));
    DrawSphere((Vector3){p.x, 2.65f + 0.15f*sinf(time*2.0f), p.z},
               0.45f*pulse, c);
}

/* ---- Minimap and HUD --------------------------------------------------- */
static void DrawMinimap(Rectangle map, Vector3 actor, Vector3 target,
                        bool hasTarget, bool inCar) {
    DrawPanel(map, (Color){ 11, 19, 28, 226 }, (Color){ 153, 174, 188, 220 });
    Rectangle inner = {map.x + 9.0f, map.y + 9.0f, map.width - 18.0f, map.height - 18.0f};
    DrawRectangle((int)inner.x, (int)inner.y, (int)inner.width, (int)inner.height,
                  (Color){ 47, 78, 69, 255 });

    float sx = inner.width/(WORLD_HALF*2.0f);
    float sz = inner.height/(WORLD_HALF*2.0f);
    Rectangle desert = {
        inner.x + (SAND_X_MIN + WORLD_HALF)*sx,
        inner.y + (SAND_Z_MIN + WORLD_HALF)*sz,
        (SAND_X_MAX - SAND_X_MIN)*sx,
        (SAND_Z_MAX - SAND_Z_MIN)*sz
    };
    DrawRectangle((int)desert.x, (int)desert.y, (int)desert.width,
                  (int)desert.height, (Color){196, 157, 101, 255});

    for (int i = -4; i <= 4; i++) {
        float x = inner.x + (float)(i + 4)*(inner.width/8.0f);
        float y = inner.y + (float)(i + 4)*(inner.height/8.0f);
        DrawRectangle((int)(x - 1.0f), (int)inner.y, 2, (int)inner.height,
                      (Color){ 85, 91, 96, 255 });
        DrawRectangle((int)inner.x, (int)(y - 1.0f), (int)inner.width, 2,
                      (Color){ 85, 91, 96, 255 });
    }

    float px = inner.x + (actor.x + WORLD_HALF)*sx;
    float py = inner.y + (actor.z + WORLD_HALF)*sz;
    if (hasTarget) {
        float tx = inner.x + (target.x + WORLD_HALF)*sx;
        float ty = inner.y + (target.z + WORLD_HALF)*sz;
        DrawCircle((int)tx, (int)ty, 3.8f, C_GOLD);
    }
    DrawCircle((int)px, (int)py, inCar ? 4.6f : 3.5f, C_MINT);
    DrawCircleLines((int)px, (int)py, 6.0f, C_WHITE);
}

static void DrawGameHUD(int screenW, int screenH, int cash,
                        int delivered, int missionStep, float speed,
                        bool inCar, bool nearCar, bool nearMission,
                        Vector3 actor, Vector3 target, bool hasTarget,
                        MoveInput stick, float stickX, float stickY,
                        float time, bool radioEnabled, int radioTrack, int carModel,
                        float driftScore, int bestDrift, float nightAmount, bool raining,
                        int weaponIndex, int ammo, int reserveAmmo, int points) {
    DrawPanel((Rectangle){14.0f, 14.0f, 312.0f, 218.0f}, C_HUD, (Color){111, 142, 160, 220});
    DrawArabicLabel(AR_CITY, (Rectangle){22.0f, 18.0f, 150.0f, 25.0f}, C_MINT);
    DrawArabicLabel(GetArabicDistrictLabel(actor), (Rectangle){22.0f, 48.0f, 128.0f, 22.0f}, Fade(C_WHITE, 0.92f));
    DrawArabicLabel(AR_CASH, (Rectangle){22.0f, 77.0f, 54.0f, 20.0f}, C_GOLD);
    DrawText(TextFormat("$%d", cash), 84, 79, 17, C_GOLD);
    DrawArabicLabel(AR_MISSIONS, (Rectangle){22.0f, 105.0f, 54.0f, 20.0f}, C_WHITE);
    DrawText(TextFormat("%d / 6", delivered), 84, 107, 16, C_WHITE);
    DrawArabicLabel(AR_POINTS, (Rectangle){174.0f, 77.0f, 55.0f, 20.0f}, C_MINT);
    DrawText(TextFormat("%d", points), 232, 79, 16, C_MINT);
    DrawArabicLabel(AR_LEVEL, (Rectangle){174.0f, 105.0f, 55.0f, 20.0f}, C_WHITE);
    DrawText(TextFormat("%d", points/500 + 1), 232, 107, 16, C_WHITE);
    if (!inCar) {
        DrawArabicLabel((weaponIndex == 0) ? AR_PULSE : ((weaponIndex == 1) ? AR_RIFLE : AR_SHOCK),
                        (Rectangle){22.0f, 135.0f, 103.0f, 24.0f}, C_GOLD);
        DrawArabicLabel(AR_AMMO, (Rectangle){132.0f, 135.0f, 40.0f, 24.0f}, C_WHITE);
        DrawText(TextFormat("%d/%d", ammo, reserveAmmo), 176, 140, 13, C_WHITE);
        Rectangle reloadRect = {222.0f, 132.0f, 92.0f, 31.0f};
        DrawArabicButton(reloadRect, AR_RELOAD, (Color){64, 99, 135, 255}, false);
    }
    DrawArabicLabel(AR_HEALTH, (Rectangle){22.0f, 166.0f, 56.0f, 18.0f}, C_RED);
    DrawRectangleRounded((Rectangle){82.0f, 168.0f, 92.0f, 12.0f}, 0.3f, 6, (Color){53, 57, 66, 255});
    DrawRectangleRounded((Rectangle){82.0f, 168.0f, 92.0f*(float)gPlayerHealth/100.0f, 12.0f},
                         0.3f, 6, (Color){218, 72, 73, 255});
    DrawText(TextFormat("%d", gPlayerHealth), 181, 165, 14, C_WHITE);
    DrawArabicLabel(AR_HUNGER, (Rectangle){22.0f, 189.0f, 56.0f, 18.0f}, C_GOLD);
    DrawRectangleRounded((Rectangle){82.0f, 191.0f, 92.0f, 12.0f}, 0.3f, 6, (Color){53, 57, 66, 255});
    DrawRectangleRounded((Rectangle){82.0f, 191.0f, 92.0f*gPlayerHunger/100.0f, 12.0f},
                         0.3f, 6, (Color){221, 171, 66, 255});
    DrawText(TextFormat("%d%%", (int)gPlayerHunger), 181, 188, 14, C_WHITE);
    DrawArabicLabel(AR_ARMOR, (Rectangle){224.0f, 185.0f, 52.0f, 19.0f}, C_BLUE);
    DrawText(TextFormat("%d", gPlayerArmor), 280, 188, 13, C_BLUE);

    Rectangle map = {(float)screenW - 160.0f, 14.0f, 146.0f, 146.0f};
    DrawMinimap(map, actor, target, hasTarget, inCar);
    DrawPanel((Rectangle){(float)screenW - 160.0f, 169.0f, 146.0f, 42.0f}, C_HUD,
              Fade(C_WHITE, 0.30f));
    DrawArabicLabel(AR_SPEED, (Rectangle){(float)screenW - 151.0f, 171.0f, 45.0f, 18.0f}, C_WHITE);
    if (inCar) {
        DrawText(TextFormat("%03d", (int)(fabsf(speed)*4.0f)), screenW - 105, 174, 13, C_WHITE);
        DrawArabicLabel(AR_KMH, (Rectangle){(float)screenW - 61.0f, 174.0f, 44.0f, 15.0f}, C_WHITE);
    } else {
        DrawArabicLabel(AR_FOOT, (Rectangle){(float)screenW - 106.0f, 173.0f, 90.0f, 18.0f}, C_WHITE);
    }
    DrawArabicLabel(AR_DRIFT, (Rectangle){(float)screenW - 151.0f, 191.0f, 42.0f, 16.0f}, C_GOLD);
    DrawText(TextFormat("%05d / %05d", (int)driftScore, bestDrift), screenW - 106, 194, 9, C_GOLD);
    DrawPanel((Rectangle){(float)screenW - 160.0f, 216.0f, 146.0f, 38.0f},
              (Color){12, 20, 31, 220}, Fade(C_MINT, 0.35f));
    DrawArabicLabel(AR_RADIO, (Rectangle){(float)screenW - 151.0f, 221.0f, 45.0f, 21.0f},
                    radioEnabled ? C_MINT : C_RED);
    DrawText((radioEnabled && GetRadioTrackCount() > 0) ? TextFormat("%02d / %02d", radioTrack + 1, GetRadioTrackCount()) : "OFF",
             screenW - 105, 228, 11, radioEnabled ? C_MINT : C_RED);
    DrawPanel((Rectangle){(float)screenW - 160.0f, 260.0f, 146.0f, 38.0f},
              (Color){12, 20, 31, 220}, Fade(C_GOLD, 0.35f));
    DrawArabicLabel(AR_GARAGE, (Rectangle){(float)screenW - 151.0f, 265.0f, 45.0f, 21.0f}, C_GOLD);
    DrawArabicLabel(AR_VEHICLE, (Rectangle){(float)screenW - 105.0f, 267.0f, 43.0f, 17.0f}, C_GOLD);
    DrawText(TextFormat("%02d", carModel + 1), screenW - 58, 271, 11, C_GOLD);
    DrawArabicLabel(AR_WEATHER, (Rectangle){(float)screenW - 151.0f, 288.0f, 42.0f, 17.0f}, C_WHITE);
    DrawArabicLabel(raining ? AR_RAIN : (nightAmount > 0.55f ? AR_NIGHT : AR_CLEAR),
                    (Rectangle){(float)screenW - 105.0f, 288.0f, 88.0f, 17.0f},
                    raining ? C_BLUE : (nightAmount > 0.55f ? C_MINT : C_WHITE));
    int gameDay = (int)(time/240.0f) + 1;
    float hourFloat = fmodf(time/10.0f + gClockHourOffset, 24.0f);
    int gameHour = (int)hourFloat;
    int gameMinute = (int)((hourFloat - (float)gameHour)*60.0f);
    DrawPanel((Rectangle){14.0f, 238.0f, 312.0f, 30.0f},
              (Color){12, 20, 31, 218}, Fade(C_GOLD, 0.38f));
    DrawArabicLabel(AR_TIME, (Rectangle){22.0f, 242.0f, 54.0f, 20.0f}, C_GOLD);
    DrawText(TextFormat("DAY %02d   %02d:%02d", gameDay, gameHour, gameMinute),
             82, 244, 14, C_WHITE);
    DrawText("IMMORTAL MODE  |  AUTO SAVE", 22, 274, 12, C_MINT);

    if (missionStep < MISSION_COUNT) {
        DrawPanel((Rectangle){(float)screenW*0.5f - 128.0f, 16.0f, 256.0f, 44.0f},
                  (Color){14, 23, 35, 210}, Fade(C_GOLD, 0.7f));
        DrawArabicLabel(((missionStep%2) == 0) ? AR_PICKUP : AR_DELIVER,
                        (Rectangle){(float)screenW*0.5f - 118.0f, 23.0f, 236.0f, 30.0f}, C_GOLD);
    } else {
        DrawPanel((Rectangle){(float)screenW*0.5f - 128.0f, 16.0f, 256.0f, 44.0f},
                  (Color){14, 23, 35, 210}, Fade(C_MINT, 0.7f));
        DrawArabicLabel(AR_ALL_DONE, (Rectangle){(float)screenW*0.5f - 118.0f, 23.0f, 236.0f, 30.0f}, C_MINT);
    }
    DrawPanel((Rectangle){(float)screenW*0.5f - 142.0f, 66.0f, 284.0f, 31.0f},
              (Color){11, 19, 30, 212}, Fade(C_RED, 0.42f));
    DrawArabicLabel(AR_CHAPTER, (Rectangle){(float)screenW*0.5f - 132.0f, 70.0f, 68.0f, 20.0f}, C_GOLD);
    DrawText(TextFormat("%d/5", gChapter), screenW/2 - 54, 72, 14, C_WHITE);
    DrawArabicLabel(AR_WAVE, (Rectangle){screenW/2 - 12.0f, 70.0f, 54.0f, 20.0f}, C_RED);
    DrawText(TextFormat("%d", gWaveNumber), screenW/2 + 49, 72, 14, C_WHITE);
    DrawArabicLabel(AR_KILLS, (Rectangle){screenW/2 + 72.0f, 70.0f, 54.0f, 20.0f}, C_MINT);
    DrawText(TextFormat("%d", gZombieKills + gGangKills), screenW/2 + 126, 72, 14, C_WHITE);

    /* Virtual analogue stick and context-sensitive controls. */
    DrawCircle((int)stickX, (int)stickY, 62.0f, Fade(C_DARK, 0.48f));
    DrawCircleLines((int)stickX, (int)stickY, 62.0f, Fade(C_WHITE, 0.85f));
    DrawCircle((int)(stickX + stick.x*34.0f), (int)(stickY - stick.y*34.0f),
               24.0f, Fade(C_WHITE, 0.88f));

    Rectangle actionRect = {(float)screenW - 106.0f, (float)screenH - 142.0f, 88.0f, 54.0f};
    Rectangle jumpRect = {(float)screenW - 206.0f, (float)screenH - 142.0f, 88.0f, 54.0f};
    Rectangle gasRect = {(float)screenW - 106.0f, (float)screenH - 286.0f, 88.0f, 68.0f};
    Rectangle brakeRect = {(float)screenW - 206.0f, (float)screenH - 286.0f, 88.0f, 68.0f};
    Rectangle sprintRect = {(float)screenW - 106.0f, (float)screenH - 214.0f, 88.0f, 54.0f};
    Rectangle driftRect = {(float)screenW - 206.0f, (float)screenH - 214.0f, 88.0f, 54.0f};
    Rectangle fireRect = {(float)screenW - 106.0f, (float)screenH - 278.0f, 88.0f, 54.0f};
    Rectangle weaponRect = {(float)screenW - 206.0f, (float)screenH - 278.0f, 88.0f, 54.0f};
    int actionLabel = inCar ? AR_EXIT : (nearCar ? AR_ENTER_CAR : AR_INTERACT);
    DrawArabicButton(actionRect, actionLabel, C_BLUE, nearCar || nearMission);
    if (inCar) {
        DrawArabicButton(jumpRect, AR_CAMERA, (Color){67, 113, 168, 255}, false);
        DrawArabicButton(gasRect, AR_GAS, (Color){34, 139, 91, 255}, false);
        DrawArabicButton(brakeRect, AR_BRAKE, (Color){163, 68, 64, 255}, false);
        DrawArabicButton(sprintRect, AR_NITRO, (Color){121, 94, 164, 255}, false);
        DrawArabicButton(driftRect, AR_HANDBRAKE, (Color){169, 104, 49, 255}, false);
    } else {
        DrawArabicButton(jumpRect, AR_JUMP, (Color){67, 113, 168, 255}, false);
        DrawArabicButton(sprintRect, AR_MOVE, (Color){121, 94, 164, 255}, false);
        DrawArabicButton(driftRect, AR_PUNCH, (Color){166, 103, 59, 255}, false);
        DrawArabicButton(fireRect, AR_FIRE, (Color){172, 63, 54, 255}, true);
        DrawArabicButton(weaponRect, AR_SWITCH_WEAPON, (Color){71, 91, 126, 255}, false);
    }
    DrawArabicLabel(AR_MOVE, (Rectangle){16.0f, (float)screenH - 26.0f, 130.0f, 20.0f}, Fade(C_WHITE, 0.86f));
    (void)time;
}
/* ---- Drift, tyre smoke and user radio ------------------------------ */
static void SpawnDriftEffects(const Vehicle *car) {
    for (int side = -1; side <= 1; side += 2) {
        float localX = (float)side*0.88f;
        float localZ = 1.48f;
        float wx = car->position.x + cosf(car->yaw)*localX + sinf(car->yaw)*localZ;
        float wz = car->position.z - sinf(car->yaw)*localX + cosf(car->yaw)*localZ;
        SmokeParticle *p = &gSmoke[gSmokeCursor++ % SMOKE_COUNT];
        bool sand = IsInSandArena(car->position);
        p->position = (Vector3){wx, sand ? 0.18f : 0.38f, wz};
        p->velocity = (Vector3){(float)side*(sand ? 0.52f : 0.35f),
                                sand ? 0.92f : 0.72f, 0.0f};
        p->life = sand ? 1.35f : 0.95f;
        p->maxLife = p->life;
        p->size = sand ? 0.46f : 0.32f;
        SkidMark *m = &gSkids[gSkidCursor++ % SKID_COUNT];
        m->a = (Vector3){wx - 0.18f, 0.12f, wz};
        m->b = (Vector3){wx + 0.18f, 0.12f, wz};
        m->life = 13.0f;
    }
}

static void UpdateDriftEffects(float dt) {
    for (int i = 0; i < SMOKE_COUNT; i++) {
        SmokeParticle *p = &gSmoke[i];
        if (p->life <= 0.0f) continue;
        p->life -= dt;
        p->position.x += p->velocity.x*dt;
        p->position.y += p->velocity.y*dt;
        p->position.z += p->velocity.z*dt;
        p->size += dt*0.28f;
    }
    for (int i = 0; i < SKID_COUNT; i++) {
        if (gSkids[i].life > 0.0f) gSkids[i].life -= dt;
    }
}

static void DrawDriftEffects(void) {
    for (int i = 0; i < SKID_COUNT; i++) {
        if (gSkids[i].life > 0.0f) {
            float alpha = ClampF(gSkids[i].life/13.0f, 0.0f, 0.45f);
            Color mark = IsInSandArena(gSkids[i].a) ? (Color){126, 91, 55, 255} :
                         (Color){22, 25, 29, 255};
            DrawLine3D(gSkids[i].a, gSkids[i].b, Fade(mark, alpha));
        }
    }
    for (int i = 0; i < SMOKE_COUNT; i++) {
        SmokeParticle *p = &gSmoke[i];
        if (p->life > 0.0f) {
            float alpha = ClampF(p->life/p->maxLife, 0.0f, 1.0f)*0.38f;
            Color puff = IsInSandArena(p->position) ? (Color){207, 171, 119, 255} :
                         (Color){191, 199, 205, 255};
            DrawSphere(p->position, p->size, Fade(puff, alpha));
        }
    }
}

static Sound CreateEngineLoop(void) {
    const unsigned int rate = 22050;
    const unsigned int frames = rate*2u;
    short *pcm = (short *)MemAlloc(frames*sizeof(short));
    if (pcm == NULL) return (Sound){0};
    for (unsigned int i = 0; i < frames; i++) {
        float t = (float)i/(float)rate;
        float envelope = 0.65f + 0.35f*sinf(2.0f*PI_F*t*3.0f);
        float engine = sinf(2.0f*PI_F*58.0f*t) + 0.35f*sinf(2.0f*PI_F*116.0f*t);
        float pulse = 0.16f*sinf(2.0f*PI_F*19.0f*t);
        pcm[i] = (short)(ClampF((engine*0.12f + pulse)*envelope, -0.7f, 0.7f)*32767.0f);
    }
    Wave wave = {frames, rate, 16, 1, pcm};
    Sound result = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return result;
}

static void LoadGameSfx(void) {
    static const char *paths[SFX_COUNT] = {
        "sfx/pistol.wav", "sfx/rifle.wav", "sfx/shotgun.wav", "sfx/reload.wav",
        "sfx/jump.wav", "sfx/footstep.wav", "sfx/melee.wav", "sfx/impact.wav"
    };
    static const float volumes[SFX_COUNT] = {0.52f, 0.42f, 0.62f, 0.42f, 0.38f, 0.24f, 0.38f, 0.48f};
    for (int i = 0; i < SFX_COUNT; i++) {
        gGameSfx[i] = LoadSound(paths[i]);
        if (IsSoundValid(gGameSfx[i])) SetSoundVolume(gGameSfx[i], volumes[i]);
    }
}

static void PlayGameSfx(GameSfx id) {
    if (!gAudioDeviceReady || id < 0 || id >= SFX_COUNT) return;
    if (IsSoundValid(gGameSfx[id])) PlaySound(gGameSfx[id]);
}

static void LoadMusicLibrary(void) {
    gMusicCount = 0;
    char *playlist = LoadFileText("music/playlist.txt");
    if (playlist == NULL) return;
    char *line = strtok(playlist, "\r\n");
    while (line != NULL && gMusicCount < MUSIC_CAPACITY) {
        if (line[0] != '#' && line[0] != '\0') {
            char *separator = strchr(line, '|');
            if (separator != NULL) {
                *separator = '\0';
                const char *fileName = line;
                const char *title = separator + 1;
                char path[192];
                snprintf(path, sizeof(path), "music/%s", fileName);
                Music track = LoadMusicStream(path);
                if (IsMusicValid(track)) {
                    gMusicTracks[gMusicCount] = track;
                    snprintf(gMusicTitles[gMusicCount], sizeof(gMusicTitles[gMusicCount]),
                             "%s", (title[0] != '\0') ? title : fileName);
                    SetMusicVolume(gMusicTracks[gMusicCount], 0.32f);
                    gMusicCount++;
                }
            }
        }
        line = strtok(NULL, "\r\n");
    }
    UnloadFileText(playlist);
}

static int GetRadioTrackCount(void) {
    return gMusicCount;
}

static void StopRadioPlayback(void) {
    if (gMusicCount > 0) {
        for (int i = 0; i < gMusicCount; i++)
            if (IsMusicStreamPlaying(gMusicTracks[i])) StopMusicStream(gMusicTracks[i]);
    }
}

static void UpdateAudio(bool inCar, float speed, bool radioEnabled) {
    if (!gAudioReady) return;
    if (gMusicCount > 0) {
        Music *track = &gMusicTracks[gRadioIndex % gMusicCount];
        if (radioEnabled) {
            UpdateMusicStream(*track);
            if (!IsMusicStreamPlaying(*track)) {
                SeekMusicStream(*track, 0.0f);
                PlayMusicStream(*track);
            }
        } else if (IsMusicStreamPlaying(*track)) {
            StopMusicStream(*track);
        }
    }
    if (IsSoundValid(gEngineSound) && inCar && fabsf(speed) > 0.8f) {
        if (!IsSoundPlaying(gEngineSound)) PlaySound(gEngineSound);
        SetSoundPitch(gEngineSound, 0.78f + ClampF(fabsf(speed)/38.0f, 0.0f, 1.0f)*0.85f);
        SetSoundVolume(gEngineSound, 0.12f);
    } else if (IsSoundValid(gEngineSound) && IsSoundPlaying(gEngineSound)) {
        StopSound(gEngineSound);
    }
}

/* ---- Arcade training range --------------------------------------------- */
static void InitTrainingRange(void) {
    for (int i = 0; i < TARGET_COUNT; i++) {
        gTargets[i].position = (Vector3){
            SAND_CENTER_X - 84.0f + (float)i*24.0f,
            1.35f,
            SAND_CENTER_Z - 218.0f
        };
        gTargets[i].active = true;
        gTargets[i].respawnTimer = 0.0f;
    }
}

static void UpdateTrainingTargets(float dt) {
    gMuzzleFlashTimer = fmaxf(0.0f, gMuzzleFlashTimer - dt);
    gShotEffect.life = fmaxf(0.0f, gShotEffect.life - dt);
    for (int i = 0; i < TARGET_COUNT; i++) {
        TrainingTarget *target = &gTargets[i];
        if (!target->active) {
            target->respawnTimer -= dt;
            if (target->respawnTimer <= 0.0f) {
                target->active = true;
                target->respawnTimer = 0.0f;
            }
        }
    }
    if (gWeaponCooldown > 0.0f) gWeaponCooldown = fmaxf(0.0f, gWeaponCooldown - dt);
    if (gMeleeCooldown > 0.0f) gMeleeCooldown = fmaxf(0.0f, gMeleeCooldown - dt);
}

static void DrawTrainingRange(Vector3 focus, float time) {
    if (Distance2D(focus.x, focus.z, SAND_CENTER_X, SAND_CENTER_Z - 182.0f) > 210.0f) return;
    DrawCube((Vector3){SAND_CENTER_X, -0.015f, SAND_CENTER_Z - 182.0f}, 220.0f, 0.12f, 94.0f,
             (Color){174, 145, 102, 255});
    DrawCube((Vector3){SAND_CENTER_X, 0.045f, SAND_CENTER_Z - 228.0f}, 220.0f, 0.10f, 2.0f,
             (Color){93, 82, 68, 255});
    for (int i = 0; i < TARGET_COUNT; i++) {
        const TrainingTarget *target = &gTargets[i];
        float x = target->position.x;
        float z = target->position.z;
        DrawCylinder((Vector3){x, 0.62f, z}, 0.18f, 0.18f, 1.24f, 8,
                     (Color){83, 87, 91, 255});
        if (target->active) {
            float pulse = 0.92f + 0.08f*sinf(time*3.0f + (float)i);
            DrawCube((Vector3){x, 1.65f*pulse, z}, 1.55f, 1.75f*pulse, 0.28f,
                     (Color){194, 72, 58, 255});
            DrawCube((Vector3){x, 1.65f*pulse, z - 0.16f}, 0.86f, 1.02f*pulse, 0.04f,
                     (Color){228, 218, 190, 255});
            DrawSphere((Vector3){x, 1.65f*pulse, z - 0.22f}, 0.22f,
                       (Color){54, 121, 163, 255});
        } else {
            DrawCube((Vector3){x, 0.16f, z}, 1.2f, 0.25f, 0.28f,
                     (Color){88, 93, 97, 255});
        }
    }
}

/* ---- Shops, interiors and outbreak factions ---------------------------- */
static Vector3 ShopPosition(InteriorType type) {
    switch (type) {
        case INTERIOR_WEAPON: return (Vector3){-24.0f, 0.0f, -24.0f};
        case INTERIOR_MARKET: return (Vector3){ 72.0f, 0.0f, -24.0f};
        case INTERIOR_DINER:  return (Vector3){-24.0f, 0.0f,  72.0f};
        case INTERIOR_HOME:   return (Vector3){ 72.0f, 0.0f,  72.0f};
        default: return (Vector3){0};
    }
}

static InteriorType FindNearestShop(Vector3 p) {
    InteriorType best = INTERIOR_NONE;
    float bestDistance = 7.0f;
    for (int type = INTERIOR_WEAPON; type <= INTERIOR_HOME; type++) {
        Vector3 shop = ShopPosition((InteriorType)type);
        float d = Distance2D(p.x, p.z, shop.x, shop.z);
        if (d < bestDistance) {
            bestDistance = d;
            best = (InteriorType)type;
        }
    }
    return best;
}

static void DrawWorldShops(Vector3 focus, float time) {
    static const Color signs[4] = {
        {205, 65, 54, 255}, {52, 151, 104, 255},
        {202, 143, 58, 255}, {58, 112, 170, 255}
    };
    for (int type = INTERIOR_WEAPON; type <= INTERIOR_HOME; type++) {
        Vector3 p = ShopPosition((InteriorType)type);
        if (Distance2D(focus.x, focus.z, p.x, p.z) > 90.0f) continue;
        DrawCube((Vector3){p.x, 2.35f, p.z}, 8.2f, 4.7f, 7.0f,
                 (Color){56, 65, 77, 255});
        DrawCube((Vector3){p.x, 4.82f, p.z}, 8.6f, 0.24f, 7.4f,
                 (Color){30, 36, 46, 255});
        DrawCube((Vector3){p.x, 2.05f, p.z - 3.56f}, 6.7f, 2.55f, 0.13f,
                 (Color){75, 135, 158, 255});
        DrawCube((Vector3){p.x, 4.0f, p.z - 3.85f}, 7.5f, 0.72f, 0.34f,
                 signs[type - 1]);
        DrawCube((Vector3){p.x - 2.9f, 0.25f, p.z - 3.9f}, 0.16f, 0.5f, 0.16f,
                 C_GOLD);
        DrawCube((Vector3){p.x + 2.9f, 0.25f, p.z - 3.9f}, 0.16f, 0.5f, 0.16f,
                 C_GOLD);
        float pulse = 1.0f + 0.13f*sinf(time*2.3f + (float)type);
        DrawSphere((Vector3){p.x, 6.1f*pulse, p.z}, 0.42f,
                   Fade(signs[type - 1], 0.82f));
    }
}

static void DrawInteriorRoom(InteriorType type, float time) {
    Color wall = (Color){49, 58, 70, 255};
    Color floor = (Color){82, 82, 78, 255};
    if (type == INTERIOR_HOME) wall = (Color){72, 66, 62, 255};
    if (type == INTERIOR_DINER) wall = (Color){79, 53, 43, 255};
    if (type == INTERIOR_MARKET) wall = (Color){51, 73, 65, 255};
    DrawCube((Vector3){0.0f, -0.18f, 0.0f}, 17.0f, 0.35f, 17.0f, floor);
    DrawCube((Vector3){0.0f, 3.0f, -8.3f}, 17.0f, 6.0f, 0.35f, wall);
    DrawCube((Vector3){-8.3f, 3.0f, 0.0f}, 0.35f, 6.0f, 17.0f, wall);
    DrawCube((Vector3){8.3f, 3.0f, 0.0f}, 0.35f, 6.0f, 17.0f, wall);
    DrawCube((Vector3){0.0f, 5.9f, 0.0f}, 17.0f, 0.25f, 17.0f,
             (Color){28, 33, 42, 255});
    DrawCube((Vector3){0.0f, 5.4f, 0.0f}, 0.22f, 0.10f, 0.22f,
             (Color){255, 220, 153, 255});
    DrawSphere((Vector3){0.0f, 5.1f, 0.0f}, 0.55f,
               Fade((Color){255, 215, 142, 255}, 0.20f));
    if (type == INTERIOR_WEAPON) {
        for (int i = 0; i < 3; i++) {
            float x = -4.8f + (float)i*4.8f;
            DrawCube((Vector3){x, 1.35f, -5.8f}, 3.5f, 2.6f, 0.8f,
                     (Color){83, 93, 105, 255});
            DrawCube((Vector3){x, 2.9f, -5.8f}, 3.7f, 0.16f, 1.0f, C_GOLD);
            DrawCube((Vector3){x, 2.15f, -5.25f}, 1.0f, 0.16f, 0.18f, C_DARK);
        }
        DrawCube((Vector3){0.0f, 1.0f, -2.0f}, 8.0f, 1.8f, 1.2f,
                 (Color){95, 69, 48, 255});
        DrawCube((Vector3){0.0f, 2.0f, -2.0f}, 8.2f, 0.15f, 1.35f, C_GOLD);
    } else if (type == INTERIOR_MARKET) {
        for (int row = 0; row < 2; row++) {
            for (int i = 0; i < 4; i++) {
                float x = -5.5f + (float)i*3.6f;
                DrawCube((Vector3){x, 1.5f, -4.2f + (float)row*7.0f},
                         1.5f, 3.0f, 1.0f, (Color){99, 113, 91, 255});
                for (int shelf = 0; shelf < 3; shelf++)
                    DrawCube((Vector3){x, 0.65f + (float)shelf*0.85f,
                                       -3.58f + (float)row*7.0f},
                             1.1f, 0.34f, 0.18f, (Color){191, 137, 67, 255});
            }
        }
        DrawCube((Vector3){0.0f, 1.1f, 6.4f}, 10.0f, 2.0f, 1.0f,
                 (Color){45, 93, 71, 255});
    } else if (type == INTERIOR_DINER) {
        DrawCube((Vector3){0.0f, 1.25f, -5.8f}, 10.0f, 2.4f, 1.25f,
                 (Color){107, 63, 47, 255});
        for (int i = 0; i < 3; i++) {
            float x = -4.0f + (float)i*4.0f;
            DrawCylinder((Vector3){x, 0.58f, 0.8f}, 0.13f, 0.13f, 1.1f, 8, C_DARK);
            DrawCube((Vector3){x, 1.2f, 0.8f}, 2.4f, 0.18f, 1.7f,
                     (Color){132, 88, 55, 255});
        }
    } else if (type == INTERIOR_HOME) {
        /* Bedroom, bed, bedside table and living-room sofa. */
        DrawCube((Vector3){-3.0f, 0.65f, -3.8f}, 4.8f, 0.65f, 6.0f,
                 (Color){76, 56, 51, 255});
        DrawCube((Vector3){-3.0f, 1.05f, -3.8f}, 4.6f, 0.25f, 5.8f,
                 (Color){209, 209, 199, 255});
        DrawCube((Vector3){-3.0f, 1.42f, -6.0f}, 4.5f, 0.65f, 1.2f,
                 (Color){70, 102, 134, 255});
        DrawCube((Vector3){3.5f, 0.95f, 4.6f}, 5.0f, 1.4f, 1.4f,
                 (Color){86, 104, 120, 255});
        DrawCube((Vector3){5.6f, 1.45f, 2.7f}, 1.2f, 2.6f, 1.2f,
                 (Color){94, 69, 49, 255});
        DrawSphere((Vector3){5.6f, 2.95f + 0.05f*sinf(time), 2.7f},
                   0.17f, C_GOLD);
    }
    /* Front opening/doorway. */
    DrawCube((Vector3){0.0f, 1.45f, 8.25f}, 3.1f, 2.9f, 0.3f,
             (Color){28, 35, 44, 255});
}

static void GetInteriorButtons(int screenW, int screenH,
                               Rectangle *one, Rectangle *two,
                               Rectangle *three, Rectangle *exitButton) {
    float cx = (float)screenW*0.5f;
    float y = (float)screenH*0.42f;
    *one = (Rectangle){cx - 156.0f, y, 312.0f, 54.0f};
    *two = (Rectangle){cx - 156.0f, y + 66.0f, 312.0f, 54.0f};
    *three = (Rectangle){cx - 156.0f, y + 132.0f, 312.0f, 54.0f};
    *exitButton = (Rectangle){cx - 108.0f, (float)screenH - 76.0f, 216.0f, 52.0f};
}

static void DrawInteriorHUD(int screenW, int screenH, int cash) {
    Rectangle one, two, three, exitButton;
    GetInteriorButtons(screenW, screenH, &one, &two, &three, &exitButton);
    DrawRectangle(0, 0, screenW, screenH, Fade((Color){4, 8, 15, 255}, 0.32f));
    DrawPanel((Rectangle){(float)screenW*0.5f - 190.0f, 14.0f, 380.0f, 100.0f},
              (Color){9, 17, 28, 239}, Fade(C_GOLD, 0.7f));
    int title = AR_HOME;
    if (gInteriorType == INTERIOR_WEAPON) title = AR_GUNSHOP;
    else if (gInteriorType == INTERIOR_MARKET) title = AR_MARKET;
    else if (gInteriorType == INTERIOR_DINER) title = AR_DINER;
    DrawArabicLabel(title, (Rectangle){(float)screenW*0.5f - 145.0f, 24.0f, 290.0f, 36.0f}, C_GOLD);
    DrawText(TextFormat("$ %d", cash), screenW/2 - 60, 68, 22, C_MINT);
    if (gInteriorType == INTERIOR_WEAPON) {
        DrawArabicButton(one, AR_BUY_RIFLE, (Color){54, 92, 137, 255}, false);
        DrawText("$2,200", screenW/2 - 145, (int)one.y + 18, 16, C_GOLD);
        DrawArabicButton(two, AR_BUY_SHOTGUN, (Color){54, 92, 137, 255}, false);
        DrawText("$3,000", screenW/2 - 145, (int)two.y + 18, 16, C_GOLD);
        DrawArabicButton(three, AR_BUY_AMMO, (Color){54, 92, 137, 255}, false);
        DrawText("$150", screenW/2 - 145, (int)three.y + 18, 16, C_GOLD);
    } else if (gInteriorType == INTERIOR_MARKET) {
        DrawArabicButton(one, AR_BUY_FOOD, (Color){48, 124, 87, 255}, false);
        DrawText("$40", screenW/2 - 145, (int)one.y + 18, 16, C_GOLD);
        DrawArabicButton(two, AR_HEALTH, (Color){48, 124, 87, 255}, false);
        DrawText("$125", screenW/2 - 145, (int)two.y + 18, 16, C_GOLD);
        DrawArabicButton(three, AR_BUY_AMMO, (Color){48, 124, 87, 255}, false);
        DrawText("$150", screenW/2 - 145, (int)three.y + 18, 16, C_GOLD);
    } else if (gInteriorType == INTERIOR_DINER) {
        DrawArabicButton(one, AR_EAT_MEAL, (Color){151, 88, 50, 255}, false);
        DrawText("$75", screenW/2 - 145, (int)one.y + 18, 16, C_GOLD);
        DrawArabicButton(two, AR_BUY_FOOD, (Color){151, 88, 50, 255}, false);
        DrawText("$40", screenW/2 - 145, (int)two.y + 18, 16, C_GOLD);
        DrawArabicButton(three, AR_HEALTH, (Color){151, 88, 50, 255}, false);
        DrawText("$125", screenW/2 - 145, (int)three.y + 18, 16, C_GOLD);
    } else {
        DrawArabicButton(one, AR_SLEEP, (Color){66, 92, 135, 255}, true);
        DrawArabicButton(two, AR_EAT_MEAL, (Color){66, 92, 135, 255}, false);
        DrawText("$75", screenW/2 + 92, (int)two.y + 16, 16, C_GOLD);
        DrawArabicButton(three, AR_HEALTH, (Color){66, 92, 135, 255}, false);
        DrawText("$125", screenW/2 + 92, (int)three.y + 16, 16, C_GOLD);
    }
    DrawArabicButton(exitButton, AR_EXIT_STORE, (Color){143, 62, 57, 255}, false);
}

static void SpawnHostileWave(Vector3 around, int wave) {
    int total = 12 + wave*2;
    if (total > 28) total = 28;
    gHostileActive = 0;
    for (int i = 0; i < HOSTILE_CAPACITY; i++) gHostiles[i].active = false;
    for (int i = 0; i < total; i++) {
        float angle = (float)i*2.39996323f + (float)wave*0.41f;
        float radius = 42.0f + (float)(i%4)*9.0f;
        Vector3 p = {around.x + cosf(angle)*radius, 0.0f,
                     around.z + sinf(angle)*radius};
        for (int attempt = 0; attempt < 12 && HitsBuilding(p.x, p.z, 1.1f); attempt++) {
            angle += 0.53f;
            radius += 2.0f;
            p.x = around.x + cosf(angle)*radius;
            p.z = around.z + sinf(angle)*radius;
        }
        if (HitsBuilding(p.x, p.z, 1.1f)) {
            float roadX = roundf(p.x/CITY_BLOCK)*CITY_BLOCK;
            float roadZ = roundf(p.z/CITY_BLOCK)*CITY_BLOCK;
            p.x = roadX;
            p.z = roadZ + ((i%2) ? 8.0f : -8.0f);
        }
        Hostile *h = &gHostiles[i];
        h->position = p;
        h->yaw = angle + PI_F;
        h->phase = (float)i*0.8f;
        h->type = ((i%4) == 0) ? HOSTILE_GANG : HOSTILE_ZOMBIE;
        h->faction = (i%3);
        h->health = (h->type == HOSTILE_ZOMBIE ? 62.0f : 96.0f) + (float)(wave-1)*4.0f;
        h->moveSpeed = (h->type == HOSTILE_ZOMBIE ? 1.45f : 1.9f) + (float)(wave-1)*0.035f;
        h->attackCooldown = 0.7f + (float)(i%4)*0.24f;
        h->active = true;
        gHostileActive++;
    }
}

static void InitHostiles(Vector3 around) {
    gWaveNumber = 1;
    gChapter = 1;
    gZombieKills = 0;
    gGangKills = 0;
    SpawnHostileWave(around, gWaveNumber);
}

static void DrawHostile(const Hostile *h) {
    Color shirt = (h->type == HOSTILE_ZOMBIE) ?
        (Color){74, 91, 67, 255} :
        ((h->faction == 0) ? (Color){140, 52, 54, 255} :
         ((h->faction == 1) ? (Color){70, 67, 132, 255} : (Color){61, 105, 111, 255}));
    Color skin = (h->type == HOSTILE_ZOMBIE) ? (Color){95, 126, 84, 255} :
                 (Color){174, 131, 105, 255};
    Color pants = (h->type == HOSTILE_ZOMBIE) ? (Color){55, 61, 50, 255} :
                  (Color){42, 47, 59, 255};
    rlPushMatrix();
    rlTranslatef(h->position.x, h->position.y, h->position.z);
    rlRotatef(h->yaw*RAD_TO_DEG, 0.0f, 1.0f, 0.0f);
    DrawHuman((Vector3){0.0f, 0.0f, 0.0f}, 0.0f, h->phase,
              shirt, pants, skin, false, true);
    if (h->type == HOSTILE_ZOMBIE) {
        DrawSphere((Vector3){-0.105f, 1.90f, -0.285f}, 0.045f, C_RED);
        DrawSphere((Vector3){ 0.105f, 1.90f, -0.285f}, 0.045f, C_RED);
    }
    rlPopMatrix();
}

static void UpdateHostiles(float dt, Vector3 *player, bool inCar,
                           int *health, int *cash, int *points,
                           char *message, size_t messageSize) {
    int active = 0;
    for (int i = 0; i < HOSTILE_CAPACITY; i++) {
        Hostile *h = &gHostiles[i];
        if (!h->active) continue;
        h->phase += dt*(h->type == HOSTILE_ZOMBIE ? 6.0f : 7.5f);
        h->attackCooldown -= dt;

        /* Rival gangs and zombies can attack each other, not only the player. */
        int rivalIndex = -1;
        float rivalDistance = 9.0f;
        for (int j = 0; j < HOSTILE_CAPACITY; j++) {
            if (j == i || !gHostiles[j].active || gHostiles[j].type == h->type) continue;
            float rx = gHostiles[j].position.x - h->position.x;
            float rz = gHostiles[j].position.z - h->position.z;
            float rd = sqrtf(rx*rx + rz*rz);
            if (rd < rivalDistance) {
                rivalDistance = rd;
                rivalIndex = j;
            }
        }
        bool engaged = (rivalIndex >= 0);
        if (engaged) {
            Hostile *rival = &gHostiles[rivalIndex];
            float rx = rival->position.x - h->position.x;
            float rz = rival->position.z - h->position.z;
            h->yaw = atan2f(-rx, -rz);
            if (rivalDistance > 1.8f) {
                float step = h->moveSpeed*dt;
                MoveWithCollision(&h->position, -sinf(h->yaw)*step,
                                  -cosf(h->yaw)*step, 0.58f);
            } else if (h->attackCooldown <= 0.0f) {
                rival->health -= (h->type == HOSTILE_ZOMBIE) ? 12.0f : 15.0f;
                h->attackCooldown = 1.25f;
                if (rival->health <= 0.0f) rival->active = false;
            }
        }

        float dx = player->x - h->position.x;
        float dz = player->z - h->position.z;
        float dist = sqrtf(dx*dx + dz*dz);
        if (!engaged && dist < 66.0f) {
            h->yaw = atan2f(-dx, -dz);
            if (dist > (h->type == HOSTILE_ZOMBIE ? 1.45f : 2.4f)) {
                float step = h->moveSpeed*dt;
                Vector3 old = h->position;
                MoveWithCollision(&h->position, -sinf(h->yaw)*step,
                                  -cosf(h->yaw)*step, 0.58f);
                if (Distance2D(old.x, old.z, h->position.x, h->position.z) < 0.002f)
                    h->yaw += 0.9f*dt;
            }
            bool meleeAttack = !inCar &&
                dist < (h->type == HOSTILE_ZOMBIE ? 1.9f : 2.3f);
            bool gangShot = h->type == HOSTILE_GANG && dist >= 4.5f && dist < 24.0f;
            if ((meleeAttack || gangShot) && h->attackCooldown <= 0.0f) {
                if (!gPlayerInvulnerable) {
                    int damage = gangShot ? 6 : ((h->type == HOSTILE_ZOMBIE) ? 7 : 5);
                    if (gPlayerArmor > 0) {
                        int absorbed = (damage < gPlayerArmor) ? damage : gPlayerArmor;
                        gPlayerArmor -= absorbed;
                        damage -= absorbed;
                    }
                    *health -= damage;
                }
                h->attackCooldown = gangShot ? 2.1f :
                    ((h->type == HOSTILE_ZOMBIE) ? 1.0f : 1.35f);
                snprintf(message, messageSize,
                         gPlayerInvulnerable ? "IMMORTAL MODE - NO DAMAGE" :
                         (gangShot ? "RIVAL GANG MEMBER FIRED" : "THE OUTBREAK IS ATTACKING"));
            }
        }
    }
    active = 0;
    for (int i = 0; i < HOSTILE_CAPACITY; i++) if (gHostiles[i].active) active++;
    gHostileActive = active;
    if (*health <= 0 && !gPlayerInvulnerable) {
        *health = 60;
        gPlayerHunger = fmaxf(20.0f, gPlayerHunger - 20.0f);
        *cash = (*cash > 250) ? *cash - 250 : 0;
        *player = (Vector3){0.0f, 0.0f, 12.0f};
        snprintf(message, messageSize, "YOU WERE RESCUED - RETURN HOME");
    } else if (gPlayerInvulnerable && *health < 100) {
        *health = 100;
    }
    if (gHostileActive == 0) {
        gWaveNumber++;
        gChapter = ((gWaveNumber - 1)/2) + 1;
        if (gChapter > 5) gChapter = 5;
        *cash += 300 + gWaveNumber*40;
        *points += 200 + gWaveNumber*25;
        SpawnHostileWave(*player, gWaveNumber);
        snprintf(message, messageSize, "NEW OUTBREAK WAVE - SURVIVE");
    }
}

static bool PurchaseItem(int item, int *cash, int *health,
                         char *message, size_t messageSize) {
    int price = 0;
    switch (item) {
        case 1: price = 2200; if (gWeaponOwned[1]) { snprintf(message,messageSize,"WEAPON ALREADY OWNED"); return false; } break;
        case 2: price = 3000; if (gWeaponOwned[2]) { snprintf(message,messageSize,"WEAPON ALREADY OWNED"); return false; } break;
        case 3: price = 150; break;
        case 4: price = 40; break;
        case 5: price = 75; break;
        case 6: price = 125; break;
        default: return false;
    }
    if (*cash < price) {
        snprintf(message, messageSize, "NOT ENOUGH CASH");
        return false;
    }
    *cash -= price;
    if (item == 1) {
        gWeaponOwned[1] = true;
        gWeaponAmmo[1] += 60;
        gWeaponReserve[1] += 180;
    } else if (item == 2) {
        gWeaponOwned[2] = true;
        gWeaponAmmo[2] += 6;
        gWeaponReserve[2] += 30;
    } else if (item == 3) {
        gWeaponReserve[gWeaponIndex] += (gWeaponIndex == 0 ? 45 : (gWeaponIndex == 1 ? 90 : 8));
    } else if (item == 4) {
        gPlayerHunger = ClampF(gPlayerHunger + 38.0f, 0.0f, 100.0f);
    } else if (item == 5) {
        gPlayerHunger = 100.0f;
        *health = (*health < 95) ? *health + 5 : 100;
    } else if (item == 6) {
        *health = (*health < 70) ? *health + 30 : 100;
    }
    snprintf(message, messageSize, "PURCHASE COMPLETE");
    return true;
}

static bool ReloadCurrentWeapon(char *message, size_t messageSize) {
    static const int clipSizes[3] = {45, 120, 12};
    if (gWeaponIndex < 0 || gWeaponIndex >= 3) gWeaponIndex = 0;
    int clipSize = clipSizes[gWeaponIndex];
    int missing = clipSize - gWeaponAmmo[gWeaponIndex];

    if (missing <= 0) {
        snprintf(message, messageSize, "MAGAZINE FULL");
        return false;
    }
    if (gWeaponReserve[gWeaponIndex] <= 0) {
        snprintf(message, messageSize, "NO AMMO IN RESERVE");
        return false;
    }

    int amount = (gWeaponReserve[gWeaponIndex] < missing) ?
                 gWeaponReserve[gWeaponIndex] : missing;
    gWeaponAmmo[gWeaponIndex] += amount;
    gWeaponReserve[gWeaponIndex] -= amount;
    PlayGameSfx(SFX_RELOAD);
    snprintf(message, messageSize, "WEAPON RELOADED");
    return true;
}

static void PunchNearbyHostile(Vector3 player, float yaw, int *cash, int *points,
                               char *message, size_t messageSize) {
    if (gMeleeCooldown > 0.0f) return;
    float dirX = -sinf(yaw);
    float dirZ = -cosf(yaw);
    int bestIndex = -1;
    float bestDistance = 2.7f;
    for (int i = 0; i < HOSTILE_CAPACITY; i++) {
        Hostile *h = &gHostiles[i];
        if (!h->active) continue;
        float dx = h->position.x - player.x;
        float dz = h->position.z - player.z;
        float distance = sqrtf(dx*dx + dz*dz);
        if (distance < 0.75f || distance > bestDistance) continue;
        float dot = (dx*dirX + dz*dirZ)/distance;
        if (dot > 0.05f) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    gMeleeCooldown = 0.55f;
    PlayGameSfx(SFX_MELEE);
    if (bestIndex < 0) {
        snprintf(message, messageSize, "NO ENEMY IN PUNCH RANGE");
        return;
    }
    Hostile *h = &gHostiles[bestIndex];
    h->health -= 28.0f;
    h->position.x -= dirX*0.45f;
    h->position.z -= dirZ*0.45f;
    if (h->health <= 0.0f) {
        h->active = false;
        gHostileActive--;
        if (h->type == HOSTILE_ZOMBIE) gZombieKills++;
        else gGangKills++;
        *cash += (h->type == HOSTILE_ZOMBIE) ? 70 : 90;
        *points += 55;
        snprintf(message, messageSize,
                 (h->type == HOSTILE_ZOMBIE) ? "ZOMBIE DEFEATED +$70" : "GANG MEMBER DEFEATED +$90");
    } else {
        snprintf(message, messageSize, "MELEE HIT - ENEMY HURT");
    }
}

static void FireTrainingWeapon(Vector3 player, float yaw, int *cash, int *points,
                               char *message, size_t messageSize) {
    static const float rangeByWeapon[3] = {42.0f, 64.0f, 34.0f};
    static const float coneByWeapon[3] = {0.86f, 0.90f, 0.68f};
    static const int rewardPoints[3] = {80, 100, 150};
    static const int rewardCash[3] = {20, 30, 45};
    static const float damageByWeapon[3] = {42.0f, 36.0f, 78.0f};
    if (gWeaponAmmo[gWeaponIndex] <= 0) {
        snprintf(message, messageSize, "NO AMMO - PRESS R TO RELOAD");
        return;
    }
    gWeaponAmmo[gWeaponIndex]--;
    PlayGameSfx((GameSfx)(SFX_PISTOL + gWeaponIndex));
    gMuzzleFlashTimer = 0.105f;
    float dirX = -sinf(yaw);
    float dirZ = -cosf(yaw);
    gShotEffect.start = (Vector3){player.x + dirX*0.72f, player.y + 1.18f, player.z + dirZ*0.72f};
    gShotEffect.end = (Vector3){gShotEffect.start.x + dirX*rangeByWeapon[gWeaponIndex],
                                gShotEffect.start.y, gShotEffect.start.z + dirZ*rangeByWeapon[gWeaponIndex]};
    gShotEffect.life = 0.11f;
    int hostileIndex = -1;
    float bestHostileDistance = rangeByWeapon[gWeaponIndex] + 1.0f;
    for (int i = 0; i < HOSTILE_CAPACITY; i++) {
        Hostile *h = &gHostiles[i];
        if (!h->active) continue;
        float dx = h->position.x - player.x;
        float dz = h->position.z - player.z;
        float distance = sqrtf(dx*dx + dz*dz);
        if (distance < 2.0f || distance > rangeByWeapon[gWeaponIndex]) continue;
        float dot = (dx*dirX + dz*dirZ)/distance;
        if (dot >= coneByWeapon[gWeaponIndex] && distance < bestHostileDistance &&
            HasClearShot(player, h->position)) {
            bestHostileDistance = distance;
            hostileIndex = i;
        }
    }
    if (hostileIndex >= 0) {
        Hostile *h = &gHostiles[hostileIndex];
        gShotEffect.end = (Vector3){h->position.x, h->position.y + 1.15f, h->position.z};
        PlayGameSfx(SFX_IMPACT);
        h->health -= damageByWeapon[gWeaponIndex];
        if (h->health <= 0.0f) {
            h->active = false;
            gHostileActive--;
            if (h->type == HOSTILE_ZOMBIE) gZombieKills++;
            else gGangKills++;
            *cash += (h->type == HOSTILE_ZOMBIE) ? 100 : 140;
            *points += rewardPoints[gWeaponIndex];
            snprintf(message, messageSize,
                     (h->type == HOSTILE_ZOMBIE) ? "ZOMBIE DEFEATED +$100" : "GANG MEMBER DEFEATED +$140");
        } else {
            snprintf(message, messageSize, "ENEMY HIT - KEEP FIRING");
        }
        return;
    }
    int bestIndex = -1;
    float bestDistance = rangeByWeapon[gWeaponIndex] + 1.0f;
    for (int i = 0; i < TARGET_COUNT; i++) {
        const TrainingTarget *target = &gTargets[i];
        if (!target->active) continue;
        float dx = target->position.x - player.x;
        float dz = target->position.z - player.z;
        float distance = sqrtf(dx*dx + dz*dz);
        if (distance < 2.0f || distance > rangeByWeapon[gWeaponIndex]) continue;
        float dot = (dx*dirX + dz*dirZ)/distance;
        if (dot >= coneByWeapon[gWeaponIndex] && distance < bestDistance &&
            HasClearShot(player, target->position)) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    if (bestIndex >= 0) {
        gShotEffect.end = gTargets[bestIndex].position;
        PlayGameSfx(SFX_IMPACT);
        gTargets[bestIndex].active = false;
        gTargets[bestIndex].respawnTimer = 4.0f;
        *points += rewardPoints[gWeaponIndex];
        *cash += rewardCash[gWeaponIndex];
        snprintf(message, messageSize, "TARGET HIT  +%d PTS", rewardPoints[gWeaponIndex]);
    } else {
        snprintf(message, messageSize, "NO TARGET IN FRONT");
    }
}

/* ---- Persistent player profile (raylib app storage) ------------------- */
static int FloatToStorage(float value) {
    union { float f; int32_t i; } bits;
    bits.f = value;
    return (int)bits.i;
}

static float StorageToFloat(int value) {
    union { float f; int32_t i; } bits;
    bits.i = (int32_t)value;
    return bits.f;
}

static void SaveGameProgress(int cash, int points, int missionStep, int delivered,
                             Vector3 player, float playerYaw, const Vehicle *car,
                             float worldTime) {
    /*
     * SaveStorageValue/LoadStorageValue are helper functions from a raylib
     * example, not part of the raylib 5.5 public API. Store the complete
     * profile in one small binary file instead. On Android raylib routes
     * SaveFileData/LoadFileData to the app's private writable data directory.
     */
    int32_t slots[SAVE_DATA_SLOT_COUNT] = { 0 };
    int ownedMask = (gWeaponOwned[0] ? 1 : 0) | (gWeaponOwned[1] ? 2 : 0) |
                    (gWeaponOwned[2] ? 4 : 0);

    slots[0]  = (int32_t)SAVE_MAGIC;
    slots[1]  = (int32_t)cash;
    slots[2]  = (int32_t)points;
    slots[3]  = (int32_t)missionStep;
    slots[4]  = (int32_t)delivered;
    slots[5]  = (int32_t)FloatToStorage(player.x);
    slots[6]  = (int32_t)FloatToStorage(player.z);
    slots[7]  = (int32_t)FloatToStorage(playerYaw);
    slots[8]  = (int32_t)gWeaponIndex;
    slots[9]  = (int32_t)gWeaponAmmo[0];
    slots[10] = (int32_t)gWeaponReserve[0];
    slots[11] = (int32_t)gWeaponAmmo[1];
    slots[12] = (int32_t)gWeaponReserve[1];
    slots[13] = (int32_t)gWeaponAmmo[2];
    slots[14] = (int32_t)gWeaponReserve[2];
    slots[15] = (int32_t)ownedMask;
    slots[16] = (int32_t)FloatToStorage(car->position.x);
    slots[17] = (int32_t)FloatToStorage(car->position.z);
    slots[18] = (int32_t)FloatToStorage(car->yaw);
    slots[19] = (int32_t)car->model;
    slots[20] = (int32_t)gBestDrift;
    slots[21] = (int32_t)SAVE_SCHEMA_VERSION;
    slots[22] = (int32_t)gPlayerHealth;
    slots[23] = (int32_t)FloatToStorage(gPlayerHunger);
    slots[24] = (int32_t)gPlayerArmor;
    slots[25] = (int32_t)FloatToStorage(worldTime);
    slots[26] = (int32_t)FloatToStorage(gClockHourOffset);
    slots[27] = (int32_t)gWaveNumber;
    slots[28] = (int32_t)gChapter;
    slots[29] = (int32_t)gZombieKills;
    slots[30] = (int32_t)gGangKills;
    slots[31] = (int32_t)gRadioIndex;
    slots[32] = (int32_t)(gRadioEnabled ? 1 : 0);

    if (!SaveFileData(SAVE_FILE_NAME, slots, (int)sizeof(slots))) {
        /* Saving is best-effort; keep the active game running if storage is unavailable. */
        fprintf(stderr, "CITY LIFE 3D: could not save profile to %s\n", SAVE_FILE_NAME);
    }
}

static bool LoadGameProgress(int *cash, int *points, int *missionStep, int *delivered,
                             Vector3 *player, float *playerYaw, Vehicle *car,
                             float *worldTime) {
    int fileSize = 0;
    unsigned char *fileData = LoadFileData(SAVE_FILE_NAME, &fileSize);
    if (fileData == NULL) return false;

    const int minLegacySize = 21 * (int)sizeof(int32_t);
    if (fileSize < minLegacySize) {
        UnloadFileData(fileData);
        return false;
    }

    int32_t slots[SAVE_DATA_SLOT_COUNT] = { 0 };
    int availableSlots = fileSize / (int)sizeof(int32_t);
    if (availableSlots > SAVE_DATA_SLOT_COUNT) availableSlots = SAVE_DATA_SLOT_COUNT;
    memcpy(slots, fileData, (size_t)availableSlots * sizeof(int32_t));
    UnloadFileData(fileData);

    if (slots[0] != (int32_t)SAVE_MAGIC) return false;

    *cash = (int)ClampF((float)slots[1], 0.0f, 99999999.0f);
    *points = (int)slots[2];
    *missionStep = (int)ClampF((float)slots[3], 0.0f, (float)MISSION_COUNT);
    *delivered = (int)ClampF((float)slots[4], 0.0f, 6.0f);
    player->x = StorageToFloat((int)slots[5]);
    player->z = StorageToFloat((int)slots[6]);
    *playerYaw = StorageToFloat((int)slots[7]);
    if (!isfinite(*playerYaw)) *playerYaw = 0.0f;
    else *playerYaw = NormalizeAngle(*playerYaw);
    gWeaponIndex = (int)ClampF((float)slots[8], 0.0f, 2.0f);
    gWeaponAmmo[0] = (int)ClampF((float)slots[9], 0.0f, 500.0f);
    gWeaponReserve[0] = (int)ClampF((float)slots[10], 0.0f, 3000.0f);
    gWeaponAmmo[1] = (int)ClampF((float)slots[11], 0.0f, 500.0f);
    gWeaponReserve[1] = (int)ClampF((float)slots[12], 0.0f, 3000.0f);
    gWeaponAmmo[2] = (int)ClampF((float)slots[13], 0.0f, 500.0f);
    gWeaponReserve[2] = (int)ClampF((float)slots[14], 0.0f, 3000.0f);
    int mask = (int)slots[15];
    for (int i = 0; i < 3; i++) gWeaponOwned[i] = (mask & (1 << i)) != 0;
    gWeaponOwned[0] = true; /* the starter pistol is always available */
    if (!gWeaponOwned[gWeaponIndex]) gWeaponIndex = 0;
    car->position.x = StorageToFloat((int)slots[16]);
    car->position.z = StorageToFloat((int)slots[17]);
    car->yaw = StorageToFloat((int)slots[18]);
    if (!isfinite(car->yaw)) car->yaw = 0.0f;
    else car->yaw = NormalizeAngle(car->yaw);
    car->model = (int)ClampF((float)slots[19], 0.0f, 11.0f);
    gBestDrift = (int)ClampF((float)slots[20], 0.0f, 999999.0f);

    /* Fields 21..32 were added in save schema 2; older profiles keep defaults. */
    if (availableSlots >= SAVE_DATA_SLOT_COUNT && slots[21] == SAVE_SCHEMA_VERSION) {
        gPlayerHealth = (int)ClampF((float)slots[22], 1.0f, 100.0f);
        gPlayerHunger = StorageToFloat((int)slots[23]);
        if (!isfinite(gPlayerHunger)) gPlayerHunger = 100.0f;
        gPlayerHunger = ClampF(gPlayerHunger, 0.0f, 100.0f);
        gPlayerArmor = (int)ClampF((float)slots[24], 0.0f, 100.0f);
        *worldTime = StorageToFloat((int)slots[25]);
        if (!isfinite(*worldTime) || *worldTime < 0.0f || *worldTime > 100000000.0f)
            *worldTime = 0.0f;
        gClockHourOffset = StorageToFloat((int)slots[26]);
        if (!isfinite(gClockHourOffset)) gClockHourOffset = 8.0f;
        gClockHourOffset = fmodf(gClockHourOffset, 24.0f);
        if (gClockHourOffset < 0.0f) gClockHourOffset += 24.0f;
        gWaveNumber = (int)ClampF((float)slots[27], 1.0f, 10000.0f);
        gChapter = (int)ClampF((float)slots[28], 1.0f, 5.0f);
        gZombieKills = (int)ClampF((float)slots[29], 0.0f, 999999.0f);
        gGangKills = (int)ClampF((float)slots[30], 0.0f, 999999.0f);
        gRadioIndex = (int)ClampF((float)slots[31], 0.0f, (float)(MUSIC_CAPACITY - 1));
        gRadioEnabled = (slots[32] != 0) && (gMusicCount > 0);
    }
    if (!isfinite(player->x) || !isfinite(player->z) || fabsf(player->x) > WORLD_HALF || fabsf(player->z) > WORLD_HALF) {
        player->x = 0.0f; player->z = 12.0f;
    }
    if (!isfinite(car->position.x) || !isfinite(car->position.z) || fabsf(car->position.x) > WORLD_HALF || fabsf(car->position.z) > WORLD_HALF) {
        car->position.x = 0.0f; car->position.z = 4.0f;
    }
    player->y = 0.0f;
    car->position.y = 0.0f;
    car->speed = 0.0f;
    return true;
}

/* ---- Main game --------------------------------------------------------- */
int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "CITY LIFE 3D - ULTIMATE WORLD V7.2.1");
#if !defined(PLATFORM_ANDROID)
    /* Desktop builds run from the project root; Android uses APK assets as cwd. */
    ChangeDirectory("assets");
#endif
    SetTargetFPS(60);

    InitAudioDevice();
    gArabicAtlas = LoadTexture("ui/arabic-ui.png");
    gAudioDeviceReady = IsAudioDeviceReady();
    gAudioReady = gAudioDeviceReady;
    if (gAudioDeviceReady) {
        gEngineSound = CreateEngineLoop();
        LoadGameSfx();
        LoadMusicLibrary();
        gAudioReady = true;
        if (IsSoundValid(gEngineSound)) SetSoundVolume(gEngineSound, 0.12f);
    }
    gRadioEnabled = (gMusicCount > 0);

    BuildCity();
    InitTrainingRange();
    InitPedestrians();
    InitTraffic();
    InitWorldTrees();

    GameState state = GAME_TITLE;
    float worldTime = 0.0f;
    float walkPhase = 0.0f;
    float jumpVelocity = 0.0f;
    bool grounded = true;
    bool inCar = false;
    int cameraMode = 0;
    int cash = 1800;
    int missionStep = 0;
    int delivered = 0;
    float messageTimer = 0.0f;
    float driftComboTimer = 0.0f;
    bool raining = false;
    float weatherTimer = 0.0f;
    char message[96] = "WELCOME TO NOVA BAY MEGA WORLD";

    Vector3 player = { 0.0f, 0.0f, 12.0f };
    InitHostiles(player);
    gPlayerHealth = 100;
    gPlayerHunger = 100.0f;
    gPlayerArmor = 0;
    float playerYaw = 0.0f;
    Vehicle playerCar = {
        { 0.0f, 0.0f, 4.0f }, 0.0f, 0.0f, 0.0f,
        { 206, 58, 67, 255 }, 0
    };

    Camera3D camera = {0};
    camera.position = (Vector3){ -8.0f, 7.0f, 20.0f };
    camera.target = (Vector3){ 0.0f, 1.2f, 12.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 58.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool actionWasDown = false;
    bool jumpWasDown = false;
    bool pauseWasDown = false;
    bool startWasDown = false;
    bool resumeWasDown = false;
    bool restartWasDown = false;
    bool radioWasDown = false;
    bool garageWasDown = false;
    bool muteWasDown = false;
    bool reloadWasDown = false;
    bool weaponWasDown = false;
    bool punchWasDown = false;
    int points = 0;
    static const Color playerPaints[12] = {
        {206, 58, 67, 255}, {46, 106, 186, 255}, {71, 134, 102, 255},
        {198, 151, 65, 255}, {220, 224, 229, 255}, {80, 94, 115, 255},
        {65, 58, 52, 255}, {224, 192, 54, 255}, {231, 123, 52, 255},
        {39, 42, 51, 255}, {102, 187, 191, 255}, {170, 178, 187, 255}
    };
    bool loadedProfile = LoadGameProgress(&cash, &points, &missionStep, &delivered,
                                          &player, &playerYaw, &playerCar, &worldTime);
    if (loadedProfile) {
        playerCar.paint = playerPaints[playerCar.model];
        /* Immortality is enabled by default: do not restore stale low health
           from an older profile and briefly show a nearly defeated player. */
        if (gPlayerInvulnerable) gPlayerHealth = 100;
        SpawnHostileWave(player, gWaveNumber);
        if (gMusicCount > 0) gRadioIndex %= gMusicCount;
        else { gRadioIndex = 0; gRadioEnabled = false; }
        snprintf(message, sizeof(message), "PROFILE LOADED - WELCOME BACK");
        messageTimer = 2.4f;
    }
    float autosaveTimer = 0.0f;

    while (!WindowShouldClose()) {
        float dt = ClampF(GetFrameTime(), 0.0f, 0.045f);
        /* Freeze world clock and weather while the title or pause screen is shown. */
        if (state == GAME_PLAYING) {
            worldTime += dt;
            weatherTimer += dt;
            if (weatherTimer > 95.0f) { raining = !raining; weatherTimer = 0.0f; }
        }
        float dayPhase = fmodf(worldTime, 240.0f)/240.0f;
        float daylight = 0.5f + 0.5f*cosf(dayPhase*2.0f*PI_F);
        gNightAmount = 1.0f - daylight;
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        Rectangle actionRect = {(float)screenW - 106.0f, (float)screenH - 142.0f, 88.0f, 54.0f};
        Rectangle jumpRect = {(float)screenW - 206.0f, (float)screenH - 142.0f, 88.0f, 54.0f};
        Rectangle gasRect = {(float)screenW - 106.0f, (float)screenH - 286.0f, 88.0f, 68.0f};
        Rectangle brakeRect = {(float)screenW - 206.0f, (float)screenH - 286.0f, 88.0f, 68.0f};
        Rectangle sprintRect = {(float)screenW - 106.0f, (float)screenH - 214.0f, 88.0f, 54.0f};
        Rectangle pauseRect = {(float)screenW - 54.0f, 304.0f, 40.0f, 38.0f};
        Rectangle radioRect = {(float)screenW - 160.0f, 216.0f, 146.0f, 38.0f};
        Rectangle garageRect = {(float)screenW - 160.0f, 260.0f, 146.0f, 38.0f};
        Rectangle muteRect = {(float)screenW - 108.0f, 304.0f, 48.0f, 38.0f};
        Rectangle driftRect = {(float)screenW - 206.0f, (float)screenH - 214.0f, 88.0f, 54.0f};
        Rectangle fireRect = {(float)screenW - 106.0f, (float)screenH - 278.0f, 88.0f, 54.0f};
        Rectangle weaponRect = {(float)screenW - 206.0f, (float)screenH - 278.0f, 88.0f, 54.0f};
        Rectangle reloadRect = {222.0f, 132.0f, 92.0f, 31.0f};
        Rectangle interiorOne, interiorTwo, interiorThree, interiorExit;
        GetInteriorButtons(screenW, screenH, &interiorOne, &interiorTwo,
                           &interiorThree, &interiorExit);
        Rectangle playRect = {(float)screenW*0.5f - 128.0f, (float)screenH*0.62f, 256.0f, 66.0f};
        Rectangle resumeRect = {(float)screenW*0.5f - 115.0f, (float)screenH*0.53f, 230.0f, 58.0f};
        Rectangle restartRect = {(float)screenW*0.5f - 115.0f, (float)screenH*0.63f, 230.0f, 58.0f};

        if (state == GAME_TITLE) {
            bool start = TouchPressed(playRect, &startWasDown) || IsKeyPressed(KEY_ENTER);
            if (start) {
                state = GAME_PLAYING;
                messageTimer = 2.6f;
                snprintf(message, sizeof(message), "YOUR FIRST DELIVERY IS READY");
            }
        } else if (state == GAME_PAUSED) {
            bool resume = TouchPressed(resumeRect, &resumeWasDown) || IsKeyPressed(KEY_ENTER);
            bool restart = TouchPressed(restartRect, &restartWasDown);
            if (resume) state = GAME_PLAYING;
            if (restart) {
                player = (Vector3){0.0f, 0.0f, 12.0f};
                playerYaw = 0.0f;
                playerCar.position = (Vector3){0.0f, 0.0f, 4.0f};
                playerCar.speed = 0.0f;
                playerCar.steering = 0.0f;
                playerCar.yaw = 0.0f;
                playerCar.model = 0;
                playerCar.paint = playerPaints[0];
                inCar = false;
                cameraMode = 0;
                /* A restart must always return to the outdoor spawn point. */
                gIndoors = false;
                gInteriorType = INTERIOR_NONE;
                gExteriorPosition = (Vector3){0.0f, 0.0f, 0.0f};
                gInteriorExitWasDown = false;
                gInteriorBuy1WasDown = false;
                gInteriorBuy2WasDown = false;
                gInteriorBuy3WasDown = false;
                StopRadioPlayback();
                gRadioIndex = 0;
                worldTime = 0.0f;
                weatherTimer = 0.0f;
                raining = false;
                gClockHourOffset = 8.0f;
                gNightAmount = 0.0f;
                gHungerDamageTimer = 0.0f;
                gDriftScore = 0.0f;
                gBestDrift = 0;
                driftComboTimer = 0.0f;
                gSmokeCursor = 0;
                gSkidCursor = 0;
                gSmokeSpawnTimer = 0.0f;
                memset(gSmoke, 0, sizeof(gSmoke));
                memset(gSkids, 0, sizeof(gSkids));
                messageTimer = 0.0f;
                walkPhase = 0.0f;
                jumpVelocity = 0.0f;
                grounded = true;
                pauseWasDown = false;
                resumeWasDown = false;
                restartWasDown = false;
                radioWasDown = false;
                garageWasDown = false;
                muteWasDown = false;
                actionWasDown = false;
                jumpWasDown = false;
                reloadWasDown = false;
                weaponWasDown = false;
                punchWasDown = false;
                cash = 1800;
                points = 0;
                gWeaponAmmo[0] = 45; gWeaponAmmo[1] = 0; gWeaponAmmo[2] = 0;
                gWeaponReserve[0] = 135; gWeaponReserve[1] = 0; gWeaponReserve[2] = 0;
                gWeaponIndex = 0;
                gWeaponOwned[0] = true; gWeaponOwned[1] = false; gWeaponOwned[2] = false;
                gWeaponCooldown = 0.0f; gMeleeCooldown = 0.0f;
                gPlayerHealth = 100; gPlayerHunger = 100.0f; gPlayerArmor = 0;
                InitTrainingRange();
                InitHostiles(player);
                missionStep = 0;
                delivered = 0;
                state = GAME_PLAYING;
            }
        } else {
            bool pauseDown = TouchHeld(pauseRect) || IsKeyDown(KEY_ESCAPE);
            bool pausePressed = pauseDown && !pauseWasDown;
            pauseWasDown = pauseDown;
            if (pausePressed) state = GAME_PAUSED;
            if (TouchPressed(radioRect, &radioWasDown)) {
                StopRadioPlayback();
                if (GetRadioTrackCount() > 0) gRadioIndex = (gRadioIndex + 1)%GetRadioTrackCount();
                gRadioEnabled = (gMusicCount > 0);
                snprintf(message, sizeof(message), "RADIO: USER TRACK CHANGED");
                messageTimer = 2.0f;
            }
            if (TouchPressed(muteRect, &muteWasDown) || IsKeyPressed(KEY_M))
                gRadioEnabled = !gRadioEnabled;
            if (!gIndoors && (TouchPressed(garageRect, &garageWasDown) || IsKeyPressed(KEY_C))) {
                playerCar.model = (playerCar.model + 1)%12;
                playerCar.paint = playerPaints[playerCar.model];
                snprintf(message, sizeof(message), "GARAGE: %s", GetCarName(playerCar.model));
                messageTimer = 2.0f;
            }

            if (gIndoors) {
                if (TouchPressed(interiorExit, &gInteriorExitWasDown)) {
                    gIndoors = false;
                    gInteriorType = INTERIOR_NONE;
                    player = gExteriorPosition;
                    player.y = 0.0f;
                    playerYaw = 0.0f;
                } else if (TouchPressed(interiorOne, &gInteriorBuy1WasDown)) {
                    if (gInteriorType == INTERIOR_WEAPON) PurchaseItem(1, &cash, &gPlayerHealth, message, sizeof(message));
                    else if (gInteriorType == INTERIOR_MARKET) PurchaseItem(4, &cash, &gPlayerHealth, message, sizeof(message));
                    else if (gInteriorType == INTERIOR_DINER) PurchaseItem(5, &cash, &gPlayerHealth, message, sizeof(message));
                    else {
                        float hour = fmodf(worldTime/10.0f + gClockHourOffset, 24.0f);
                        float untilMorning = fmodf(7.0f - hour + 24.0f, 24.0f);
                        if (untilMorning < 1.0f) untilMorning += 24.0f;
                        worldTime += untilMorning*10.0f;
                        gPlayerHealth = 100;
                        gPlayerHunger = fmaxf(35.0f, gPlayerHunger - 16.0f);
                        gPlayerArmor = 25;
                        snprintf(message, sizeof(message), "RESTED UNTIL MORNING");
                    }
                    messageTimer = 2.0f;
                } else if (TouchPressed(interiorTwo, &gInteriorBuy2WasDown)) {
                    if (gInteriorType == INTERIOR_WEAPON) PurchaseItem(2, &cash, &gPlayerHealth, message, sizeof(message));
                    else if (gInteriorType == INTERIOR_MARKET) PurchaseItem(6, &cash, &gPlayerHealth, message, sizeof(message));
                    else if (gInteriorType == INTERIOR_DINER) PurchaseItem(4, &cash, &gPlayerHealth, message, sizeof(message));
                    else PurchaseItem(5, &cash, &gPlayerHealth, message, sizeof(message));
                    messageTimer = 2.0f;
                } else if (TouchPressed(interiorThree, &gInteriorBuy3WasDown)) {
                    if (gInteriorType == INTERIOR_WEAPON || gInteriorType == INTERIOR_MARKET)
                        PurchaseItem(3, &cash, &gPlayerHealth, message, sizeof(message));
                    else PurchaseItem(6, &cash, &gPlayerHealth, message, sizeof(message));
                    messageTimer = 2.0f;
                }
            }
        }

        Vector3 actor = gIndoors ? (Vector3){0.0f, 0.0f, 0.0f} :
                        (inCar ? playerCar.position : player);
        Vector3 missionTarget = (missionStep < MISSION_COUNT) ? gMissionPoints[missionStep] : actor;
        bool hasTarget = missionStep < MISSION_COUNT;
        bool nearCar = !gIndoors && !inCar && Distance2D(player.x, player.z,
                                             playerCar.position.x, playerCar.position.z) < 5.0f;
        InteriorType nearestShop = gIndoors ? INTERIOR_NONE : FindNearestShop(actor);
        bool nearShop = nearestShop != INTERIOR_NONE && !inCar;
        bool nearMission = !gIndoors && hasTarget &&
            Distance2D(actor.x, actor.z, missionTarget.x, missionTarget.z) < 5.2f;

        MoveInput stick = {0.0f, 0.0f};
        float stickX = (float)screenW*0.165f;
        float stickY = (float)screenH*0.79f;
        bool gasDown = false;
        bool brakeDown = false;
        bool sprintDown = false;
        bool nitroDown = false;
        bool handbrakeDown = false;
        bool jumpPressed = false;
        bool actionPressed = false;

        if (state == GAME_PLAYING) {
            /* Keyboard support for desktop testing. */
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) stick.x -= 1.0f;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) stick.x += 1.0f;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) stick.y += 1.0f;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) stick.y -= 1.0f;

            /* Touch joystick is on the lower left; other buttons are right side. */
            for (int t = 0; t < GetTouchPointCount(); t++) {
                Vector2 p = GetTouchPosition(t);
                if (p.x < (float)screenW*0.46f && p.y > (float)screenH*0.40f) {
                    float dx = p.x - stickX;
                    float dy = p.y - stickY;
                    float len = sqrtf(dx*dx + dy*dy);
                    if (len > 8.0f) {
                        float scale = (len > 62.0f) ? 62.0f/len : 1.0f;
                        stick.x = ClampF(dx*scale/62.0f, -1.0f, 1.0f);
                        stick.y = ClampF(-dy*scale/62.0f, -1.0f, 1.0f);
                    }
                }
            }
            gasDown = TouchHeld(gasRect) || IsKeyDown(KEY_LEFT_SHIFT);
            brakeDown = TouchHeld(brakeRect) || IsKeyDown(KEY_SPACE);
            sprintDown = !inCar && (TouchHeld(sprintRect) || IsKeyDown(KEY_LEFT_CONTROL));
            nitroDown = inCar && (TouchHeld(sprintRect) || IsKeyDown(KEY_N));
            handbrakeDown = inCar && (TouchHeld(driftRect) || IsKeyDown(KEY_LEFT_ALT));
            jumpPressed = TouchPressed(jumpRect, &jumpWasDown) || IsKeyPressed(KEY_SPACE);
            actionPressed = TouchPressed(actionRect, &actionWasDown) || IsKeyPressed(KEY_E);
            bool fireDown = !inCar && !gIndoors && (TouchHeld(fireRect) || IsKeyDown(KEY_F));
            bool reloadTouchPressed = TouchPressed(reloadRect, &reloadWasDown);
            bool reloadPressed = !inCar && !gIndoors && (reloadTouchPressed || IsKeyPressed(KEY_R));
            bool weaponPressed = !inCar && !gIndoors && (TouchPressed(weaponRect, &weaponWasDown) || IsKeyPressed(KEY_Q));
            bool punchPressed = !inCar && !gIndoors && (TouchPressed(driftRect, &punchWasDown) || IsKeyPressed(KEY_G));
            if (weaponPressed) {
                int candidate = gWeaponIndex;
                for (int tries = 0; tries < 3; tries++) {
                    candidate = (candidate + 1)%3;
                    if (gWeaponOwned[candidate]) { gWeaponIndex = candidate; break; }
                }
                snprintf(message, sizeof(message), "WEAPON: %s", (gWeaponIndex == 0) ? "PISTOL" : ((gWeaponIndex == 1) ? "RIFLE" : "SHOTGUN"));
                messageTimer = 1.5f;
            }
            if (punchPressed) {
                PunchNearbyHostile(player, playerYaw, &cash, &points, message, sizeof(message));
                messageTimer = 1.2f;
            }
            /* Reload is an independent tap action, not gated by the fire cooldown. */
            if (reloadPressed) {
                ReloadCurrentWeapon(message, sizeof(message));
                messageTimer = 1.2f;
            } else if (fireDown && gWeaponCooldown <= 0.0f) {
                FireTrainingWeapon(player, playerYaw, &cash, &points, message, sizeof(message));
                messageTimer = 1.1f;
                gWeaponCooldown = (gWeaponIndex == 1) ? 0.12f :
                                  ((gWeaponIndex == 2) ? 0.68f : 0.28f);
            }

            stick.x = ClampF(stick.x, -1.0f, 1.0f);
            stick.y = ClampF(stick.y, -1.0f, 1.0f);

            if (gIndoors) {
                /* Interior controls are handled by the contextual shop menu above. */
            } else if (!inCar) {
                if (actionPressed && nearCar) {
                    inCar = true;
                    cameraMode = 0;
                    playerCar.position = player;
                    playerCar.position.y = 0.0f;
                    playerCar.yaw = playerYaw;
                    snprintf(message, sizeof(message), "VEHICLE READY - DRIVE SAFE");
                    messageTimer = 2.0f;
                } else if (actionPressed && nearShop) {
                    gExteriorPosition = player;
                    gInteriorType = nearestShop;
                    gIndoors = true;
                    player = (Vector3){0.0f, 0.0f, 2.8f};
                    playerYaw = 0.0f;
                    snprintf(message, sizeof(message), "ENTERED STORE");
                    messageTimer = 1.5f;
                } else if (actionPressed && nearMission && hasTarget) {
                    if ((missionStep%2) == 0) {
                        missionStep++;
                        snprintf(message, sizeof(message), "PACKAGE COLLECTED - DELIVER IT");
                    } else {
                        missionStep++;
                        delivered++;
                        cash += 650;
                        points += 250;
                        snprintf(message, sizeof(message), "DELIVERY COMPLETE  +$650  +250 PTS");
                    }
                    messageTimer = 2.6f;
                    if (missionStep >= MISSION_COUNT) {
                        snprintf(message, sizeof(message), "NOVA BAY THANKS YOU - ALL JOBS DONE");
                        messageTimer = 4.0f;
                    }
                }

                if (!gIndoors) {
                    playerYaw = NormalizeAngle(playerYaw - stick.x*2.3f*dt);
                    float speed = sprintDown ? 7.3f : 4.6f;
                    float move = stick.y*speed;
                    float dx = -sinf(playerYaw)*move*dt;
                    float dz = -cosf(playerYaw)*move*dt;
                    MoveWithCollision(&player, dx, dz, PLAYER_RADIUS);
                    if (fabsf(stick.y) > 0.06f) walkPhase += dt*(sprintDown ? 13.0f : 9.0f);

                    if (jumpPressed && grounded) {
                        jumpVelocity = 6.7f;
                        grounded = false;
                        PlayGameSfx(SFX_JUMP);
                    }
                    if (fabsf(stick.y) > 0.14f && grounded) {
                        gFootstepTimer += dt;
                        float stepInterval = sprintDown ? 0.25f : 0.38f;
                        if (gFootstepTimer >= stepInterval) {
                            PlayGameSfx(SFX_FOOTSTEP);
                            gFootstepTimer = 0.0f;
                        }
                    } else {
                        gFootstepTimer = 0.0f;
                    }
                    jumpVelocity -= 18.0f*dt;
                    player.y += jumpVelocity*dt;
                    if (player.y <= 0.0f) {
                        player.y = 0.0f;
                        jumpVelocity = 0.0f;
                        grounded = true;
                    }
                }
            } else {
                if (jumpPressed) {
                    cameraMode = (cameraMode + 1)%3;
                    snprintf(message, sizeof(message), "CAMERA MODE %d / 3", cameraMode + 1);
                    messageTimer = 1.2f;
                }
                if (actionPressed) {
                    inCar = false;
                    playerCar.speed = 0.0f;
                    player = playerCar.position;
                    player.x += cosf(playerCar.yaw)*3.2f;
                    player.z -= sinf(playerCar.yaw)*3.2f;
                    player.y = 0.0f;
                    playerYaw = playerCar.yaw;
                    if (HitsBuilding(player.x, player.z, PLAYER_RADIUS)) {
                        player.x = playerCar.position.x - cosf(playerCar.yaw)*3.2f;
                        player.z = playerCar.position.z + sinf(playerCar.yaw)*3.2f;
                    }
                    snprintf(message, sizeof(message), "YOU ARE ON FOOT");
                    messageTimer = 1.5f;
                } else {
                    float throttle = stick.y;
                    if (gasDown) throttle = 1.0f;
                    if (brakeDown) throttle = -0.50f;

                    playerCar.speed += throttle*(nitroDown ? 23.0f : 14.5f)*dt;
                    if (!gasDown && !brakeDown && fabsf(stick.y) < 0.05f) {
                        playerCar.speed *= (1.0f - ClampF((handbrakeDown ? 0.48f : 0.86f)*dt, 0.0f, 0.9f));
                    }
                    if (brakeDown) {
                        if (playerCar.speed > 0.0f) playerCar.speed = fmaxf(0.0f, playerCar.speed - 26.0f*dt);
                        else playerCar.speed = fminf(0.0f, playerCar.speed + 26.0f*dt);
                    }
                    bool onSand = IsInSandArena(playerCar.position);
                    playerCar.speed = ClampF(playerCar.speed, -9.0f,
                                             nitroDown ? 54.0f : (onSand ? 34.0f : 39.0f));
                    float steerRate = handbrakeDown ? (onSand ? 2.85f : 2.45f) :
                                      (onSand ? 1.38f : 1.68f);
                    float turn = stick.x*steerRate*ClampF(fabsf(playerCar.speed)/4.0f, 0.18f, 1.15f)*dt;
                    playerCar.yaw = NormalizeAngle(playerCar.yaw - turn*(playerCar.speed < 0.0f ? -1.0f : 1.0f));
                    bool driftingNow = handbrakeDown && fabsf(playerCar.speed) > 7.0f && fabsf(stick.x) > 0.12f;
                    if (driftingNow) {
                        gDriftScore += fabsf(playerCar.speed)*fabsf(stick.x)*dt*(onSand ? 7.2f : 4.3f);
                        driftComboTimer = 1.25f;
                        gSmokeSpawnTimer += dt;
                        if (gSmokeSpawnTimer > 0.065f) { SpawnDriftEffects(&playerCar); gSmokeSpawnTimer = 0.0f; }
                    } else {
                        if (driftComboTimer > 0.0f) driftComboTimer -= dt;
                        else if (gDriftScore > 0.0f) {
                            if ((int)gDriftScore > gBestDrift) gBestDrift = (int)gDriftScore;
                            cash += (int)(gDriftScore/35.0f);
                            points += (int)(gDriftScore/8.0f);
                            gDriftScore = 0.0f;
                        }
                    }

                    float dx = -sinf(playerCar.yaw)*playerCar.speed*dt;
                    float dz = -cosf(playerCar.yaw)*playerCar.speed*dt;
                    float oldX = playerCar.position.x;
                    float oldZ = playerCar.position.z;
                    MoveWithCollision(&playerCar.position, dx, dz, CAR_RADIUS);
                    if (fabsf(playerCar.position.x - (oldX + dx)) > 0.01f ||
                        fabsf(playerCar.position.z - (oldZ + dz)) > 0.01f) {
                        playerCar.speed *= -0.18f;
                        PlayGameSfx(SFX_IMPACT);
                        snprintf(message, sizeof(message), "BE CAREFUL - COLLISION");
                        messageTimer = 1.0f;
                    }
                }
            }

            UpdateTraffic(dt);
            UpdatePedestrians(dt);
            UpdateTrainingTargets(dt);
            if (!gIndoors) {
                gPlayerHunger = fmaxf(0.0f, gPlayerHunger - dt*0.035f);
                if (gPlayerHunger <= 0.0f && !gPlayerInvulnerable) {
                    gHungerDamageTimer += dt;
                    if (gHungerDamageTimer >= 2.5f) {
                        gPlayerHealth = (gPlayerHealth > 1) ? gPlayerHealth - 1 : 1;
                        gHungerDamageTimer = 0.0f;
                    }
                } else {
                    gHungerDamageTimer = 0.0f;
                }
                Vector3 *hostileTarget = inCar ? &playerCar.position : &player;
                UpdateHostiles(dt, hostileTarget, inCar, &gPlayerHealth, &cash,
                               &points, message, sizeof(message));
            }
            UpdateDriftEffects(dt);
            autosaveTimer += dt;
            if (autosaveTimer >= 8.0f) {
                Vector3 persistPlayer = gIndoors ? gExteriorPosition : player;
                float persistYaw = playerYaw;
                if (inCar) {
                    persistPlayer = playerCar.position;
                    persistPlayer.x += cosf(playerCar.yaw)*3.2f;
                    persistPlayer.z -= sinf(playerCar.yaw)*3.2f;
                    persistYaw = playerCar.yaw;
                }
                SaveGameProgress(cash, points, missionStep, delivered, persistPlayer, persistYaw, &playerCar, worldTime);
                autosaveTimer = 0.0f;
            }
            if (messageTimer > 0.0f) messageTimer -= dt;
        }
        UpdateAudio(inCar, playerCar.speed, gRadioEnabled);

        /* Camera follows smoothly behind the player or vehicle. */
        actor = gIndoors ? (Vector3){0.0f, 0.0f, 0.0f} :
                (inCar ? playerCar.position : player);
        float actorYaw = gIndoors ? 0.0f : (inCar ? playerCar.yaw : playerYaw);
        Vector3 desiredTarget = { actor.x, actor.y + 1.35f, actor.z };
        float cameraBack = gIndoors ? 10.0f :
                           (inCar ? ((cameraMode == 1) ? 17.0f :
                                    ((cameraMode == 2) ? 6.2f : 10.8f)) : 10.6f);
        if (inCar && cameraMode == 0) cameraBack += ClampF(fabsf(playerCar.speed)*0.075f, 0.0f, 3.4f);
        Vector3 desiredCamera = gIndoors ?
            (Vector3){3.8f, 4.4f, 10.0f} :
            (Vector3){
                actor.x + sinf(actorYaw)*cameraBack,
                actor.y + (inCar ? ((cameraMode == 1) ? 8.2f : ((cameraMode == 2) ? 3.4f : 6.1f)) : 6.7f),
                actor.z + cosf(actorYaw)*cameraBack
            };
        if (state == GAME_TITLE) {
            desiredTarget = (Vector3){ 0.0f, 8.0f, 8.0f };
            desiredCamera = (Vector3){ 8.0f, 46.0f, 70.0f };
        } else if (!gIndoors) {
            desiredCamera = ResolveCameraCollision(desiredTarget, desiredCamera);
        }
        camera.target.x = LerpF(camera.target.x, desiredTarget.x, 1.0f - expf(-5.2f*dt));
        camera.target.y = LerpF(camera.target.y, desiredTarget.y, 1.0f - expf(-5.2f*dt));
        camera.target.z = LerpF(camera.target.z, desiredTarget.z, 1.0f - expf(-5.2f*dt));
        camera.position.x = LerpF(camera.position.x, desiredCamera.x, 1.0f - expf(-3.5f*dt));
        camera.position.y = LerpF(camera.position.y, desiredCamera.y, 1.0f - expf(-3.5f*dt));
        camera.position.z = LerpF(camera.position.z, desiredCamera.z, 1.0f - expf(-3.5f*dt));

        /* ---- Render 3D world ------------------------------------------- */
        BeginDrawing();
        Color sky = LerpColor((Color){9, 16, 34, 255}, C_SKY, daylight);
        ClearBackground(sky);
        BeginMode3D(camera);

        if (gIndoors) {
            DrawInteriorRoom(gInteriorType, worldTime);
            DrawHuman(player, playerYaw, walkPhase,
                      (Color){32, 111, 190, 255}, (Color){42, 53, 74, 255},
                      (Color){196, 151, 117, 255}, true, false);
            DrawPlayerWeapon(player, playerYaw, gWeaponIndex, 0.0f);
        } else {
        DrawRoadNetwork(actor);
        DrawCentralPlaza(worldTime);
        DrawWorldLandmarks(actor, worldTime);
        DrawWorldShops(actor, worldTime);
        DrawTrainingRange(actor, worldTime);

        /* Spatially cull buildings by grid cell before distance testing. */
        DrawNearbyBuildings(actor);

        /* Procedural urban trees, park greenery and coastal tree belts. */
        for (int i = 0; i < gTreeCount; i++) {
            WorldTree *tree = &gTrees[i];
            if (Distance2D(actor.x, actor.z, tree->x, tree->z) < 178.0f)
                DrawTree(tree->x, tree->z, tree->scale, tree->species);
        }

        /* Street lamps along the centre boulevards. */
        for (int i = -24; i <= 24; i++) {
            float c = (float)i*48.0f;
            float lx1 = c + 11.0f;
            float lx2 = c - 11.0f;
            float lz = (float)((i%2 == 0) ? 48 : -48);
            if (Distance2D(actor.x, actor.z, lx1, lz) < DRAW_DISTANCE) DrawStreetLamp(lx1, lz);
            if (Distance2D(actor.x, actor.z, lx2, -lz) < DRAW_DISTANCE) DrawStreetLamp(lx2, -lz);
        }

        /* Traffic and walkers are updated continuously. */
        for (int i = 0; i < TRAFFIC_COUNT; i++) {
            if (Distance2D(actor.x, actor.z, gTraffic[i].position.x,
                           gTraffic[i].position.z) < DRAW_DISTANCE) {
                DrawVehicleModel(&gTraffic[i], false);
            }
        }
        DrawSandMeetCars(actor);
        for (int i = 0; i < WALKER_COUNT; i++) {
            Pedestrian *p = &gPedestrians[i];
            float drawRange = p->isStatic ? 132.0f : 112.0f;
            if (Distance2D(actor.x, actor.z, p->position.x, p->position.z) < drawRange) {
                DrawHuman(p->position, p->yaw, p->isStatic ? 0.0f : p->phase, p->shirt,
                          p->trousers, p->skin, false, false);
            }
        }

        /* Hostile AI factions: zombie outbreak and three rival crews. */
        for (int i = 0; i < HOSTILE_CAPACITY; i++) {
            if (gHostiles[i].active &&
                Distance2D(actor.x, actor.z, gHostiles[i].position.x,
                           gHostiles[i].position.z) < 125.0f)
                DrawHostile(&gHostiles[i]);
        }

        /* Player vehicle and character. */
        DrawVehicleModel(&playerCar, true);
        if (!inCar) {
            DrawHuman(player, playerYaw, walkPhase,
                      (Color){ 32, 111, 190, 255 },
                      (Color){ 42, 53, 74, 255 },
                      (Color){ 196, 151, 117, 255 }, true,
                      false);
            DrawPlayerWeapon(player, playerYaw, gWeaponIndex, 0.0f);
        }
        DrawShotEffect();

        if (missionStep < MISSION_COUNT) {
            DrawMissionMarker(gMissionPoints[missionStep], worldTime,
                              (missionStep%2) == 0);
        }

        /* Sun/moon, tyre effects and local weather. */
        if (daylight > 0.22f) {
            DrawSphere((Vector3){actor.x - 100.0f, 72.0f, actor.z - 145.0f}, 7.0f,
                       (Color){255, 226, 166, 255});
        } else {
            DrawSphere((Vector3){actor.x + 110.0f, 78.0f, actor.z - 135.0f}, 5.3f,
                       (Color){218, 229, 244, 255});
        }
        DrawDriftEffects();
        DrawRainAround(actor, worldTime, raining);
        }
        EndMode3D();

        /* ---- Render 2D interface -------------------------------------- */
        if (state == GAME_TITLE) {
            DrawRectangle(0, 0, screenW, screenH, Fade((Color){ 5, 12, 22, 255 }, 0.54f));
            DrawPanel((Rectangle){(float)screenW*0.5f - 250.0f, 42.0f, 500.0f, 148.0f},
                      (Color){ 10, 19, 31, 238 }, Fade(C_MINT, 0.78f));
            DrawArabicLabel(AR_CITY, (Rectangle){(float)screenW*0.5f - 170.0f, 57.0f, 340.0f, 40.0f}, C_MINT);
            DrawText("CITY LIFE 3D", screenW/2 - MeasureText("CITY LIFE 3D", 34)/2,
                     102, 34, C_WHITE);
            DrawArabicLabel(AR_ULTIMATE_WORLD, (Rectangle){(float)screenW*0.5f - 170.0f, 142.0f, 340.0f, 30.0f}, C_GOLD);
            DrawText("ULTIMATE WORLD  |  V7.2.1", screenW/2 - MeasureText("ULTIMATE WORLD  |  V7.2.1", 16)/2,
                     171, 16, C_WHITE);
            DrawPanel((Rectangle){(float)screenW*0.5f - 250.0f, 202.0f, 500.0f, 92.0f},
                      (Color){ 10, 19, 31, 226 }, Fade(C_GOLD, 0.50f));
            DrawArabicLabel(AR_DEV_CREDIT, (Rectangle){(float)screenW*0.5f - 205.0f, 208.0f, 410.0f, 26.0f}, C_GOLD);
            DrawArabicLabel(AR_CREATOR_NAME, (Rectangle){(float)screenW*0.5f - 230.0f, 236.0f, 460.0f, 30.0f}, C_WHITE);
            DrawArabicLabel(AR_COPYRIGHT_LABEL, (Rectangle){(float)screenW*0.5f - 205.0f, 268.0f, 410.0f, 22.0f}, Fade(C_WHITE, 0.90f));
            DrawText("ALL RIGHTS RESERVED  © 2026", screenW/2 - MeasureText("ALL RIGHTS RESERVED  © 2026", 12)/2,
                     292, 12, Fade(C_WHITE, 0.78f));
            DrawArabicButton(playRect, AR_START, (Color){35, 137, 93, 255}, true);
            DrawArabicLabel(AR_READY, (Rectangle){(float)screenW*0.5f - 160.0f,
                            playRect.y + 76.0f, 320.0f, 26.0f}, Fade(C_WHITE, 0.82f));
            DrawArabicLabel(AR_MOVE, (Rectangle){(float)screenW*0.5f - 120.0f,
                            (float)screenH - 54.0f, 240.0f, 24.0f}, Fade(C_WHITE, 0.72f));
            DrawArabicLabel(AR_MUSIC, (Rectangle){(float)screenW*0.5f - 52.0f,
                            (float)screenH - 33.0f, 44.0f, 20.0f}, Fade(C_MINT, 0.85f));
            DrawText(TextFormat("%d", gAudioReady ? gMusicCount : 0),
                     screenW/2 + 8, screenH - 31, 14, Fade(C_MINT, 0.85f));
        } else {
            if (gIndoors) {
                DrawInteriorHUD(screenW, screenH, cash);
                DrawArabicButton(radioRect, AR_RADIO, (Color){52, 91, 116, 255}, false);
            } else {
                DrawGameHUD(screenW, screenH, cash, delivered, missionStep,
                            playerCar.speed, inCar, nearCar, (nearMission || nearShop),
                            actor, missionTarget, hasTarget,
                            stick, stickX, stickY, worldTime, gRadioEnabled, gRadioIndex,
                            playerCar.model, gDriftScore, gBestDrift, gNightAmount, raining,
                            gWeaponIndex, gWeaponAmmo[gWeaponIndex], gWeaponReserve[gWeaponIndex], points);
            }

            if (!gIndoors && !inCar) {
                int cx = screenW/2, cy = screenH/2;
                DrawCircleLines(cx, cy, 7.0f, Fade(C_WHITE, 0.82f));
                DrawLine(cx - 13, cy, cx - 5, cy, Fade(C_WHITE, 0.82f));
                DrawLine(cx + 5, cy, cx + 13, cy, Fade(C_WHITE, 0.82f));
                DrawLine(cx, cy - 13, cx, cy - 5, Fade(C_WHITE, 0.82f));
                DrawLine(cx, cy + 5, cx, cy + 13, Fade(C_WHITE, 0.82f));
            }
            DrawButton(pauseRect, "II", (Color){ 58, 73, 91, 255 }, false);
            DrawArabicButton(muteRect, gRadioEnabled ? AR_RADIO : AR_CLOSE, (Color){62, 90, 112, 255}, false);

            if (messageTimer > 0.0f) {
                Rectangle mr = {(float)screenW*0.5f - 190.0f,
                                (float)screenH - 60.0f, 380.0f, 36.0f};
                DrawPanel(mr, (Color){ 12, 22, 32, 218 }, Fade(C_MINT, 0.45f));
                int arabicMessage = GetArabicMessageLabel(message);
                if (arabicMessage >= 0) {
                    DrawArabicLabel(arabicMessage, (Rectangle){(float)screenW*0.5f - 150.0f,
                                    (float)screenH - 55.0f, 300.0f, 28.0f}, C_WHITE);
                } else {
                    DrawText(message, screenW/2 - MeasureText(message, 15)/2,
                             screenH - 50, 15, C_WHITE);
                }
            }

            if (state == GAME_PAUSED) {
                DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.62f));
                DrawPanel((Rectangle){(float)screenW*0.5f - 165.0f,
                                      (float)screenH*0.5f - 140.0f,
                                      330.0f, 290.0f},
                          (Color){ 12, 21, 34, 245 }, Fade(C_MINT, 0.75f));
                DrawArabicLabel(AR_PAUSED, (Rectangle){(float)screenW*0.5f - 100.0f,
                                (float)screenH*0.5f - 118.0f, 200.0f, 34.0f}, C_WHITE);
                DrawArabicButton(resumeRect, AR_RESUME, (Color){36, 139, 94, 255}, true);
                DrawArabicButton(restartRect, AR_RESTART, (Color){55, 99, 153, 255}, false);
                DrawArabicLabel(AR_READY, (Rectangle){(float)screenW*0.5f - 150.0f,
                                (float)screenH*0.5f + 103.0f, 300.0f, 24.0f}, Fade(C_WHITE, 0.72f));
            }
        }

        EndDrawing();
    }

    Vector3 persistPlayer = gIndoors ? gExteriorPosition : player;
    float persistYaw = playerYaw;
    if (inCar) {
        persistPlayer = playerCar.position;
        persistPlayer.x += cosf(playerCar.yaw)*3.2f;
        persistPlayer.z -= sinf(playerCar.yaw)*3.2f;
        persistYaw = playerCar.yaw;
    }
    SaveGameProgress(cash, points, missionStep, delivered, persistPlayer, persistYaw, &playerCar, worldTime);
    if (gAudioDeviceReady) {
        StopRadioPlayback();
        for (int track = 0; track < gMusicCount; track++) {
            if (IsMusicValid(gMusicTracks[track])) UnloadMusicStream(gMusicTracks[track]);
        }
        if (IsSoundValid(gEngineSound)) {
            StopSound(gEngineSound);
            UnloadSound(gEngineSound);
        }
        for (int i = 0; i < SFX_COUNT; i++) {
            if (IsSoundValid(gGameSfx[i])) UnloadSound(gGameSfx[i]);
        }
        CloseAudioDevice();
    }
    if (gArabicAtlas.id != 0) UnloadTexture(gArabicAtlas);
    CloseWindow();
    return 0;
}
