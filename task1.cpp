
#define FREEGLUT_STATIC
#define GLUT_DISABLE_ATEXIT_HACK
#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>


// ----------------------------- Config -----------------------------

static float gCoverCatX = 0.f;   
//cover cat pause control
static const float COVER_CAT_PAUSE_DUR = 0.8f;  //pause duration (seconds)
static float gCoverCatSpeedBase = 350.f;        //base speed (for resume after pause)
static bool  gCoverCatPaused = false;           //whether paused
static float gCoverCatPauseStart = 0.f;         //pause start timestamp


static int   gCoverCatDir = 1;           
static float gCoverCatSpeed = 350.f;     
static float gCoverStartTime = 1.f;       
static bool  gCoverPromptShown = false;    

//black fade in/out
static bool  gCoverTransiting = false;     
static float gCoverTransT = 0.f;           
static const float COVER_TRANS_TOTAL = 2.f; 

// // ===== Fireworks Minimal =====
struct Particle { float x, y, vx, vy, life, r, g, b; };
struct Rocket { float x, y, vx, vy, targetY; bool burst; float trailAcc; };

static std::vector<Particle> gParticles;   ///fireworks & trail
static std::vector<Rocket>   gRockets;     //ascending rockets


static const float FW_COLS[][3] = {
    {0.78f, 0.30f, 1.00f},  
    {0.20f, 0.45f, 1.00f},  
    {1.00f, 0.45f, 0.00f},  
    {1.00f, 0.55f, 0.75f}, 
    {0.20f, 0.90f, 0.35f},  
    {1.00f, 0.65f, 0.00f}  
};

static const int FW_COL_N = sizeof(FW_COLS) / sizeof(FW_COLS[0]);

static const float GRAVITY_FUSE = -40.f;     
static const float GRAVITY_BURST = 0.f;      
static const int   PARTICLE_MAX = 4000;      
static const float FIREWORK_RATE = 1.3f;     
static       float gRainbowT = 0.f;       
static const float COL_EFE9D0[3] = { 239 / 255.f, 233 / 255.f, 208 / 255.f };
static const float COL_AD786D[3] = { 173 / 255.f, 120 / 255.f, 109 / 255.f };
static const float COL_835679[3] = { 131 / 255.f,  86 / 255.f, 121 / 255.f };
static const float COL_4C2A2D[3] = { 76 / 255.f,  42 / 255.f,  45 / 255.f };
static const float COL_242546[3] = { 36 / 255.f,  37 / 255.f,  70 / 255.f };
static const float COL_161818[3] = { 22 / 255.f,  24 / 255.f,  24 / 255.f };

static const int   WIN_W = 960, WIN_H = 540;

// Flashlight & black cover (5 soft rings)
static const float LAMP_RADIUS_BASE = 50.f;
static const float LAMP_SOFT_DELTA1 = 15.f;
static const float LAMP_SOFT_DELTA2 = 23.f;
static const float LAMP_SOFT_DELTA3 = 32.f;
static const float LAMP_SOFT_DELTA4 = 48.f;

static const float BLACK_ALPHA_BASE0 = 0.90f;
static const float BLACK_ALPHA_BASE1 = 0.70f;
static const float BLACK_ALPHA_BASE2 = 0.54f;
static const float BLACK_ALPHA_BASE3 = 0.38f;
static const float BLACK_ALPHA_BASE4 = 0.24f;
static const float CELEBRATE_MIN_LAMP = 150.f;

// Camera
static const float CAMERA_ZOOM_TARGET = 0.65f;
static const float CAMERA_SHAKE_UP = 120.f;
static const float CAMERA_PAN_X = 60.f;
static const float CAMERA_SPEED_ZOOM = 0.35f;
static const float CAMERA_SPEED_LIFT = 55.f;
static const float CAMERA_SPEED_PAN = 30.f;

// Hero & animation
static const float HERO_SPEED = 125.f;
static const float ARM_SWING_DEG = 20.f;
static const float FOUND_TO_GUIDING_TIME = 0.8f;

// Interaction & scene
static const float SWITCH_RADIUS = 40.f;
static const float FIND_RADIUS = 70.f;
static const float GUIDE_STRIP_WIDTH = 4.0f;

// Unified ground/road heights
static const float GROUND_TOP = 120.f; 
static const float ROAD_TOP = 150.f; 

// Clouds layout
static const float CLOUD_SPACING_X = 300.f;
static const float CLOUD_SPACING_Y = 150.f;
static const int   CLOUD_ROWS = 2;
static const int   CLOUD_COLS_BASE = 7;

// Lighting dir
static const float SUN_DIR_X = 0.7f;
static const float SUN_DIR_Y = 0.7f;

// Flat-cut band thickness
static const float CLOUD_CUT_HARD = 60.f;
static const float CLOUD_CUT_SOFT1 = 5.f;
static const float CLOUD_CUT_SOFT2 = 8.f;

static float gCelebrateStartTime = -1.f;


// ----------------------------- State -----------------------------
enum class GameState { SEARCHING, FOUND, GUIDING, AT_SWITCH, CELEBRATE, DONE };
static GameState gState = GameState::SEARCHING;
enum class AppStage { COVER, MAIN };
static AppStage gStage = AppStage::COVER;
static int   gLastTicks = 0;
static float gDt = 0.f;
static float gTime = 0.f;

static float gCamTopLift = 0.f;
static float gCamZoom = 1.0f;
static float gCamPan = 0.f;
static float gGlobalFade = 1.0f;

static int   gWidth = WIN_W, gHeight = WIN_H;

// initial lamp position: upper center
static float gMouseX = WIN_W * 0.5f;
static float gMouseY = WIN_H * 0.72f;

static float heroX = 90.f, heroY = 75.f;
static float switchX = WIN_W - 90.f, switchY = 60.f;

static float heroArmPhase = 0.f;
static bool  heroHasLamp = false;

static bool keyL[256] = { false };
static bool keyS[512] = { false };

static float tFound = 0.f;

static bool  gThankBubbleActive = false;   //flag bubble
static float gThankBubbleStart = 0.f;     
// ----------------------------- Utils -----------------------------
inline float clampf(float v, float lo, float hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }
inline float lerp(float a, float b, float t) { t = clampf(t, 0.f, 1.f); return a + (b - a) * t; }
inline float easeInOut(float t) { t = clampf(t, 0.f, 1.f); return (t < 0.5f) ? (2.f * t * t) : (1.f - std::pow(-2.f * t + 2.f, 2.f) * 0.5f); }
inline float smooth01(float x) { x = clampf(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
inline float dist2(float x1, float y1, float x2, float y2) { float dx = x1 - x2, dy = y1 - y2; return dx * dx + dy * dy; }

inline float maxHalfW() { return (gWidth * 0.5f) / CAMERA_ZOOM_TARGET + CAMERA_PAN_X + 50.f; }
inline float maxHalfH() { return (gHeight * 0.5f) / CAMERA_ZOOM_TARGET + CAMERA_SHAKE_UP + 50.f; }

// simple hashes
inline float hash01(int x) {
    unsigned h = 2166136261u; h ^= (unsigned)x; h *= 16777619u; h ^= h >> 13; h *= 16777619u;
    return (h & 0xFFFFFF) / 16777215.0f;
}
inline float hash2d01(int ix, int iy, int salt = 0) {
    return hash01(ix * 73856093 ^ iy * 19349663 ^ (salt * 83492791));
}

// mix 3-float colors
inline void mix3(const float a[3], const float b[3], float k, float out[3]) {
    out[0] = a[0] + (b[0] - a[0]) * k;
    out[1] = a[1] + (b[1] - a[1]) * k;
    out[2] = a[2] + (b[2] - a[2]) * k;
}

// bright HSV → RGB
inline void brightHSV(float h, float s, float v, float& r, float& g, float& b) {
    int i = int(h * 6.f); float f = h * 6.f - i;
    float p = v * (1.f - s), q = v * (1.f - f * s), t = v * (1.f - (1.f - f) * s);
    switch (i % 6) {
    case 0: r = v; g = t; b = p; break; case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break; case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break; default: r = v; g = p; b = q; break;
    }
}

// Current ortho bounds (for full-cover layers)
void currentOrthoBounds(double& L, double& R, double& B, double& T) {
    double cx = gWidth * 0.5 + gCamPan;
    double cy = gHeight * 0.5 + gCamTopLift;
    double halfW = gWidth * 0.5 / gCamZoom;
    double halfH = gHeight * 0.5 / gCamZoom;
    L = cx - halfW; R = cx + halfW; B = cy - halfH; T = cy + halfH;
}

// ----------------------- Basic Drawing ---------------------------
void drawFilledRect(float x0, float y0, float x1, float y1) {
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
}
void drawLine(float x0, float y0, float x1, float y1, float w = 2.f) {
    glLineWidth(w);
    glBegin(GL_LINES);
    glVertex2f(x0, y0); glVertex2f(x1, y1);
    glEnd();
}
void drawCircleFan(float cx, float cy, float r, int seg = 64) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) {
        float a = (2.f * 3.1415926f * i) / seg;
        glVertex2f(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    glEnd();
}
inline void drawCircle(float cx, float cy, float r, int seg = 28) { // wrapper → unified impl
    drawCircleFan(cx, cy, r, seg);
}
void drawEllipseFan(float cx, float cy, float rx, float ry, int seg = 48) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) {
        float a = (2.0f * 3.1415926f * i) / seg;
        glVertex2f(cx + rx * std::cos(a), cy + ry * std::sin(a));
    }
    glEnd();
}

// ----------------------- Reusable Widgets ------------------------
//fixed top pennants
void drawBannerFixedTop() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    float y = gHeight - 8.f;
    int   N = 17;
    float w = (gWidth - 20) / (float)N * 1.3f;

    for (int i = 0; i < N; ++i) {
        float x = 10 + i * w;
        float sway = 5.0f * std::sinf(gTime * 2.5f + i * 0.6f);
        float tipX = x + w * 0.42f;
        float tipY = y - 20.f - sway;

        if (i % 2 == 0) glColor3f(0.60f, 0.45f, 0.85f);
        else            glColor3f(0.82f, 0.72f, 0.95f);

        glBegin(GL_TRIANGLES);
        glVertex2f(x, y);
        glVertex2f(x + w * 0.85f, y);
        glVertex2f(tipX, tipY);
        glEnd();

        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPointSize(2.6f);
        glBegin(GL_POINTS);
        for (int k = 0; k < 10; ++k) {
            float rx = x + w * 0.1f + (w * 0.7f) * ((k * 37) % 100) / 100.f;
            float ry = y - 2.f - 10.f * ((k * 73) % 100) / 100.f - 0.5f * sway;
            glColor4f(1, 1, 1, 0.45f);
            glVertex2f(rx, ry);
        }
        glEnd();
        glDisable(GL_BLEND);
    }
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

//button
inline void drawSwitchButton(float cx, float cy) {
    glColor3f(0.05f, 0.08f, 0.25f);
    drawFilledRect(cx - 20, cy - 10, cx + 20, cy + 30);
    glColor3f(0.95f, 0.25f, 0.55f);
    drawCircleFan(cx, cy + 10, 8, 24);
}

// ================== Balloons  ==================
static const float BAL_COLS[][3] = {
    {0.85f, 0.35f, 1.00f}, 
    {1.00f, 0.55f, 0.75f}, 
    {0.25f, 0.55f, 1.00f}, 
    {0.45f, 0.75f, 1.00f}, 
    {1.00f, 0.70f, 0.20f}, 
    {1.00f, 0.85f, 0.35f}, 
    {0.25f, 0.90f, 0.45f}, 
    {0.70f, 1.00f, 0.80f}, 
    {0.90f, 0.82f, 0.98f}, 
    {1.00f, 0.80f, 0.90f}, 
    {0.95f, 0.70f, 1.00f}, 
    {0.80f, 0.90f, 1.00f}  
};
static const int BAL_COL_N = sizeof(BAL_COLS) / sizeof(BAL_COLS[0]);

static void drawBalloonSimple(float x, float y, float scale, int colorIndex)
{
    float bob = 6.0f * std::sinf(gTime * 2.0f + x * 0.01f);
    float cx = x, cy = y + bob;
    float R = 12.0f * scale;
    const float* C = BAL_COLS[(colorIndex % BAL_COL_N + BAL_COL_N) % BAL_COL_N];
    glColor3f(C[0], C[1], C[2]);
    drawCircleFan(cx, cy, R, 28);

    //balloon knot (small triangle)
    glColor3f(C[0] * 0.7f, C[1] * 0.7f, C[2] * 0.7f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - 3.5f * scale, cy - R + 1.0f * scale);
    glVertex2f(cx + 3.5f * scale, cy - R + 1.0f * scale);
    glVertex2f(cx, cy - R - 4.5f * scale);
    glEnd();

    //string: simple bent polyline 
    glColor3f(0.20f, 0.20f, 0.22f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    float y0 = cy - R - 3.5f * scale;
    glVertex2f(cx, y0);
    glVertex2f(cx - 4.0f * scale, y0 - 10.0f * scale);
    glVertex2f(cx + 3.0f * scale, y0 - 22.0f * scale);
    glVertex2f(cx - 2.0f * scale, y0 - 34.0f * scale);
    glEnd();

    //top-right white '!' highlight: a short bar + a dot
    glColor3f(1, 1, 1);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(cx + R * 0.35f, cy + R * 0.30f);
    glVertex2f(cx + R * 0.35f, cy + R * 0.10f);
    glEnd();
    drawCircleFan(cx + R * 0.30f, cy + R * 0.02f, 2.0f * scale, 10);
}

// ================== Sets (keep original names) ==================
inline void drawBalloonsInScreen() {
    drawBalloonSimple(140, 300, 1.10f, 0); 
    drawBalloonSimple(210, 330, 0.90f, 1); 
    drawBalloonSimple(320, 312, 1.20f, 2); 
    drawBalloonSimple(430, 340, 0.80f, 3); 
    drawBalloonSimple(630, 300, 1.00f, 4); 

}
inline void drawBalloonsOnTop() {
    drawBalloonSimple(20, 600, 1.10f, 0); 
    drawBalloonSimple(1, 500, 0.90f, 1); 
    drawBalloonSimple(100, 550, 1.20f, 2); 
    drawBalloonSimple(150, 430, 0.80f, 3); 
    drawBalloonSimple(500, 540, 1.00f, 6); 
    drawBalloonSimple(710, 580, 0.80f, 3); 
    drawBalloonSimple(800, 500, 0.90f, 1); 
    drawBalloonSimple(770, 400, 1.20f, 2); 
    drawBalloonSimple(1130, 430, 0.80f, 8); 
    drawBalloonSimple(5, 500, 0.90f, 8); 
    drawBalloonSimple(1290, 540, 1.20f, 5); 
    drawBalloonSimple(150, 430, 0.80f, 3); 
    drawBalloonSimple(1200, 540, 1.00f, 9); 
    drawBalloonSimple(1000, 700, 0.80f, 7); 

}
inline void drawBalloonEdgeSet() {
    drawBalloonSimple(-120.f, 310.f, 1.25f, 3); 
    drawBalloonSimple(gWidth + 80.f, 330.f, 0.95f, 4); 
    drawBalloonSimple(gWidth + 160.f, 290.f, 1.35f, 5); 
    drawBalloonSimple(-200.f, 340.f, 0.85f, 2); 
}

// ----------------------- Sky / Backdrop / Grass ------------------
static void skyGradientColor(float y, float B, float T, float& r, float& g, float& b) {
    double Ld, Rd, Bd, Td;
    currentOrthoBounds(Ld, Rd, Bd, Td);
    const float Bv = (float)Bd + 0.02f * (float)(Td - Bd);
    const float Tv = (float)Td;

    float t = (Tv > Bv) ? clampf((y - Bv) / (Tv - Bv), 0.f, 1.f) : 0.f;

    const float c_neg1[3] = { 1.00f, 0.15f, 0.00f };
    const float c0[3] = { 1.00f, 0.55f, 0.15f };
    const float c1[3] = { 1.00f, 0.72f, 0.40f };
    const float c2[3] = { 0.45f, 0.70f, 0.95f };
    const float c3[3] = { 0.02f, 0.05f, 0.20f };

    const float k0 = 0.30f, k1 = 0.40f, k2 = 0.50f;

    float r0, g0, b0, r1, g1, b1, u;
    if (t < k0) { u = smooth01(t / k0);              r0 = c_neg1[0]; g0 = c_neg1[1]; b0 = c_neg1[2]; r1 = c0[0]; g1 = c0[1]; b1 = c0[2]; }
    else if (t < k1) { u = smooth01((t - k0) / (k1 - k0)); r0 = c0[0]; g0 = c0[1]; b0 = c0[2]; r1 = c1[0]; g1 = c1[1]; b1 = c1[2]; }
    else if (t < k2) { u = smooth01((t - k1) / (k2 - k1)); r0 = c1[0]; g0 = c1[1]; b0 = c1[2]; r1 = c2[0]; g1 = c2[1]; b1 = c2[2]; }
    else { u = smooth01((t - k2) / (1.f - k2)); r0 = c2[0]; g0 = c2[1]; b0 = c2[2]; r1 = c3[0]; g1 = c3[1]; b1 = c3[2]; }

    r = lerp(r0, r1, u); g = lerp(g0, g1, u); b = lerp(b0, b1, u);
}
static void drawSkyGradientAndDots(float L, float R, float B, float T) {
    const int slices = 120;
    float y0 = B;
    for (int i = 0; i < slices; ++i) {
        float y1 = B + (T - B) * (i + 1) / slices;
        float r0, g0, b0, r1, g1, b1;
        skyGradientColor(y0, B, T, r0, g0, b0);
        skyGradientColor(y1, B, T, r1, g1, b1);

        glBegin(GL_QUADS);
        glColor3f(r0, g0, b0); glVertex2f(L, y0);
        glVertex2f(R, y0);
        glColor3f(r1, g1, b1); glVertex2f(R, y1);
        glVertex2f(L, y1);
        glEnd();
        y0 = y1;
    }

    float skyB = B + (T - B) * 0.45f;
    float skyT = T;
    auto rand01 = [](int s)->float { unsigned h = 2166136261u; h ^= (unsigned)s; h *= 16777619u; return (h & 0xffff) / 65535.0f; };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);

    const float stepX = 20.f, stepY = 20.f;
    const float density = 0.40f;
    glPointSize(3.0f);

    for (int iy = 0; ; ++iy) {
        float y = skyB + iy * stepY; if (y > skyT) break;
        for (int ix = -32; ; ++ix) {
            float x = L + ix * stepX; if (x > R) break;
            float rv = rand01(ix * 73856093 ^ iy * 19349663);
            if (rv < density) continue;

            float r, g, b; skyGradientColor(y, B, T, r, g, b);
            r *= 0.92f; g *= 0.92f; b *= 0.92f;
            float a = 0.10f + 0.08f * rand01(ix * 83492791 ^ iy * 29765729);

            glBegin(GL_POINTS);
            glColor4f(r, g, b, a);
            glVertex2f(x, y);
            glEnd();
        }
    }
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
}
inline float cloudWarmFactorAtY(float y) {
    double Ld, Rd, Bd, Td; currentOrthoBounds(Ld, Rd, Bd, Td);
    const float Bv = (float)Bd + 0.02f * (float)(Td - Bd);
    const float Tv = (float)Td;
    float t = (Tv > Bv) ? clampf((y - Bv) / (Tv - Bv), 0.f, 1.f) : 0.f;
    float w = 1.f - smooth01((t - 0.00f) / 0.35f);
    float coldBoost = smooth01((t - 0.55f) / (1.f - 0.55f));
    w *= (1.f - 0.60f * coldBoost);
    return clampf(w, 0.f, 1.f);
}
static void fillSkyBandGradient(float left, float right, float y0, float y1,
    float B, float T, float a0, float a1, int slices = 18)
{
    slices = std::max(2, slices);
    float yy = y0;
    for (int i = 0; i < slices; ++i) {
        float yNext = y0 + (y1 - y0) * (i + 1) / slices;
        float r0, g0, b0, r1, g1, b1;
        skyGradientColor(yy, B, T, r0, g0, b0);
        skyGradientColor(yNext, B, T, r1, g1, b1);
        float ta0 = lerp(a0, a1, (yy - y0) / (y1 - y0));
        float ta1 = lerp(a0, a1, (yNext - y0) / (y1 - y0));

        glBegin(GL_QUADS);
        glColor4f(r0, g0, b0, ta0); glVertex2f(left, yy);
        glVertex2f(right, yy);
        glColor4f(r1, g1, b1, ta1); glVertex2f(right, yNext);
        glVertex2f(left, yNext);
        glEnd();
        yy = yNext;
    }
}
static void drawConfettiOverlay() {
    double Ld, Rd, Bd, Td; currentOrthoBounds(Ld, Rd, Bd, Td);
    float L = (float)Ld, R = (float)Rd, B = (float)Bd, T = (float)Td;

    float confB = B + (T - B) * 0.30f;
    float confT = T - (T - B) * 0.06f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_POINT_SMOOTH);

    glPointSize(3.f);
    const float stepCX = 34.f, stepCY = 28.f;

    auto rand01c = [](int s)->float { unsigned h = 2166136261u; h ^= (unsigned)s; h *= 16777619u; return (h & 0xffffff) / 16777215.0f; };

    for (int iy = 0; ; ++iy) {
        float y = confB + iy * stepCY; if (y > confT) break;
        for (int ix = -24; ; ++ix) {
            float x = L + ix * stepCX; if (x > R) break;

            float rv = rand01c(ix * 9151 ^ iy * 10501);
            if (rv < 0.78f) continue;

            int pick = (int)(rv * 1000.0f) % 5;
            float r = 1, g = 1, b = 1;
            if (pick == 0) { r = 0.65f; g = 0.95f; b = 0.98f; }
            if (pick == 1) { r = 0.98f; g = 0.72f; b = 0.86f; }
            if (pick == 2) { r = 0.99f; g = 0.90f; b = 0.40f; }
            if (pick == 3) { r = 0.72f; g = 0.98f; b = 0.60f; }
            if (pick == 4) { r = 0.84f; g = 0.78f; b = 0.98f; }

            float tw = 0.55f + 0.45f * std::fabs(std::sinf(gTime * 4.f + ix * 0.7f + iy * 0.5f));
            float a = 0.10f * tw;

            glBegin(GL_POINTS);
            glColor4f(r, g, b, a);
            glVertex2f(x, y);
            glEnd();
        }
    }
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
}
// 草地
static void drawGrassTufts(float L, float R, float grassB, float grassTop) {
    const float DEEP[3] = { 0.09f, 0.26f, 0.12f };
    const float MID[3] = { 0.22f, 0.60f, 0.25f };
    const float BRIGHT[3] = { 0.40f, 0.82f, 0.35f };

    const float cellW = 12.f, cellH = 9.f;
    const float tuftProb = 0.93f;
    const float clumpProb = 0.27f;
    const float alphaGrass = 0.90f;
    const float alphaClump = 0.40f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto render_grid = [&](float xOffset, float yOffset, int salt) {
        for (int iy = (int)std::floor((grassB - yOffset) / cellH) - 1;
            iy <= (int)std::ceil((grassTop - yOffset) / cellH) + 1; ++iy) {

            float y0 = iy * cellH + cellH * 0.5f + yOffset;
            if (y0 < grassB || y0 > grassTop) continue;

            for (int ix = (int)std::floor((L - xOffset) / cellW) - 1;
                ix <= (int)std::ceil((R - xOffset) / cellW) + 1; ++ix) {

                float x0 = ix * cellW + cellW * 0.5f + xOffset;
                if (x0 < L || x0 > R) continue;

                float jx = (hash2d01(ix, iy, salt + 1) - 0.5f) * cellW * 0.65f;
                float jy = (hash2d01(ix, iy, salt + 2) - 0.5f) * cellH * 0.35f;
                float cx = x0 + jx;
                float cy = y0 + jy;

                if (hash2d01(ix, iy, salt + 3) > (1.f - tuftProb)) {
                    int blades = 1 + (int)(hash2d01(ix, iy, salt + 4) * 4.0f);
                    for (int k = 0; k < blades; ++k) {
                        float r = hash2d01(ix, iy, salt + 10 + k);
                        float h = 6.5f + r * 9.5f;
                        float w = 2.0f + hash2d01(ix, iy, salt + 11 + k) * 3.0f;
                        float ang = (hash2d01(ix, iy, salt + 12 + k) - 0.5f) * 0.56f;

                        float bx0 = cx - w * 0.5f, bx1 = cx + w * 0.5f, by = cy;
                        float tx = cx + h * std::sin(ang);
                        float ty = cy + h * std::cos(ang);

                        float col1[3], tmp[3];
                        mix3(DEEP, MID, hash2d01(ix, iy, salt + 13 + k) * 0.75f, tmp);
                        mix3(tmp, BRIGHT, hash2d01(ix, iy, salt + 14 + k) * 0.55f, col1);

                        glBegin(GL_TRIANGLES);
                        glColor4f(col1[0], col1[1], col1[2], alphaGrass);
                        glVertex2f(bx0, by);
                        glVertex2f(bx1, by);
                        glVertex2f(tx, ty);
                        glEnd();

                        float vein[3]; mix3(DEEP, col1, 0.32f, vein);
                        glLineWidth(1.0f);
                        glBegin(GL_LINES);
                        glColor4f(vein[0], vein[1], vein[2], alphaGrass * 0.62f);
                        glVertex2f(cx, by);
                        glVertex2f(tx, ty);
                        glEnd();
                    }
                    if (hash2d01(ix, iy, salt + 21) > 0.6f) {
                        float h = 5.0f, w = 1.6f;
                        float ang = (hash2d01(ix, iy, salt + 22) - 0.5f) * 0.4f;
                        float bx0 = cx - w * 0.5f, bx1 = cx + w * 0.5f, by = cy - 1.f;
                        float tx = cx + h * std::sin(ang);
                        float ty = by + h * std::cos(ang);
                        float col[3]; mix3(MID, BRIGHT, 0.5f, col);
                        glBegin(GL_TRIANGLES);
                        glColor4f(col[0], col[1], col[2], alphaGrass * 0.9f);
                        glVertex2f(bx0, by); glVertex2f(bx1, by); glVertex2f(tx, ty);
                        glEnd();
                    }
                }

                if (hash2d01(ix, iy, salt + 5) < clumpProb) {
                    float r = hash2d01(ix, iy, salt + 6);
                    float radx = 5.5f + r * 11.f;
                    float rady = 3.0f + r * 6.f;
                    float darkK = hash2d01(ix, iy, salt + 7);
                    float col[3];
                    mix3(DEEP, MID, 0.50f + 0.5f * darkK, col);
                    if (hash2d01(ix, iy, salt + 8) > 0.88f) mix3(col, BRIGHT, 0.28f, col);

                    glBegin(GL_TRIANGLE_FAN);
                    glColor4f(col[0], col[1], col[2], alphaClump);
                    glVertex2f(cx, cy);
                    const int seg = 12;
                    for (int s = 0; s <= seg; ++s) {
                        float a = (2.f * 3.1415926f * s) / seg;
                        float ex = cx + radx * std::cos(a);
                        float ey = cy + rady * std::sin(a) * 0.85f;
                        glVertex2f(ex, ey);
                    }
                    glEnd();
                }
            }
        }
        };
    render_grid(0.f, 0.f, 0);
    glDisable(GL_BLEND);
}
void drawExtendedBackdrop() {
    double Ld, Rd, Bd, Td;
    currentOrthoBounds(Ld, Rd, Bd, Td);
    float viewW = float(Rd - Ld), viewH = float(Td - Bd);

    float L = (float)Ld - 3.0f * viewW;
    float R = (float)Rd + 3.0f * viewW;
    float B = (float)Bd - 2.0f * viewH;
    float T = (float)Td + 3.0f * viewH;

    drawSkyGradientAndDots(L, R, B, T);

    const float GRASS_BOTTOM = -150.f;
    glColor3f(0.45f, 0.55f, 0.25f);
    drawFilledRect(L, GRASS_BOTTOM, R, GROUND_TOP);
    drawGrassTufts(L, R, GRASS_BOTTOM, GROUND_TOP);

    glColor3f(0.30f, 0.30f, 0.33f);
    drawFilledRect(L, GROUND_TOP, R, ROAD_TOP);
}

// ----------------------- Clouds ----------------------------------
void drawCloudBlob(float cx, float cy, float baseR, float aspect, float tint,
    float highlightAlpha = 0.75f, float shadowAlpha = 0.55f)
{
    const float W = cloudWarmFactorAtY(cy);

    const float baseCold[3] = { 0.82f * tint, 0.86f * tint, 0.94f * tint };
    const float hiCold[3] = { 0.94f, 0.96f, 1.00f };
    const float shCold[3] = { 0.68f * tint, 0.72f * tint, 0.82f * tint };

    const float baseWarm[3] = { 0.96f, 0.86f, 0.76f };
    const float hiWarm[3] = { 1.00f, 0.90f, 0.70f };
    const float shWarm[3] = { 0.78f, 0.68f, 0.62f };

    float baseCol[3], hiCol[3], shCol[3];
    mix3(baseCold, baseWarm, W, baseCol);
    mix3(hiCold, hiWarm, W, hiCol);
    mix3(shCold, shWarm, 0.60f * W, shCol);

    float rx = baseR, ry = baseR * aspect;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(shCol[0], shCol[1], shCol[2], shadowAlpha);
    drawEllipseFan(cx - 0.35f * rx, cy - 0.28f * ry, rx * 1.05f, ry * 1.05f, 56);
    drawEllipseFan(cx - 0.55f * rx, cy + 0.10f * ry, rx * 0.55f, ry * 0.50f, 48);

    glColor4f(baseCol[0], baseCol[1], baseCol[2], 0.90f);
    drawEllipseFan(cx, cy, rx * 0.95f, ry * 0.95f, 56);
    drawEllipseFan(cx + 0.55f * rx, cy - 0.12f * ry, rx * 0.55f, ry * 0.50f, 48);
    drawEllipseFan(cx - 0.45f * rx, cy + 0.08f * ry, rx * 0.50f, ry * 0.45f, 48);
    drawEllipseFan(cx + 0.15f * rx, cy + 0.18f * ry, rx * 0.35f, ry * 0.30f, 40);

    glColor4f(hiCol[0], hiCol[1], hiCol[2], highlightAlpha);
    drawEllipseFan(cx + 0.38f * rx, cy + 0.26f * ry, rx * 0.45f, ry * 0.40f, 48);
    drawEllipseFan(cx + 0.70f * rx, cy + 0.18f * ry, rx * 0.28f, ry * 0.24f, 40);
    drawEllipseFan(cx + 0.22f * rx, cy - 0.10f * ry, rx * 0.25f, ry * 0.20f, 36);

    float rimA = 0.18f + 0.10f * W;
    glColor4f(baseCol[0], baseCol[1], baseCol[2], rimA);
    drawEllipseFan(cx, cy, rx * 1.12f, ry * 1.10f, 64);

    float rimWarm[3] = { 1.00f, 0.85f, 0.60f };
    float rimK = 0.10f + 0.25f * cloudWarmFactorAtY(cy);
    glColor4f(rimWarm[0], rimWarm[1], rimWarm[2], 0.06f * rimK);
    drawEllipseFan(cx, cy - 0.14f * ry, rx * 0.98f, ry * 0.86f, 48);

    glDisable(GL_BLEND);
}
void drawCloudCluster(float cx, float cy, float baseR, float aspect, float tint, int lobes = 4)
{
    glEnable(GL_STENCIL_TEST);
    glClearStencil(0);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    float rx = baseR, ry = baseR * aspect;
    drawEllipseFan(cx, cy, rx * 1.28f, ry * 1.28f, 64);
    drawEllipseFan(cx - rx * 0.65f, cy + ry * 0.10f, rx * 0.80f, ry * 0.80f, 56);
    drawEllipseFan(cx + rx * 0.70f, cy - ry * 0.12f, rx * 0.78f, ry * 0.78f, 56);
    if (lobes >= 4) drawEllipseFan(cx + rx * 0.18f, cy + ry * 0.22f, rx * 0.60f, ry * 0.60f, 48);
    if (lobes >= 5) drawEllipseFan(cx - rx * 0.25f, cy - ry * 0.18f, rx * 0.58f, ry * 0.58f, 48);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    drawCloudBlob(cx, cy, baseR, aspect, tint, 0.78f, 0.58f);
    drawCloudBlob(cx - baseR * 0.65f, cy + baseR * 0.10f, baseR * 0.65f, aspect * 0.95f, tint * 0.98f, 0.76f, 0.56f);
    drawCloudBlob(cx + baseR * 0.70f, cy - baseR * 0.12f, baseR * 0.60f, aspect * 0.92f, tint * 1.00f, 0.78f, 0.55f);
    if (lobes >= 4) drawCloudBlob(cx + baseR * 0.18f, cy + baseR * 0.22f, baseR * 0.42f, aspect * 0.90f, tint * 1.02f, 0.80f, 0.52f);
    if (lobes >= 5) drawCloudBlob(cx - baseR * 0.25f, cy - baseR * 0.18f, baseR * 0.40f, aspect * 0.88f, tint * 0.97f, 0.78f, 0.58f);

    double Ld, Rd, Bd, Td; currentOrthoBounds(Ld, Rd, Bd, Td);
    float flatY = cy - baseR * 0.30f;
    float left = cx - baseR * 1.70f;
    float right = cx + baseR * 1.70f;

    fillSkyBandGradient(left, right, flatY - CLOUD_CUT_HARD, flatY, (float)Bd, (float)Td, 1.0f, 1.0f, 22);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    fillSkyBandGradient(left, right, flatY, flatY + CLOUD_CUT_SOFT1, (float)Bd, (float)Td, 0.45f, 0.12f, 10);
    fillSkyBandGradient(left, right, flatY + CLOUD_CUT_SOFT1, flatY + CLOUD_CUT_SOFT1 + CLOUD_CUT_SOFT2,
        (float)Bd, (float)Td, 0.25f, 0.0f, 10);
    glDisable(GL_BLEND);

    glDisable(GL_STENCIL_TEST);
}
void drawCloudsClustersFlat() {
    double Ld, Rd, Bd, Td; currentOrthoBounds(Ld, Rd, Bd, Td);
    float L = (float)Ld - gWidth, R = (float)Rd + gWidth;
    float B = (float)Bd, T = (float)Td;

    float skyB = B + (T - B) * 0.55f;
    float skyT = B + (T - B) * 0.92f;

    auto rand01 = [](int s)->float { unsigned h = 2166136261u; h ^= (unsigned)s; h *= 16777619u; return (h & 0xffff) / 65535.0f; };

    for (int row = 0; row < CLOUD_ROWS; ++row) {
        float yCenter = lerp(skyB, skyT, (row + 0.5f) / CLOUD_ROWS);
        float spacingX = CLOUD_SPACING_X * (1.0f + 0.18f * row);
        float spacingY = CLOUD_SPACING_Y;
        int count = std::max(CLOUD_COLS_BASE - row, (int)std::ceil((R - L) / spacingX) + 2);

        for (int i = -2; i < count + 2; ++i) {
            float jx = (rand01(i * 97 + row * 313) - 0.5f) * spacingX * 0.35f;
            float jy = (rand01(i * 197 + row * 911) - 0.5f) * spacingY * 0.35f;
            float cx = L + i * spacingX + jx;
            float cy = yCenter + jy;

            float s = 1.00f - 0.22f * row;
            float baseR = (54.f + 26.f * rand01(i * 31 + row * 7)) * s;
            float aspect = 0.70f + 0.20f * rand01(i * 211 + row * 19);
            float tint = 0.86f + 0.06f * rand01(i * 571 + row * 43);
            int   lobes = 3 + (rand01(i * 887 + row * 17) > 0.65f);

            drawCloudCluster(cx, cy, baseR, aspect, tint, lobes);
        }
    }
}

// ----------------------- Campus (DRY applied) --------------------
static const float BLD_BODY[3] = { 0.90f, 0.90f, 0.86f };
static const float BLD_EDGE[3] = { 0.35f, 0.35f, 0.38f };
static const float BLD_GLASS[3] = { 0.60f, 0.76f, 0.92f };
static const float BLD_GLASS_D[3] = { 0.38f, 0.58f, 0.78f };

static void drawStairPlatformGeneric(float x0, float x1, float groundY,
    int N = 5, float totalH = 40.f, float padBase = 6.f, float padStep = 5.f,
    const float darkCol[3] = nullptr, const float lightCol[3] = nullptr)
{
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    const float DARK_DEF[3] = { 0.26f, 0.26f, 0.30f };
    const float LIGHT_DEF[3] = { 0.82f, 0.82f, 0.86f };
    const float EDGE[3] = { 0.14f, 0.14f, 0.16f };

    const float* DARK = darkCol ? darkCol : DARK_DEF;
    const float* LIGHT = lightCol ? lightCol : LIGHT_DEF;

    const float H = totalH / N;

    for (int i = 0; i < N; ++i) {
        float h1 = groundY + i * H;
        float h2 = groundY + (i + 1) * H;
        float pad = padBase + padStep * i;

        glColor3fv((i % 2 == 0) ? DARK : LIGHT);
        glBegin(GL_QUADS);
        glVertex2f(x0 + pad, h1);
        glVertex2f(x1 - pad, h1);
        glVertex2f(x1 - pad, h2);
        glVertex2f(x0 + pad, h2);
        glEnd();

        if (i < N - 1) {
            glColor3fv(EDGE);
            glLineWidth(1.4f);
            glBegin(GL_LINES);
            glVertex2f(x0 + pad, h2);
            glVertex2f(x1 - pad, h2);
            glEnd();
        }
    }

    glColor3f(0.12f, 0.12f, 0.14f);
    glLineWidth(1.6f);
    glBegin(GL_LINES);
    glVertex2f(x0 + padBase, groundY);
    glVertex2f(x1 - padBase, groundY);
    glEnd();

    glPopAttrib();
}
static void drawStripedBlock(float x0, float y0, float x1, float y1, float lineStep = 6.f) {
    glColor3fv(BLD_BODY);
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
    glColor3f(1.f, 1.f, 1.f);
    glLineWidth(1.0f);
    for (float y = y0 + 3.f; y <= y1 - 3.f; y += lineStep) {
        glBegin(GL_LINES);
        glVertex2f(x0 + 4.f, y);
        glVertex2f(x1 - 4.f, y);
        glEnd();
    }
    glColor3fv(BLD_EDGE);
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
}
static void drawGlassBand(float x0, float y0, float x1, float y1, float alpha = 0.60f) {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(BLD_GLASS[0], BLD_GLASS[1], BLD_GLASS[2], alpha);
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
    glDisable(GL_BLEND);
    glColor3fv(BLD_GLASS_D);
    glLineWidth(1.0f);
    glBegin(GL_LINES); glVertex2f(x0, y0); glVertex2f(x1, y0); glEnd();
    glBegin(GL_LINES); glVertex2f(x0, y1); glVertex2f(x1, y1); glEnd();
}
static void drawWindowGrid(float xStart, float xEnd, float yStart, float yEnd,
    float stepX, float stepY, float w, float h,
    float r, float g, float b)
{
    glColor3f(r, g, b);
    for (float x = xStart; x <= xEnd; x += stepX)
        for (float y = yStart; y >= yEnd; y -= stepY)
            drawFilledRect(x, y, x + w, y + h);
}

//shadow
static inline void drawSimpleShadowSkewed(float x0, float x1,
    float yTop, float depth = 20.f,
    float alpha = 0.22f, float skewX = 0.f)
{
    float yBot = yTop - depth;
    float xb0 = x0 + skewX, xb1 = x1 + skewX;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.f, 0.f, 0.f, alpha);
    glBegin(GL_QUADS);
    glVertex2f(x0, yTop); glVertex2f(x1, yTop);
    glVertex2f(xb1, yBot); glVertex2f(xb0, yBot);
    glEnd();
    glDisable(GL_BLEND);
}


//generic text: fit box and center

static inline void drawStrokeTextFitBoxCenter(float cx, float cy,
    const char* text, float targetW, float targetH,
    float r, float g, float b,
    float lineW = 3.0f, float stretchX = 1.0f)
{
    void* font = GLUT_STROKE_ROMAN;
    float tw = 0.f; for (const char* p = text; *p; ++p) tw += glutStrokeWidth(font, *p);
    if (tw <= 0.f) return;

    const float glyphH = 119.05f;
    float sx = targetW / (tw * stretchX);
    float sy = targetH / glyphH;
    float s = std::min(sx, sy);

    glPushMatrix();
    glTranslatef(cx - (tw * s * stretchX) * 0.5f, cy - (glyphH * s) * 0.5f, 0.f);
    glScalef(s * stretchX, s, 1.f);
    glLineWidth(lineW);
    glColor3f(r, g, b);
    for (const char* p = text; *p; ++p) glutStrokeCharacter(font, *p);
    glPopMatrix();
}

//special: XJTLU blue stroke text

static void drawMonumentTextBlue_XJTLU(float cx, float yMid,
    const char* text = "XJTLU",
    float targetW = 260.f, float targetH = 26.f)
{
    float tw = 0.f; for (const char* p = text; *p; ++p) tw += glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
    if (tw <= 1.f) return;
    float sx = targetW / tw;
    float sy = targetH / 119.05f;
    float s = std::min(sx, sy);

    glPushMatrix();
    const float stretch = 2.5f;
    glTranslatef(cx - (tw * s * stretch) * 0.40f, yMid - (targetH * 0.4f), 0.f);
    glScalef(s * stretch, s * 1.3f, 1.f);
    glLineWidth(6.5f);
    glColor3f(0.05f, 0.10f, 0.26f);
    for (const char* p = text; *p; ++p) glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    glPopMatrix();
}

//stone monument
static void drawStoneMonumentFlex(float cx, float baseY,
    float W = 340.f, float H = 66.f,
    const float bg[3] = nullptr,
    const float edge[3] = nullptr,
    const char* text = "",
    const float textRGB[3] = nullptr,
    float textW = 0.f, float textH = 0.f,
    float borderW = 1.6f,
    bool withStripe = true,
    bool withShield = true,
    bool withShadow = true,
    float shieldOffsetX = 32.f)
{
    const float BG_DEF[3] = { 0.70f, 0.72f, 0.74f };
    const float EDGE_DEF[3] = { 0.35f, 0.35f, 0.38f };
    const float TXT_DEF[3] = { 0.05f, 0.10f, 0.26f };
    const float* BG = bg ? bg : BG_DEF;
    const float* EDGE = edge ? edge : EDGE_DEF;
    const float* TXT = textRGB ? textRGB : TXT_DEF;
    if (textW <= 0.f) textW = W * 0.65f;
    if (textH <= 0.f) textH = H * 0.42f;

    float x0 = cx - W * 0.5f, x1 = cx + W * 0.5f, y0 = baseY, y1 = baseY + H;

    //shadow
    if (withShadow)
        drawSimpleShadowSkewed(x0 + 6.f, x1 - 6.f, y0 + 3.f, 30.f, 0.20f, -20.f);

    glColor3f(BG[0], BG[1], BG[2]);
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
    if (withStripe) {
        glColor3f(0.86f, 0.86f, 0.88f);
        for (float y = y0 + 8; y < y1 - 8; y += 5.f) {
            glBegin(GL_LINES); glVertex2f(x0 + 8, y); glVertex2f(x1 - 8, y); glEnd();
        }
    }

    //shield

    if (withShield) {
        float shieldCx = x0 + shieldOffsetX, shieldCy = y0 + H * 0.5f;
        float shieldW = 32.f, shieldH = 40.f;
        glColor3f(0.05f, 0.10f, 0.26f);
        glBegin(GL_POLYGON);
        glVertex2f(shieldCx - shieldW * 0.5f, shieldCy + shieldH * 0.4f);
        glVertex2f(shieldCx + shieldW * 0.5f, shieldCy + shieldH * 0.4f);
        glVertex2f(shieldCx + shieldW * 0.5f - 4.f, shieldCy - shieldH * 0.25f);
        glVertex2f(shieldCx, shieldCy - shieldH * 0.5f);
        glVertex2f(shieldCx - shieldW * 0.5f + 4.f, shieldCy - shieldH * 0.25f);
        glEnd();

        glColor3f(0.15f, 0.25f, 0.60f);
        glLineWidth(1.4f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(shieldCx - shieldW * 0.5f, shieldCy + shieldH * 0.4f);
        glVertex2f(shieldCx + shieldW * 0.5f, shieldCy + shieldH * 0.4f);
        glVertex2f(shieldCx + shieldW * 0.5f - 4.f, shieldCy - shieldH * 0.25f);
        glVertex2f(shieldCx, shieldCy - shieldH * 0.5f);
        glVertex2f(shieldCx - shieldW * 0.5f + 4.f, shieldCy - shieldH * 0.25f);
        glEnd();

        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex2f(shieldCx - 6.f, shieldCy + 6.f);
        glVertex2f(shieldCx + 6.f, shieldCy + 6.f);
        glVertex2f(shieldCx, shieldCy - 4.f);
        glEnd();
    }

    //text
    if (text && text[0])
        drawStrokeTextFitBoxCenter(cx, y0 + H * 0.50f, text, textW, textH, TXT[0], TXT[1], TXT[2], 3.0f, 1.0f);
    else
        // 默认显示 XJTLU 校名
        drawMonumentTextBlue_XJTLU(cx, y0 + H * 0.45f, "XJTLU", W - 110.f, 28.f);

    glColor3f(EDGE[0], EDGE[1], EDGE[2]);
    glLineWidth(borderW);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
}

// ---- flag ----
static void drawFlagsOnLine(float cx, float yBase,
    float spacing = 68.f, float poleH = 85.f, int n = 5)
{
    float start = cx - spacing * (n - 1) * 0.5f;
    for (int i = 0; i < n; ++i) {
        float x = start + i * spacing;
        glColor3f(0.78f, 0.78f, 0.82f);
        glLineWidth(2.6f);
        glBegin(GL_LINES);
        glVertex2f(x, yBase);
        glVertex2f(x, yBase + poleH);
        glEnd();

        float y = yBase + poleH - 18.f;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.33f, 0.24f, 0.52f, 0.92f);
        glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + 36, y); glVertex2f(x + 36, y + 9); glVertex2f(x, y + 9);
        glEnd();
        glColor4f(0.56f, 0.46f, 0.74f, 0.90f);
        glBegin(GL_QUADS);
        glVertex2f(x, y + 9); glVertex2f(x + 36, y + 9); glVertex2f(x + 36, y + 18); glVertex2f(x, y + 18);
        glEnd();
        glDisable(GL_BLEND);
    }
}

// ---- combo: monument + flags ----
static void drawMonumentWithFlags(float cx, float baseY,
    float W = 340.f, float H = 66.f,
    float flagSpacing = 68.f, float poleH = 85.f)
{
    drawStoneMonumentFlex(cx, baseY, W, H);
    drawFlagsOnLine(cx, baseY + H, flagSpacing, poleH, 5);
}

// —— FB ----
static void drawCentralMainBuildingFront(float cx, float groundY,
    float W = 520.f, float H = 200.f)
{
    const float SCALE = 0.88f;
    float Wl = W * SCALE;
    float x0 = cx - Wl * 0.5f, x1 = cx + Wl * 0.5f;

    const int   STAIR_N = 5;
    const float STAIR_H = 36.f;
    drawStairPlatformGeneric(x0, x1, groundY, STAIR_N, STAIR_H, 6.f, 6.f);
    float baseY = groundY + STAIR_H + 6.f;

    const float sideRatio = 0.25f;
    const float gapX = 1.f;
    float wSide = Wl * sideRatio;
    float wMid = std::max(Wl - 2 * wSide - 2 * gapX, 60.f);

    float hMid = H;
    float hSide = H * 0.62f;

    float Lx0 = x0, Lx1 = x0 + wSide;
    float Mx0 = Lx1 + gapX, Mx1 = Lx1 + gapX + wMid;
    float Rx0 = Mx1 + gapX, Rx1 = x1;

    drawStripedBlock(Lx0 + 6, baseY, Lx1 - 6, baseY + hSide, 6.f);
    drawStripedBlock(Rx0 + 6, baseY, Rx1 - 6, baseY + hSide, 6.f);
    drawStripedBlock(Mx0 + 4, baseY, Mx1 - 4, baseY + hMid * 0.70f, 6.f);

    drawGlassBand(Lx1 - 3.f, baseY + 8.f, Lx1 + 3.f, baseY + hSide - 8.f, 0.45f);
    drawGlassBand(Mx1 - 3.f, baseY + 8.f, Mx1 + 3.f, baseY + hSide - 8.f, 0.45f);
    {
        float x0 = Mx0 + 17.f, x1 = Mx1 - 20.f;
        float y0 = baseY + 12.f, y1 = baseY + hMid * 0.70f - 20.f;
        drawWindowGrid(x0, x1, y1, y0, /*stepX*/26.f, /*stepY*/22.f, /*w*/15.f, /*h*/10.f,
            0.36f, 0.62f, 0.80f);
    }

    {
        float x0 = Lx0 + 15.f, x1 = Lx1 - 12.f;
        float y0 = baseY + 12.f, y1 = baseY + hSide - 20.f;
        drawWindowGrid(x0, x1, y1, y0, /*stepX*/28.f, /*stepY*/24.f, /*w*/15.f, /*h*/10.f,
            0.36f, 0.62f, 0.80f);
    }
    {
        float x0 = Rx0 + 15.f, x1 = Rx1 - 12.f;
        float y0 = baseY + 12.f, y1 = baseY + hSide - 20.f;
        drawWindowGrid(x0, x1, y1, y0, /*stepX*/28.f, /*stepY*/24.f, /*w*/15.f, /*h*/10.f,
            0.36f, 0.62f, 0.80f);
    }
    drawMonumentWithFlags(cx, groundY - 18.f, 320.f, 64.f, 64.f, 78.f);

}
// —— CB ——
static void drawRightIconicBuildingFront(float cx, float groundY, float W = 190.f, float H = 190.f) {
    float x0 = cx - W * 0.5f, x1 = cx + W * 0.5f;
    float y0 = groundY, y1 = groundY + H;

    float stepTotalH = 38.f;
    drawStairPlatformGeneric(x0, x1, GROUND_TOP, 7, 60.f, 4.f, 4.5f);
    float baseY = y0 + stepTotalH + 6.f;

    float lowerH = H * 0.42f;
    float L0 = x0 + 10.f, R0 = x1 - 6.f;
    drawStripedBlock(L0, baseY, R0, baseY + lowerH, 6.f);

    float bandH = 16.f;
    drawGlassBand(L0 + 2.f, baseY + lowerH, R0 - 2.f, baseY + lowerH + bandH, 0.55f);

    float upperTopY = y1;
    float upperBaseY = baseY + lowerH + bandH;

    float slitW = 18.f;
    drawGlassBand(cx - slitW * 0.5f, upperBaseY + 4.f, cx + slitW * 0.5f, upperTopY - 4.f, 0.50f);

    float LUx0 = L0, LUx1 = cx - slitW * 0.5f - 4.f;
    float LUy0 = upperBaseY + 2.f, LUy1 = upperTopY - 18.f;
    drawStripedBlock(LUx0, LUy0, LUx1, LUy1, 6.f);

    float RUx0 = cx + slitW * 0.5f + 4.f, RUx1 = R0;
    float RUy0 = upperBaseY + 2.f, RUy1 = upperTopY - 6.f;
    drawStripedBlock(RUx0, RUy0, RUx1, RUy1, 6.f);

    glColor3f(1.f, 1.f, 1.f);
    glLineWidth(1.4f);
    glBegin(GL_LINES);
    glVertex2f(L0 + 6.f, baseY + lowerH * 0.25f);
    glVertex2f(R0 - 10.f, baseY + lowerH * 0.25f + 22.f);
    glEnd();

    glColor3fv(BLD_EDGE);
    glLineWidth(1.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, baseY - 2.f); glVertex2f(x1, baseY - 2.f);
    glVertex2f(x1, y1);          glVertex2f(x0, y1);
    glEnd();
}

// --------GYM--------
static const float GYM_BLUE[3] = { 0.24f, 0.62f, 0.90f };
static const float GYM_ORANGE[3] = { 0.98f, 0.58f, 0.18f };
static const float GYM_GREEN[3] = { 0.18f, 0.72f, 0.32f };
static const float GYM_WHITE[3] = { 0.96f, 0.96f, 0.98f };
enum class GymScheme { BLUE_TO_ORANGE, ORANGE_TO_GREEN };
static void drawLeftBuildingUnit(float x0, float y0, float W = 90.f, float H = 140.f) {
    glColor3f(0.70f, 0.28f, 0.14f);
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x0 + W, y0); glVertex2f(x0 + W, y0 + H); glVertex2f(x0, y0 + H);
    glEnd();

    glColor3f(0.90f, 0.45f, 0.22f);
    glLineWidth(1.0f);
    const float step = 14.f;
    for (float x = x0 + step; x < x0 + W; x += step) { glBegin(GL_LINES); glVertex2f(x, y0); glVertex2f(x, y0 + H); glEnd(); }
    for (float y = y0 + step; y < y0 + H; y += step) { glBegin(GL_LINES); glVertex2f(x0, y); glVertex2f(x0 + W, y); glEnd(); }

    {
        const float cell = 14.f;                     
        int cols = (int)(W / cell);                
        int rows = (int)(H / cell);                  
        if (cols > 2 && rows > 1) {
            auto rand01_row = [](int r)->float {
                unsigned h = 2166136261u;            
                h ^= (unsigned)r;  h *= 16777619u;
                h ^= (unsigned)(r * 2654435761u);    
                return (h & 0xFFFFFFu) / 16777215.0f;
                };

            glColor3f(0.35f, 0.10f, 0.05f);
            int prev_ix = -9999;                     // record
            for (int iy = 1; iy < rows; ++iy) {
                int ix = 1 + (int)(rand01_row(iy) * (float)(cols - 2));
                // avoid the same cloun
                if (ix == prev_ix) ix = 1 + ((ix - 1 + 1) % (cols - 2));
                prev_ix = ix;

                float gx = x0 + ix * cell - 5.f;
                float gy = y0 + iy * cell - 5.f;

                glBegin(GL_QUADS);
                glVertex2f(gx, gy);
                glVertex2f(gx + 10.f, gy);
                glVertex2f(gx + 10.f, gy + 10.f);
                glVertex2f(gx, gy + 10.f);
                glEnd();
            }
        }
    }

    glColor3f(0.20f, 0.08f, 0.05f);
    glLineWidth(1.4f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0); glVertex2f(x0 + W, y0); glVertex2f(x0 + W, y0 + H); glVertex2f(x0, y0 + H);
    glEnd();
}
static void drawGlassCorridor(float x0, float y0, float W = 40.f, float H = 24.f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.55f, 0.70f, 0.85f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x0 + W, y0); glVertex2f(x0 + W, y0 + H); glVertex2f(x0, y0 + H);
    glEnd();
    glDisable(GL_BLEND);

    glColor3f(0.38f, 0.52f, 0.66f);
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0); glVertex2f(x0 + W, y0); glVertex2f(x0 + W, y0 + H); glVertex2f(x0, y0 + H);
    glEnd();
}
static void drawLeftBuildingGroup(float baseX, float baseY) {
    const float unitW = 65.f, unitH = 155.f, gap = 19.f;
    const float corridorH = 24.f;
    const float corridorYoff = unitH * 0.40f;

    for (int i = 0; i < 4; ++i) {
        float x = baseX + i * (unitW + gap);
        drawLeftBuildingUnit(x, baseY, unitW, unitH);
        if (i < 3) {
            float cx = x + unitW;
            float cy = baseY + corridorYoff;
            drawGlassCorridor(cx, cy, gap, corridorH);
        }
    }
}
// —— grass ——
static inline void drawCampusWideShadowFlat(
    float depth = 32.f,
    float alpha = 0.22f) {
    double Ld, Rd, Bd, Td; currentOrthoBounds(Ld, Rd, Bd, Td);
    float L = (float)Ld ;
    float R = (float)Rd ;
    float yTop = GROUND_TOP;
    float yBot = yTop - depth;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.f, 0.f, 0.f, alpha);

    glBegin(GL_QUADS);
    glVertex2f(L, yTop); glVertex2f(R, yTop);
    glVertex2f(R, yBot); glVertex2f(L, yBot);
    glEnd();

    glDisable(GL_BLEND);
}
// pixel
static inline void px(float x, float y, float w, float h, float r, float g, float b, float a = 1.f) {
    if (a < 1.f) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
    glColor4f(r, g, b, a); drawFilledRect(x, y, x + w, y + h);
    if (a < 1.f) glDisable(GL_BLEND);
}

// —— pixel tree ——
void drawTreeSimple(float x, float baseY, float scale = 1.0f) {
    float W_visual = 22.f * scale;
    drawSimpleShadowSkewed(x - W_visual * 0.5f, x + W_visual * 0.5f,
        baseY - 35.f, 14.f, 0.20f, -13.f);

    const float G1[3] = { 0.18f,0.55f,0.22f };
    const float G2[3] = { 0.28f,0.70f,0.30f };
    const float G3[3] = { 0.40f,0.82f,0.38f };
    const float BR[3] = { 0.40f,0.28f,0.16f };

    float W = 22.f * scale;       // width
    float BX = x - W * 0.5f;      
    float TY = baseY;             // botton

    // 冠层三层
    px(BX, TY + 1.f * scale, W, 8.f * scale, G1[0], G1[1], G1[2]);
    px(BX + 2.f * scale, TY + 7.f * scale, W - 4.f * scale, 6.f * scale, G1[0], G1[1], G1[2]);
    px(BX + 4.f * scale, TY + 8.f * scale, W - 8.f * scale, 4.f * scale, G2[0], G2[1], G2[2]);
    px(BX + 6.f * scale, TY + 11.f * scale, W - 12.f * scale, 5.f * scale, G3[0], G3[1], G3[2]);

    // 边缘小叶团
    px(BX - 3.f * scale, TY + 6.f * scale, 6.f * scale, 5.f * scale, G2[0], G2[1], G2[2], 0.95f);
    px(BX + W - 3.f * scale, TY + 6.f * scale, 6.f * scale, 5.f * scale, G2[0], G2[1], G2[2], 0.95f);

    // 树干
    float trunkW = 6.f * scale, trunkH = 20.f * scale;
    px(x - trunkW * 0.5f, baseY - trunkH, trunkW, trunkH, BR[0], BR[1], BR[2]);
}

//绘制横向排列的人字形(V形)折线
static void drawGymVPattern(float x0, float y0, float x1, float y1,
    float spacingX = 40.f, float spacingY = 36.f, float ampX = 18.f, float lineW = 5.f)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float COLORS[4][3] = {
        {0.95f, 0.55f, 0.15f},
        {0.98f, 0.85f, 0.25f},
        {0.35f, 0.80f, 0.40f},
        {0.20f, 0.55f, 0.90f}
    };

    glLineWidth(lineW);
    int nRows = int((y1 - y0) / spacingY) + 1;
    for (int row = 0; row < nRows; ++row) {
        float yBase = y0 + row * spacingY;
        const float* c = COLORS[row % 4];
        glColor3f(c[0], c[1], c[2]);

        glBegin(GL_LINE_STRIP);
        int nCols = int((x1 - x0) / (spacingX * 0.5f))+2;  
        for (int i = 0; i < nCols; ++i) {
            float x = x0 + i * spacingX * 0.5f;
            float y = yBase + ((i % 2 == 0) ? 0.f : spacingY * 0.5f);
            glVertex2f(x, y);
        }
        glEnd();
    }

    glDisable(GL_BLEND);
}


//  single
static void drawGymUnitSimple(float x0, float yBase, float W, float H,
    GymScheme scheme, float whiteRatio = 0.04f)
{
    float x1 = x0 + W;
    float hGlass = H * (1.f / 6.f), yGlassTop = yBase + hGlass, yTop = yBase + H;

    // 玻璃层
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(BLD_GLASS[0], BLD_GLASS[1], BLD_GLASS[2], 0.58f);
    glBegin(GL_QUADS); glVertex2f(x0, yBase); glVertex2f(x1, yBase); glVertex2f(x1, yGlassTop); glVertex2f(x0, yGlassTop); glEnd();
    glDisable(GL_BLEND);
    glColor3fv(BLD_GLASS_D);
    glBegin(GL_LINES); glVertex2f(x0, yBase); glVertex2f(x1, yBase); glEnd();
    glBegin(GL_LINES); glVertex2f(x0, yGlassTop); glVertex2f(x1, yGlassTop); glEnd();

    // 主体渐变
    const float* A = (scheme == GymScheme::BLUE_TO_ORANGE) ? GYM_BLUE : GYM_ORANGE;
    const float* B = (scheme == GymScheme::BLUE_TO_ORANGE) ? GYM_ORANGE : GYM_GREEN;
    const float* Wc = GYM_WHITE;

    whiteRatio = clampf(whiteRatio, 0.f, 0.20f);
    float mid = (x0 + x1) * 0.5f, halfW = (x1 - x0) * (whiteRatio * 0.5f);
    float wl = mid - halfW, wr = mid + halfW;

    int slices = 120;
    for (int i = 0; i < slices / 2; i++) {
        float t0 = i / (float)(slices / 2), t1 = (i + 1) / (float)(slices / 2);
        float xa = x0 + (wl - x0) * t0, xb = x0 + (wl - x0) * t1;
        float r0 = lerp(A[0], Wc[0], t0), g0 = lerp(A[1], Wc[1], t0), b0 = lerp(A[2], Wc[2], t0);
        float r1 = lerp(A[0], Wc[0], t1), g1 = lerp(A[1], Wc[1], t1), b1 = lerp(A[2], Wc[2], t1);
        glBegin(GL_QUADS);
        glColor3f(r0, g0, b0); glVertex2f(xa, yGlassTop); glVertex2f(xb, yGlassTop);
        glColor3f(r1, g1, b1); glVertex2f(xb, yTop);       glVertex2f(xa, yTop);
        glEnd();
    }
    glColor3fv(Wc);
    glBegin(GL_QUADS); glVertex2f(wl, yGlassTop); glVertex2f(wr, yGlassTop); glVertex2f(wr, yTop); glVertex2f(wl, yTop); glEnd();

    for (int i = 0; i < slices / 2; i++) {
        float t0 = i / (float)(slices / 2), t1 = (i + 1) / (float)(slices / 2);
        float xa = wr + (x1 - wr) * t0, xb = wr + (x1 - wr) * t1;
        float r0 = lerp(Wc[0], B[0], t0), g0 = lerp(Wc[1], B[1], t0), b0 = lerp(Wc[2], B[2], t0);
        float r1 = lerp(Wc[0], B[0], t1), g1 = lerp(Wc[1], B[1], t1), b1 = lerp(Wc[2], B[2], t1);
        glBegin(GL_QUADS);
        glColor3f(r0, g0, b0); glVertex2f(xa, yGlassTop); glVertex2f(xb, yGlassTop);
        glColor3f(r1, g1, b1); glVertex2f(xb, yTop);       glVertex2f(xa, yTop);
        glEnd();
    }
    // 叠加简单折线彩带
    drawGymVPattern(x0 + 6.f, yGlassTop + 8.f, x1 - 6.f, yTop - 4.f, 40.f, 20.f,18.f, 4.f);

    // 外描边
    glColor3f(0.20f, 0.20f, 0.22f);
    glLineWidth(1.6f);
    glBegin(GL_LINE_LOOP); glVertex2f(x0, yGlassTop); glVertex2f(x1, yGlassTop); glVertex2f(x1, yTop); glVertex2f(x0, yTop); glEnd();
}

// ② whole 完整体育馆：两栋并列（左大右小)
static void drawGymnasiumSimple(float cx, float groundY,
    float totalW, float H,
    float gap = 28.f, float leftScale = 1.25f,
    float rightHScale = 0.90f)
{
    leftScale = std::max(1.05f, leftScale);
    float baseW = (totalW - gap) / (leftScale + 1.f);
    float WL = baseW * leftScale, WR = baseW;

    float xL0 = cx - (WL + gap + WR) * 0.5f;
    float xR0 = xL0 + WL + gap;

    drawGymUnitSimple(xL0, groundY, WL, H * rightHScale*1.2f, GymScheme::BLUE_TO_ORANGE, 0.02f);
    drawGymUnitSimple(xR0, groundY, WR, H * rightHScale, GymScheme::ORANGE_TO_GREEN, 0.02f);
}
void drawCampusScene() {

    drawCampusWideShadowFlat( /*depth=*/32.f, /*alpha=*/0.22f);

    const float LEFT_GROUP_X =  940.f;
    const float MAIN_CENTER_X = gWidth * 0.50f;
    const float RIGHT_CENTER_X = gWidth - 150.f;

    drawLeftBuildingGroup(LEFT_GROUP_X, GROUND_TOP);
    drawCentralMainBuildingFront(MAIN_CENTER_X, GROUND_TOP, 480.f, 180.f);
    drawRightIconicBuildingFront(RIGHT_CENTER_X, GROUND_TOP, 190.f, 190.f);
    drawGymnasiumSimple(gWidth* 0.04f, GROUND_TOP, /*totalW=*/450.f, /*H=*/145.f,
        /*gap=*/20.f, /*leftScale=*/1.25f, /*rightHScale=*/0.70f);

    const float BLUE[3] = { 0.05f, 0.10f, 0.26f };
    const float WHITE[3] = { 1.f, 1.f, 1.f };

    drawStoneMonumentFlex(-150.f , GROUND_TOP - 6.f, 40.f, 50.f, BLUE, WHITE, "GM", WHITE, 30.f, 20.f, 2.0f, false, false, true);
    drawStoneMonumentFlex(RIGHT_CENTER_X - 40.f, GROUND_TOP - 6.f, 40.f, 50.f, BLUE, WHITE, "CB", WHITE,30.f, 20.f, 2.0f, false, false, true);
    drawStoneMonumentFlex(RIGHT_CENTER_X + 165.f, GROUND_TOP - 6.f, 25.f, 38.f, BLUE, WHITE, "SA", WHITE, 30.f, 20.f, 2.0f, false, false, true);
    drawStoneMonumentFlex(RIGHT_CENTER_X + 240.f, GROUND_TOP - 6.f, 25.f, 38.f, BLUE, WHITE, "SB", WHITE, 30.f, 20.f, 2.0f, false, false, true);
    drawStoneMonumentFlex(RIGHT_CENTER_X + 320.f, GROUND_TOP - 6.f, 25.f, 38.f, BLUE, WHITE, "SC", WHITE, 30.f, 20.f, 2.0f, false, false, true);
    drawStoneMonumentFlex(RIGHT_CENTER_X + 400.f, GROUND_TOP - 6.f, 25.f, 38.f, BLUE, WHITE, "SD", WHITE, 30.f, 20.f, 2.0f, false, false, true);


    drawTreeSimple(110.f, GROUND_TOP, 1.6f);
    drawTreeSimple(190.f, GROUND_TOP, 1.8f);
    drawTreeSimple(20.f, GROUND_TOP, 1.6f);
    drawTreeSimple(70.f, GROUND_TOP, 1.3f);
    drawTreeSimple(300.f, GROUND_TOP, 1.8f);
    drawTreeSimple(900.f, GROUND_TOP, 1.1f);
    drawTreeSimple(gWidth * 0.30f, GROUND_TOP, 2.0f);
    drawTreeSimple(gWidth * 1.f, GROUND_TOP, 1.4f);
    drawTreeSimple(gWidth * 0.7f, GROUND_TOP, 1.6f);
    drawTreeSimple(gWidth - 140.f, GROUND_TOP, 1.76f);
}
void drawCampusForeground() {
    drawCampusScene();
    drawSwitchButton(switchX, switchY);
    drawBannerFixedTop();
    drawBalloonsInScreen();
    drawBalloonSimple(500, 320, 1.2f, 1); 
    drawBalloonSimple(600, 340, 0.8f, 2); 
    drawBalloonsOnTop();
}

// ---------------------- Hero & Lantern ---------------------------
void drawHero(float x, float y, float armSwingDeg) {
    glPushMatrix();

    drawSimpleShadowSkewed(x - 20.f, x + 20.f, y - 10.f, 15.f, 0.25f, -7.f);
    glTranslatef(x, y, 0.f);
    // ====== 配色 ======
    const float HEAD_COLOR[3] = { 0.93f, 0.91f, 0.88f };
    const float BODY_COLOR[3] = { 0.88f, 0.86f, 0.83f };
    const float EAR_COLOR[3] = { 0.55f, 0.42f, 0.78f }; 
    const float EYE_COLOR[3] = { 0.10f, 0.08f, 0.10f };
    const float CHEEK_COLOR[3] = { 1.00f, 0.55f, 0.70f }; 

    // ====== 呼吸 + 尾巴动画 ======
    float t = gTime * 3.f;
    float breath = 0.1f * std::sinf(t);
    float tailSwing = 6.0f * std::sinf(t * 1.4f);

    // 尺寸参数
    float headSize = 28.f * (1.f + breath);
    float bodyW = headSize * 0.8f;
    float bodyH = headSize * 0.9f;

    // ====== 身体（左侧） ======
    glPushMatrix();
    glTranslatef(-headSize * 0.55f, -headSize * 0.10f, 0.f);
    glColor3f(BODY_COLOR[0], BODY_COLOR[1], BODY_COLOR[2]);
    drawFilledRect(-bodyW * 0.5f, -bodyH * 0.5f, bodyW * 0.5f, bodyH * 0.5f);
    glPopMatrix();

    // ====== 头部 ======
    glColor3f(HEAD_COLOR[0], HEAD_COLOR[1], HEAD_COLOR[2]);
    drawFilledRect(-headSize * 0.5f, -headSize * 0.5f, headSize * 0.5f, headSize * 0.5f);

    // 耳朵
    glColor3f(EAR_COLOR[0], EAR_COLOR[1], EAR_COLOR[2]);
    drawFilledRect(-headSize * 0.45f, headSize * 0.35f, -headSize * 0.25f, headSize * 0.65f);
    drawFilledRect(headSize * 0.25f, headSize * 0.35f, headSize * 0.45f, headSize * 0.65f);

    // 眼睛与嘴
    glColor3f(EYE_COLOR[0], EYE_COLOR[1], EYE_COLOR[2]);
    drawFilledRect(-headSize * 0.20f, headSize * 0.10f, -headSize * 0.05f, headSize * 0.25f);
    drawFilledRect(headSize * 0.05f, headSize * 0.10f, headSize * 0.20f, headSize * 0.25f);
    drawFilledRect(-headSize * 0.06f, -headSize * 0.05f, headSize * 0.06f, 0.f);

    // 腮红
    glColor3f(CHEEK_COLOR[0], CHEEK_COLOR[1], CHEEK_COLOR[2]);
    drawFilledRect(-headSize * 0.35f, -headSize * 0.10f, -headSize * 0.25f, 0.f);
    drawFilledRect(headSize * 0.25f, -headSize * 0.10f, headSize * 0.35f, 0.f);

    // ====== 尾巴（左侧） ======
    glPushMatrix();
    glTranslatef(-headSize * 0.9f, -headSize * 0.1f, 0.f);
    glRotatef(tailSwing, 0.f, 0.f, 1.f);
    glColor3f(EAR_COLOR[0], EAR_COLOR[1], EAR_COLOR[2]);
    drawFilledRect(0.f, -headSize * 0.06f, -headSize * 0.9f, headSize * 0.06f);
    glPopMatrix();

    // ====== 手臂摆动 ======
    float swing = std::sinf(armSwingDeg * 3.14159f / 180.f);
    glColor3f(0.75f, 0.72f, 0.68f);
    drawFilledRect(-headSize * 0.45f, -headSize * 0.2f + swing * 2.f,
        -headSize * 0.30f, -headSize * 0.05f + swing * 2.f);
    drawFilledRect(headSize * 0.30f, -headSize * 0.2f - swing * 2.f,
        headSize * 0.45f, -headSize * 0.05f - swing * 2.f);

    glPopMatrix();
}

void drawLantern(float x, float y, float angleDeg) {
    glPushMatrix(); glTranslatef(x, y, 0); glRotatef(angleDeg, 0, 0, 1);
    glLineWidth(2.f); glColor3f(0.75f, 0.72f, 0.60f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 28; i++) {
        float t = (2.f * 3.1415926f * i) / 28.f; glVertex2f(10.f * std::cos(t), 14.f + 10.f * std::sin(t));
    }
    glEnd();
    glColor3f(0.7f, 0.68f, 0.62f); drawLine(0.f, 10.f, 0.f, 4.f, 2.f);
    glColor3f(0.18f, 0.18f, 0.18f); drawFilledRect(-8.f, -12.f, 8.f, 6.f);
    glColor3f(0.95f, 0.85f, 0.35f); drawFilledRect(-6.f, -8.f, 6.f, 4.f);
    glPopMatrix();
}

// ---------------------- Fireworks -------------------------------
static inline float randf(float a, float b) {
    return a + (b - a) * (float)(std::rand() / (double)RAND_MAX);
}
static inline void pushP(std::vector<Particle>& v, float x, float y, float vx, float vy, float life, const float c[3]) {
    v.push_back({ x,y,vx,vy,life,c[0],c[1],c[2] });
}

// 尾焰
static inline void emitTrail(const Rocket& rk) {
    const float* col = FW_COLS[std::rand() % FW_COL_N];
    for (int i = 0; i < 6; i++) {
        float ang = randf(-0.9f, 0.9f);
        float sp = randf(20.f, 60.f);
        pushP(gParticles, rk.x, rk.y, sp * ang, randf(-10.f, 20.f), randf(0.20f, 0.40f), col);
    }
}
// 爆炸：亮/深交替 + 白色点
static inline void emitBurst(float cx, float cy, int n) {
    const float* base = FW_COLS[std::rand() % FW_COL_N];

    // 亮色/深色（强化对比，保持饱和）
    auto mk = [](const float* c, float k, float out[3]) {
        out[0] = std::fmin(1.f, c[0] * k);
        out[1] = std::fmin(1.f, c[1] * k);
        out[2] = std::fmin(1.f, c[2] * k);
        };
    float Cb[3], Cd[3];
    mk(base, 1.18f, Cb);   // 亮
    mk(base, 0.55f, Cd);   // 深
    static const float WHITE[3] = { 1.f,1.f,1.f };

    for (int i = 0; i < n; i++) {
        float a = (2.f * 3.1415926f * i) / n;
        float sp = randf(170.f, 260.f);           // 稍大、稍快
        float vx = sp * std::cos(a), vy = sp * std::sin(a);
        float life = randf(1.0f, 1.7f);

        // 交替使用亮/深两色
        const float* col = (i & 1) ? Cb : Cd;
        pushP(gParticles, cx, cy, vx, vy, life, col);

        // 亮芯：更快、更短
        if (i % 4 == 0) {
            pushP(gParticles, cx, cy, 1.18f * vx, 1.18f * vy, life * 0.9f, col);
        }

        if (i % 4 == 0) {                           
            float wa = a + randf(-0.10f, 0.10f);
            float wsp = randf(220.f, 360.f);       
            float wl = randf(0.6f, 1.0f);         
            pushP(gParticles, cx, cy,
                wsp * std::cos(wa),
                wsp * std::sin(wa),
                wl, WHITE);
        }
    }

    for (int k = 0; k < 35; k++) {              
        pushP(gParticles, cx, cy,
            randf(-160.f, 160.f),
            randf(-160.f, 160.f),
            randf(0.35f, 0.70f),            
            WHITE);
    }
}

static inline void launchRocket(float x, float targetY, float vx, float vy) {
    gRockets.push_back({ x,90.f,vx,vy,targetY,false,0.f });
}

void spawnFireworkBurst(float x, float y, int n = 6) {
    for (int i = 0; i < n; i++) {
        float cx = gWidth * 0.5f + gCamPan;
        float cy = gHeight * 0.5f + gCamTopLift;
        float halfW = gWidth * 0.5f / gCamZoom;
        float halfH = gHeight * 0.5f / gCamZoom;
        float L = cx - halfW, R = cx + halfW;

        float tx = randf(L + 60.f, R - 60.f);
        float ty = randf(cy + 0.05f * halfH, cy + 0.55f * halfH);

        launchRocket(tx, ty, randf(-18.f, 18.f), randf(280.f, 340.f));
    }
}

// 更新所有烟花
void updateParticles(float dt) {
    // 1) 周期性发射
    static float acc = 0.f;
    acc += FIREWORK_RATE * dt;
    while (acc >= 1.f) {
        acc -= 1.f;

        // 计算当前相机正交视口
        float cx = gWidth * 0.5f + gCamPan;
        float cy = gHeight * 0.5f + gCamTopLift;
        float halfW = gWidth * 0.5f / gCamZoom;
        float halfH = gHeight * 0.5f / gCamZoom;
        float L = cx - halfW, R = cx + halfW;

        // 每次来两枚
        for (int k = 0; k < 2; ++k) {
            float baseX = randf(L + 50.f, R - 50.f);
            float tgtY = randf(cy + 0.05f * halfH, cy + 0.58f * halfH); // 在视口上半区域
            launchRocket(baseX, tgtY, randf(-14.f, 14.f), randf(290.f, 350.f));
        }
    }

    // 2) 上升 & 尾焰 & 触顶爆
    for (auto& rk : gRockets) {
        if (rk.burst) continue;
        rk.vy += GRAVITY_FUSE * dt; 
        rk.x += rk.vx * dt; rk.y += rk.vy * dt;

        rk.trailAcc += dt;
        if (rk.trailAcc > 0.015f) { emitTrail(rk); rk.trailAcc = 0.f; }

        if (rk.y >= rk.targetY || rk.vy <= 0.f) {
            rk.burst = true;
            emitBurst(rk.x, rk.y, 110);
        }
    }
    // 清理已爆
    gRockets.erase(std::remove_if(gRockets.begin(), gRockets.end(),
        [](const Rocket& r) { return r.burst; }), gRockets.end());

    // 3) 尾焰粒子推进（不下坠，只淡出/微动）
    for (auto& p : gParticles) { p.life -= dt; p.x += p.vx * dt; p.y += p.vy * dt; p.vy += GRAVITY_BURST * dt; }
    gParticles.erase(std::remove_if(gParticles.begin(), gParticles.end(),
        [](const Particle& p) { return p.life <= 0.f; }), gParticles.end());
    if ((int)gParticles.size() > PARTICLE_MAX)
        gParticles.erase(gParticles.begin(), gParticles.begin() + ((int)gParticles.size() - PARTICLE_MAX));

}
// 画粒子（
static inline float clampf01(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
void drawParticles() {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 先画彩点
    glPointSize(3.3f);
    glBegin(GL_POINTS);
    for (const auto& p : gParticles) {
        const bool isWhite = (p.r > 0.95f && p.g > 0.95f && p.b > 0.95f);
        if (isWhite) continue;
        float a = (p.life < 0.f ? 0.f : (p.life > 1.f ? 1.f : p.life));
        glColor4f(p.r, p.g, p.b, a);
        glVertex2f(p.x, p.y);
    }
    glEnd();

    // 再画白点
    glPointSize(2.8f);
    glBegin(GL_POINTS);
    for (const auto& p : gParticles) {
        if (!(p.r > 0.95f && p.g > 0.95f && p.b > 0.95f)) continue;
        float blink = 0.8f + 0.2f * std::sin(gTime * 12.f + p.x * 0.01f);
        glColor4f(1.f, 1.f, 1.f, blink);
        glVertex2f(p.x, p.y);
    }
    glEnd();

    glDisable(GL_BLEND);
}


// ---------- Text Utilities --------------------------------------
static int bitmapTextWidth(void* font, const std::string& s) {
    int w = 0; for (unsigned char c : s) w += glutBitmapWidth(font, c); return w;
}
// 居中显示提示文字，可自定义纵向位置（默认偏上）
// 参数：s=文本；r,g,b=颜色；optY=纵向位置（像素，默认 gHeight*0.75f）
static void drawHintTopCenter(const std::string& s,
    float r = 1.0f, float g = 1.0f, float b = 1.0f,
    float optY = -1.0f)
{
    void* font = GLUT_BITMAP_TIMES_ROMAN_24;
    int tw = bitmapTextWidth(font, s);
    float x = (gWidth - tw) * 0.5f;
    float y = (optY < 0.f ? gHeight * 0.70f : optY); // 默认靠上 1/4 高度

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (unsigned char c : s) glutBitmapCharacter(font, c);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}



// “Thank you!” 对话框（白底黑框/黑字），在小猫头上方，轻微跳动，2 秒
static void drawThankBubbleAboveHero(float hx, float hy) {
    const char* text = "Thank you(*^><^*)~";
    void* font = GLUT_BITMAP_HELVETICA_18;

    // 文本尺寸与气泡框
    int tw = bitmapTextWidth(font, text);
    int th = 18; 
    float padX = 10.f, padY = 6.f;
    float bw = tw + padX * 2.f;
    float bh = th + padY * 2.f;

    // 位置：头顶上方一点
    float cx = hx;              
    float cy = hy + 45.f;      
    float wob = 4.f * std::sinf(gTime * 10.f);  
    cy += wob;

    // 屏幕坐标绘制
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    // 气泡矩形（白底黑边）
    float x0 = cx - bw * 0.5f, x1 = cx + bw * 0.5f;
    float y0 = cy - bh * 0.5f, y1 = cy + bh * 0.5f;

    glColor3f(1, 1, 1);
    drawFilledRect(x0, y0, x1, y1);
    glColor3f(0, 0, 0);
    glLineWidth(1.8f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();

    // 气泡尾巴（三角）  
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - 6.f, y0);
    glVertex2f(cx + 6.f, y0);
    glVertex2f(cx + 2.f, y0 - 10.f);
    glEnd();

    // 文本（黑色）
    glColor3f(0.70f, 0.35f, 0.90f);
    glRasterPos2f(x0 + padX, y0 + padY + 2.f);
    for (const char* p = text; *p; ++p) glutBitmapCharacter(font, *p);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


// ====== 简化且可复用：描边 + 正文 的矢量文字（水平或上拱） ======
static float strokeWidthROMAN_simple(const char* s) {
    float w = 0.f;
    for (const char* p = s; *p; ++p) w += glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
    return w;
}

// 按给定起点/缩放绘制一行；arcH>0 表示上拱高度（像素），=0 水平
static void drawStrokeLineInternal_simple(const char* s, float scale, float totalW, float arcH) {
    float x = 0.f;
    const float cxPx = totalW * 0.5f;   // 行宽的一半（像素）
    for (const char* p = s; *p; ++p) {
        float cw = glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
        float xCenterPx = (x + cw * 0.5f) * scale;     // 当前字符中心（像素）
        float lift = 0.f;
        if (arcH > 0.f) {
            float t = (xCenterPx - cxPx) / cxPx;     
            lift = arcH * (1.f - t * t);               
        }
        glPushMatrix();
        glTranslatef(x, lift / scale, 0);
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
        glPopMatrix();
        x += cw;
    }
}

// 居中绘制一行（带描边）：cx, y 为屏幕像素坐标；pixelH 字高；arcH 上拱像素高度
static void drawStrokeOutlinedLineCentered_simple(const char* s, float cx, float y,
    float pixelH, float arcH,
    float cr, float cg, float cb,     // 正文字色
    float br, float bg, float bb,     // 描边色
    float lwMain, float lwOutline)  // 主/描边线宽
{
    float scale = pixelH / 119.f;                  // GLUT_STROKE_ROMAN 高约 119
    float totalW = strokeWidthROMAN_simple(s) * scale;
    float xLeft = cx - totalW * 0.5f;

    // 进入屏幕坐标系
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glTranslatef(xLeft, y, 0);
    glScalef(scale, scale, 1);

    // 先描边（粗）
    glColor3f(br, bg, bb);
    glLineWidth(lwOutline);
    drawStrokeLineInternal_simple(s, scale, totalW, arcH);

    // 再正文（细）
    glColor3f(cr, cg, cb);
    glLineWidth(lwMain);
    drawStrokeLineInternal_simple(s, scale, totalW, arcH);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 两行庆祝标题（第一行上拱，第二行水平）
static void drawCelebrationBanner() {
    const char* L1 = "Happy 20th Anniversary";
    const char* L2 = "XJTLU";

    // 位置
    float y1 = gHeight * 0.71f;   // 第一行（上拱）
    float y2 = y1 - 80.f;         // 第二行（水平）

    // 尺寸/拱度
    float size1 = 50.f, arc1 = 50.f;
    float size2 = 80.f;

    // 颜色（正文 + 描边）与线宽
    float r1 = 1.0f, g1 = 0.85f, b1 = 0.25f;   
    float r2 = 0.85f, g2 = 0.7f, b2 = 1.0f;   
    float br = 0.70f, bg = 0.55f, bb = 0.9f; 
    float wr = 0.05f, wg = 0.05f, wb = 0.075f;    
    // 线宽
    float lwOutline = 8.0f;  
    float lwMain = 5.f;    
    float lwWhite = 13.0f;  
    // ===== 第一行（上拱） =====
 
    drawStrokeOutlinedLineCentered_simple(L1, gWidth * 0.5f, y1,
        size1, arc1,
       r1, g1, b1,
         wr, wg, wb,
       lwMain, lwWhite);
 
    drawStrokeOutlinedLineCentered_simple(L1, gWidth * 0.5f, y1,
        size1, arc1,
        r1, g1, b1,
         1.00f, 0.45f, 0.00f,
       lwMain, lwOutline);

    // ===== 第二行（水平） =====
    drawStrokeOutlinedLineCentered_simple(L2, gWidth * 0.5f, y2,
        size2, 0.f,
       r2, g2, b2,
         wr, wg, wb,
         lwMain, lwWhite);

    drawStrokeOutlinedLineCentered_simple(L2, gWidth * 0.5f, y2,
        size2, 0.f,
        r2, g2, b2,
         br, bg, bb,
        lwMain, lwOutline);
}


// ----------- 封面底部：20th❤  -----------
static void drawCoverPixelTitle_Trapezoid()
{
    const float PINK[3] = { 1.0f, 0.45f, 0.65f };
    const float cellX = 10.0f;   // 横向像素宽
    const float cellY = 18.0f;   // 纵向像素高
    const float gapX = 6.7f;       // 字间距（可调）
    const float baseY = gHeight * 0.03f; // 放在屏幕下方 1/3 区域

    // 5x7 / 8x7 位图
    const std::vector<std::string> G2 = {
        "01110","10001","00001","00010","00100","01000","11111"
    };
    const std::vector<std::string> G0 = {
        "01110","10001","10001","10001","10001","10001","01110"
    };
    const std::vector<std::string> GT = {
        "11111","00100","00100","00100","00100","00100","00100"
    };
    const std::vector<std::string> GH = {
        "10001","10001","10001","11111","10001","10001","10001"
    };
    const std::vector<std::string> HEART = {
        "01100110",
        "11111111",
        "11111111",
        "11111111",
        "01111110",
        "00111100",
        "00011000"
    };

    auto wpx = [&](const std::vector<std::string>& m) { return (float)m[0].size() * cellX; };

    const int rows = 7;
    float sp = gapX;

    // 计算整行总宽，便于居中 & 行中心
    float totalW = wpx(G2) + sp + wpx(G0) + sp + wpx(GT) + sp + wpx(GH) + sp * 2 + wpx(HEART);
    float xStart = (gWidth - totalW) * 0.50f;
    float xCenter = xStart + totalW * 0.50f;   // 每一行围绕这个中心做横向缩放

    // —— 等腰梯形参数——
    float scaleTop = 2.5f;   // 顶部行的横向缩放（<1 稍窄）
    float scaleBottom = 3.f;   // 底部行的横向缩放（>1 更宽）

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor3f(PINK[0], PINK[1], PINK[2]);

    float x = xStart;

    auto drawGlyphTrapezoid = [&](const std::vector<std::string>& M, float X) {
        int cols = (int)M[0].size();
        for (int r = 0; r < rows; ++r) {
            // r=0 顶行，r=rows-1 底行
            float t = r / float(rows - 1);                 
            float s = scaleTop + (scaleBottom - scaleTop) * t; 

            float y0 = baseY + (rows - 1 - r) * cellY;
            float y1 = y0 + cellY;

            for (int c = 0; c < cols; ++c) {
                if (M[r][c] != '1') continue;

                float x0 = X + c * cellX;
                float x1 = x0 + cellX;

                // 围绕整段标题的中心做等比放大/缩小（不产生斜切）
                float X0 = xCenter + (x0 - xCenter) * s;
                float X1 = xCenter + (x1 - xCenter) * s;

                drawFilledRect(X0, y0, X1, y1);
            }
        }
        };

    drawGlyphTrapezoid(G2, x); x += wpx(G2) + sp;
    drawGlyphTrapezoid(G0, x); x += wpx(G0) + sp;
    drawGlyphTrapezoid(GT, x); x += wpx(GT) + sp;
    drawGlyphTrapezoid(GH, x); x += wpx(GH) + sp * 2;
    drawGlyphTrapezoid(HEART, x);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 封面背景：淡黄色底 + 亮紫星星
static void drawCoverBackgroundRadial()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    // 背景底色：淡黄色
    glColor3f(1.0f, 0.97f, 0.85f);
    drawFilledRect(0, 0, (float)gWidth, (float)gHeight);

    // 星星颜色与线宽
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(0.75f, 0.50f, 0.95f);  // 亮紫
    glLineWidth(2.f);

    // 上方三分之二区域
    float yMax = gHeight * 0.95f;

    // 星星点位与基准尺寸
    const float stars[9][3] = {
        { gWidth * 0.15f, yMax * 0.90f, 10.f },
        { gWidth * 0.35f, yMax * 0.60f, 15.f },
        { gWidth * 0.55f, yMax * 0.88f, 12.f },
        { gWidth * 0.72f, yMax * 0.70f, 16.f },
        { gWidth * 0.85f, yMax * 0.82f, 20.f },
        { gWidth * 0.3f, yMax * 0.8f, 10.f },
        { gWidth * 0.90f, yMax * 0.50f, 12.f },
        { gWidth * 0.5f, yMax * 0.65f, 14.f },
        { gWidth * 0.10f, yMax * 0.45f, 18.f },
    };

    // 画一组虚线（从中心沿某方向向外）
    auto dashRays = [&](float cx, float cy, float R) {
        const int   count = 1;
        const float segLen = R * 0.15f;
        const float gap = R * 0.10f;
        const float start = R * 1.05f;

        auto ray = [&](float dx, float dy) {
            for (int i = 0; i < count; ++i) {
                float a0 = start + i * (segLen + gap);
                float a1 = a0 + segLen;
                glBegin(GL_LINES);
                glVertex2f(cx + dx * a0, cy + dy * a0);
                glVertex2f(cx + dx * a1, cy + dy * a1);
                glEnd();
            }
            };

        const float inv = 0.7071067f; 
        ray(0.f, 1.f);   // 上
        ray(0.f, -1.f);   // 下
        ray(1.f, 0.f);   // 右
        ray(-1.f, 0.f);   // 左
        ray(-inv, inv);  // 左上
        ray(inv, -inv);  // 右下
        ray(inv, inv);  // 右上
        ray(-inv, -inv);  // 左下
        };

    for (auto& s : stars) {
        float x0 = s[0], y0 = s[1], Rbase = s[2];
        // 呼吸动画
        float osc = 1.0f + 0.04f * std::sinf(gTime * 1.0f + x0 * 0.01f + y0 * 0.02f);
        float R = Rbase * osc * 1.2f;

        // === 主星线：竖 > 横 > 斜 ===
        // 竖线（最长）
        glBegin(GL_LINES);
        glVertex2f(x0, y0 - R * 1.2f);
        glVertex2f(x0, y0 + R * 1.2f);
        glEnd();

        // 横线（中等长度）
        glBegin(GL_LINES);
        glVertex2f(x0 - R * 0.8f, y0);
        glVertex2f(x0 + R * 0.8f, y0);
        glEnd();

        // 左上→右下（稍短）
        glBegin(GL_LINES);
        glVertex2f(x0 - R * 0.65f, y0 + R * 0.65f);
        glVertex2f(x0 + R * 0.65f, y0 - R * 0.65f);
        glEnd();

        // 右上→左下（稍短）
        glBegin(GL_LINES);
        glVertex2f(x0 + R * 0.6f, y0 + R * 0.6f);
        glVertex2f(x0 - R * 0.6f, y0 - R * 0.6f);
        glEnd();

        // 外沿虚线闪光
        dashRays(x0, y0, R);
    }

    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 朝右的小鱼干鼠标（灰绿色）
static void drawFishCursorAtMouse(float scale = 1.0f)
{
    // 固定屏幕坐标
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    const float cx = gMouseX, cy = gMouseY;
    const float bodyL = 30.f * scale;     // 鱼身长度
    const float bodyH = 15.f * scale;     // 鱼身高度
    const float tailL = 12.f * scale;     // 尾巴长度
    const float eyeR = 3.3f * scale;      // 眼睛“×”大小

    // --- 鱼身（椭圆）---
    glColor3f(0.60f, 0.78f, 0.65f);       // 灰绿色
    drawEllipseFan(cx, cy, bodyL * 0.5f, bodyH * 0.5f, 48);

    // --- 鱼尾（右侧三角形，朝右）---
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + bodyL * 0.5f, cy);                    // 接身体右端
    glVertex2f(cx + bodyL * 0.5f + tailL, cy + bodyH * 0.5f); // 上角
    glVertex2f(cx + bodyL * 0.5f + tailL, cy - bodyH * 0.5f); // 下角
    glEnd();

    // --- 眼睛---
    glLineWidth(2.f);
    glColor3f(0.12f, 0.15f, 0.18f);
    const float ex = cx - bodyL * 0.25f, ey = cy + 1.5f * scale; // 左侧眼睛
    drawLine(ex - eyeR, ey - eyeR, ex + eyeR, ey + eyeR);
    drawLine(ex - eyeR, ey + eyeR, ex + eyeR, ey - eyeR);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


// 封面整合：背景 + 小猫水平来回 + 气球 + 字幕
static void drawCoverScene()
{
    // 背景
    drawCoverBackgroundRadial();
    drawCoverPixelTitle_Trapezoid();
    drawBannerFixedTop();
    // 小猫自动行走（范围 = 屏幕的一半，以中心为轴）

    float y = gHeight * 0.3f;

    // 翻转朝向：向右 dir=1，向左 dir=-1（通过 X 轴缩放）
    glPushMatrix();
    glTranslatef(gCoverCatX, y, 0.f);
    glScalef((gCoverCatDir >= 0 ? 1.f : -1.f), 1.f, 1.f);
    glTranslatef(-gCoverCatX, -y, 0.f);

    float armDeg = 12.f * std::sinf(gTime * 4.2f);
    drawHero(gCoverCatX, y, armDeg);
    glPopMatrix();

    //漂浮气球
    drawBalloonsInScreen();   
    drawBalloonSimple(500, 320, 1.2f, 1); 
    drawBalloonSimple(600, 340, 0.8f, 5); 
    drawBalloonSimple(800, 330, 1.f, 0); 
    drawBalloonSimple(900, 300, 0.8f, 5); 

    gCoverPromptShown = true;
    drawHintTopCenter("Left-click to give the fish to the kitty  ~ !", 1.0f, 0.35f, 0.55f); // 玫粉色
    drawFishCursorAtMouse(1.0f);

 }

// ---------------------- Soft Lamp (5 rings) ----------------------
void writeLampToStencil5(float x, float y, float r0, float r1, float r2, float r3, float r4) {
    glEnable(GL_STENCIL_TEST);
    glClearStencil(0); glClear(GL_STENCIL_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF); glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); drawCircleFan(x, y, r4, 96);
    glStencilFunc(GL_ALWAYS, 2, 0xFF); drawCircleFan(x, y, r3, 96);
    glStencilFunc(GL_ALWAYS, 3, 0xFF); drawCircleFan(x, y, r2, 96);
    glStencilFunc(GL_ALWAYS, 4, 0xFF); drawCircleFan(x, y, r1, 96);
    glStencilFunc(GL_ALWAYS, 5, 0xFF); drawCircleFan(x, y, r0, 96);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}
void drawBlackCoversSoft5() {
    double L, R, B, T; currentOrthoBounds(L, R, B, T);

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glStencilFunc(GL_EQUAL, 0, 0xFF); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColor4f(0, 0, 0, BLACK_ALPHA_BASE0 * gGlobalFade); drawFilledRect((float)L, (float)B, (float)R, (float)T);

    glStencilFunc(GL_EQUAL, 1, 0xFF); glColor4f(0, 0, 0, BLACK_ALPHA_BASE1 * gGlobalFade); drawFilledRect((float)L, (float)B, (float)R, (float)T);
    glStencilFunc(GL_EQUAL, 2, 0xFF); glColor4f(0, 0, 0, BLACK_ALPHA_BASE2 * gGlobalFade); drawFilledRect((float)L, (float)B, (float)R, (float)T);
    glStencilFunc(GL_EQUAL, 3, 0xFF); glColor4f(0, 0, 0, BLACK_ALPHA_BASE3 * gGlobalFade); drawFilledRect((float)L, (float)B, (float)R, (float)T);
    glStencilFunc(GL_EQUAL, 4, 0xFF); glColor4f(0, 0, 0, BLACK_ALPHA_BASE4 * gGlobalFade); drawFilledRect((float)L, (float)B, (float)R, (float)T);

    glDisable(GL_BLEND); glDisable(GL_STENCIL_TEST);
}

// ---------------------- Camera / Projection ----------------------
void setOrtho() {
    float cx = gWidth * 0.5f + gCamPan;
    float cy = gHeight * 0.5f + gCamTopLift;
    float halfW = gWidth * 0.5f / gCamZoom;
    float halfH = gHeight * 0.5f / gCamZoom;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(cx - halfW, cx + halfW, cy - halfH, cy + halfH, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

// ---------------------- UI -----------------------------
void drawBitmapText(float x, float y, const char* s, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y); for (const char* p = s; *p; ++p) glutBitmapCharacter(font, *p);
}
void drawPromptBar(const std::string& s) {
    const float marginX = 16.f, marginY = 16.f;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    void* cuteFont = GLUT_BITMAP_TIMES_ROMAN_24;

    const float x = marginX, y = marginY + 6.f;
    auto draw = [&](float dx, float dy, float r, float g, float b, float a) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(r, g, b, a);
        glRasterPos2f(x + dx, y + dy);
        for (unsigned char c : s) glutBitmapCharacter(cuteFont, c);
        glDisable(GL_BLEND);
        };
    draw(1.5f, -1.5f, 0.05f, 0.07f, 0.18f, 0.85f);
    draw(0.f, 0.f, 0.98f, 0.96f, 0.92f, 1.00f);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
void drawGuideStrip(float x0, float y0, float x1, float y1) {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.9f, 0.8f, 0.2f, 0.15f); drawLine(x0, y0, x1, y1, GUIDE_STRIP_WIDTH);
    glDisable(GL_BLEND);
}

// ---------------------- Update Logic -----------------------------
void updateHero(float dt) {
    if (gState == GameState::GUIDING || gState == GameState::AT_SWITCH) {
        float vx = 0.f, vy = 0.f;
        if (keyL['a'] || keyS[GLUT_KEY_LEFT])  vx -= 1.f;
        if (keyL['d'] || keyS[GLUT_KEY_RIGHT]) vx += 1.f;
        if (keyL['w'] || keyS[GLUT_KEY_UP])    vy += 1.f;
        if (keyL['s'] || keyS[GLUT_KEY_DOWN])  vy -= 1.f;
        float len = std::sqrt(vx * vx + vy * vy); if (len > 0) { vx /= len; vy /= len; }
        heroX += vx * HERO_SPEED * dt;
        heroY += vy * HERO_SPEED * dt;

        heroX = clampf(heroX, 30.f, (float)gWidth - 30.f);
        heroY = clampf(heroY, 36.f, 130.f);

        if (gState == GameState::GUIDING && dist2(heroX, heroY, switchX, switchY) <= SWITCH_RADIUS * SWITCH_RADIUS)
            gState = GameState::AT_SWITCH;
    }
}
void updateStateMachine() {
    if (gState == GameState::SEARCHING) {
        if (dist2(gMouseX, gMouseY, heroX, heroY) <= FIND_RADIUS * FIND_RADIUS) {
            gState = GameState::FOUND; heroHasLamp = true; tFound = 0.f;
            gThankBubbleActive = true;
            gThankBubbleStart = gTime;
        }
    }
    else if (gState == GameState::FOUND) {
        tFound += gDt; heroArmPhase = std::sinf(tFound * 6.28318f);
        if (tFound >= FOUND_TO_GUIDING_TIME) { gState = GameState::GUIDING; heroArmPhase = 0.f; }
    }
    else if (gState == GameState::CELEBRATE) {
        gCamTopLift = std::min(CAMERA_SHAKE_UP, gCamTopLift + CAMERA_SPEED_LIFT * gDt);
        gCamZoom = std::max(CAMERA_ZOOM_TARGET, gCamZoom - CAMERA_SPEED_ZOOM * gDt);
        gCamPan = std::min(CAMERA_PAN_X, gCamPan + CAMERA_SPEED_PAN * gDt);
        gGlobalFade = std::max(0.f, gGlobalFade - 0.45f * gDt);

        if (gGlobalFade <= 0.02f) gState = GameState::DONE;
    }
    // —— 对话框 2 秒后自动关闭 ——
    if (gThankBubbleActive && (gTime - gThankBubbleStart) > 2.0f) {
        gThankBubbleActive = false;
    }
}

// ---------------------- Display -----------------------------
void display() {
    setOrtho();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // --------渲染封面 --------
    if (gStage == AppStage::COVER) {
        // 画封面
        drawCoverScene();

        if (gCoverTransiting) {
            // t ∈ [0,1.5]，前 50% 变黑，后 50% 切换到主界面再淡出
            float t = gCoverTransT;
            float half = COVER_TRANS_TOTAL * 0.5f;
            float alpha = 0.f;

            if (t < half) {
                // 0 → 1 变黑
                alpha = t / half;
            }
            else {
                // 首次到达一半时，切换到主界面
                static bool switched = false;
                if (!switched) {
                    gStage = AppStage::MAIN;
                    switched = true;
                }
                alpha = 1.f - (t - half) / (COVER_TRANS_TOTAL - half);
            }

            // 覆盖全屏的黑色渐变
            glMatrixMode(GL_PROJECTION);
            glPushMatrix(); glLoadIdentity(); glOrtho(0, gWidth, 0, gHeight, -1, 1);
            glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0, 0, 0, clampf(alpha, 0.f, 1.f));
            drawFilledRect(0, 0, (float)gWidth, (float)gHeight);
            glDisable(GL_BLEND);

            glPopMatrix();
            glMatrixMode(GL_PROJECTION); glPopMatrix();
            glMatrixMode(GL_MODELVIEW);

        }

        glutSwapBuffers();
        return;
    }
    else {
     

        drawExtendedBackdrop();
        drawCloudsClustersFlat();

        static float SCENE_SPAN = 1800.f; 
        drawCampusForeground();
        glPushMatrix(); glTranslatef(-SCENE_SPAN, 0, 0); drawCampusScene(); glPopMatrix();
        glPushMatrix(); glTranslatef(SCENE_SPAN, 0, 0);  drawCampusScene(); glPopMatrix();

        float armDeg = 0.f;
        if (gState == GameState::FOUND) {
            float k = easeInOut(tFound / FOUND_TO_GUIDING_TIME);
            armDeg = ARM_SWING_DEG * (0.5f * std::sinf(k * 6.28318f));
        }
        drawHero(heroX, heroY, armDeg);

        drawBalloonEdgeSet();
        drawBalloonsInScreen();   // 屏幕中上方几只漂浮气球

        float armRad = armDeg * 3.1415926f / 180.f;
        float handX = heroX + 16.f * std::cos(armRad);
        float handY = heroY - 2.f + 16.f * std::sin(armRad) * 0.25f;

        float sway = 6.f * std::sinf(gTime * 2.5f);
        if (gState == GameState::SEARCHING) drawLantern(gMouseX, gMouseY, sway);
        else                                 drawLantern(handX, handY, sway);

        float lampX = (gState == GameState::SEARCHING ? gMouseX : handX);
        float lampY = (gState == GameState::SEARCHING ? gMouseY : handY);

        float lampRBase = (gState == GameState::CELEBRATE || gState == GameState::DONE)
            ? std::max(LAMP_RADIUS_BASE, CELEBRATE_MIN_LAMP) : LAMP_RADIUS_BASE;

        float r0 = lampRBase;
        float r1 = r0 + LAMP_SOFT_DELTA1, r2 = r0 + LAMP_SOFT_DELTA2, r3 = r0 + LAMP_SOFT_DELTA3, r4 = r0 + LAMP_SOFT_DELTA4;

        writeLampToStencil5(lampX, lampY, r0, r1, r2, r3, r4);

        drawBlackCoversSoft5();

        drawConfettiOverlay();
        drawConfettiOverlay();
        drawConfettiOverlay();
        drawConfettiOverlay();

        if (gState == GameState::CELEBRATE || gState == GameState::DONE)
            drawParticles();

        // --------- 提示逻辑 ----------
        // 1) 搜索阶段：立刻显示第一句提示（居中靠上，白字）
        if (gState == GameState::SEARCHING) {
            drawHintTopCenter("Move the mouse to find the kitty ~ !");
        }

        // 2) 刚找到：显示 2 秒 Thank you 对话框，期间不显示普通提示
        if (gThankBubbleActive) {
           
            drawThankBubbleAboveHero(heroX, heroY);
        }
        else {
            // 3) 之后继续显示后续 hint（居中靠上，白字）
            if (gState == GameState::GUIDING) {
                drawHintTopCenter("Use W/A/S/D or Arrow Keys to reach the switch ~");
            }
            else if (gState == GameState::AT_SWITCH) {
                drawHintTopCenter("Press SPACE to turn on the switch ~");
            }
        }

        if (gState == GameState::CELEBRATE || gState == GameState::DONE) {
            float dt = gTime - gCelebrateStartTime;

            if (dt > 0.5f) {
                // 计算淡入透明度：1~2秒之间从0→1，之后保持1
                float alpha = std::min((dt - 1.0f) / 4.0f, 0.8f);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(1, 1, 1, alpha);     // 控制整体透明度
                drawCelebrationBanner();
                glDisable(GL_BLEND);
            }
            // ---- 放烟花 3 秒后显示“按Q退出”字幕 ----
            float sinceFire = gTime - gCelebrateStartTime;
            if (sinceFire > 5.0f) {
                drawHintTopCenter("Press Q to exit~", 1.0f, 0.85f, 0.95f, gHeight * 0.05f);
            }
        }

    }
    glutSwapBuffers();
}

void onIdle() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (gLastTicks == 0) gLastTicks = now;
    gDt = (now - gLastTicks) / 1000.f; gLastTicks = now; gTime += gDt;

    updateHero(gDt);
    updateStateMachine();
    // ---- 封面阶段的更新 ----
    if (gStage == AppStage::COVER) {
        static bool inited = false;
        if (!inited) {
            // init once
            gCoverCatX = gWidth * 0.5f;   
            gCoverCatSpeed = gCoverCatSpeedBase;
            inited = true;
        }

        // 小猫水平往返
        float leftX = gWidth * 0.10f;
        float rightX = gWidth * 0.90f;
        gCoverCatX += gCoverCatDir * gCoverCatSpeed * gDt;
        if (gCoverCatX > rightX) { gCoverCatX = rightX; gCoverCatDir = -1; }
        if (gCoverCatX < leftX) { gCoverCatX = leftX;  gCoverCatDir = 1; }
        // ---- 猫猫吃到鱼干就停下 ----
        float dx = gMouseX - gCoverCatX;
        float dy = (gHeight * 0.3f) - gMouseY;   // 猫的 y = gHeight*0.3f
        float dist2 = dx * dx + dy * dy;
        if (dist2 < 40.f * 40.f) {               // 距离 < 40 像素 → 视为吃到
            gCoverCatSpeed = 0.f;                // 停止移动
            gCoverCatDir = (dx >= 0 ? 1 : -1);   // 朝向鱼干方向
        }
        // 进入暂停：仅当未处于暂停且距离阈值内
        if (!gCoverCatPaused && dist2 < 40.f * 40.f) {
            gCoverCatPaused = true;
            gCoverCatPauseStart = gTime;
            gCoverCatSpeed = 0.f;                   // 停
            gCoverCatDir = (dx >= 0 ? 1 : -1);      // 朝向鱼干
        }

        // 退出暂停：到时自动恢复
        if (gCoverCatPaused && (gTime - gCoverCatPauseStart) >= COVER_CAT_PAUSE_DUR) {
            gCoverCatPaused = false;
            gCoverCatSpeed = gCoverCatSpeedBase;    // 复位速度
            // 恢复后按既有左右边界逻辑继续往返
        }
        // 正在做黑场转场：时间轴推进
        if (gCoverTransiting) {
            gCoverTransT += gDt;
            if (gCoverTransT >= COVER_TRANS_TOTAL) {
                gCoverTransiting = false;
                gCoverTransT = COVER_TRANS_TOTAL;
            }
        }
    }

    if (gState == GameState::CELEBRATE || gState == GameState::DONE) {
        updateParticles(gDt);
    }
    glutPostRedisplay();
}



// ---------------------- Input -------------------------------
void onPassiveMotion(int x, int y) { gMouseX = (float)x; gMouseY = (float)(gHeight - y); }

void onKeyboard(unsigned char key, int, int) {
    keyL[key] = true;

    if (gState == GameState::AT_SWITCH && key == ' ') {
        gState = GameState::CELEBRATE;
        gCelebrateStartTime = gTime;  // 记录进入庆祝的时刻

        std::srand((unsigned)glutGet(GLUT_ELAPSED_TIME));
        gRainbowT = 0.f;
    }
    if (key == 'q' || key == 'Q') exit(0);  // ← 按 Q 退出程序

}
void onMouse(int button, int state, int mx, int my)
{
    // 把屏幕坐标转成 OpenGL 坐标（左下角为原点）
    float x = (float)mx;
    float y = (float)(gHeight - my);

    // ---- 封面阶段点击 ----
    if (gStage == AppStage::COVER && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // 猫猫中心和半径
        float catX = gCoverCatX;     // 猫猫当前中心
        float catY = gHeight * 0.3f;
        float R = 24.0f;             // 猫猫的碰撞半径

        // 如果鼠标在猫猫区域内
        float dx = x - catX, dy = y - catY;
        if (dx * dx + dy * dy <= R * R && gCoverPromptShown && !gCoverTransiting) {
            gCoverTransiting = true;
            gCoverTransT = 0.f;
        }
    }
}


void onKeyboardUp(unsigned char key, int, int) { keyL[key] = false; }
void onSpecial(int key, int, int) { keyS[key] = true; }
void onSpecialUp(int key, int, int) { keyS[key] = false; }

// ---------------------- Reshape -----------------------------
void onReshape(int w, int h) {
    gWidth = (w > 1) ? w : 1; gHeight = (h > 1) ? h : 1;
    glViewport(0, 0, gWidth, gHeight);
    setOrtho();
}

// ---------------------- Main -------------------------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_STENCIL);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 60);
    glutCreateWindow("CPT205 A1 - Pixel Light Greeting Card (FreeGLUT + Core OpenGL)");

    glDisable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);

    glutDisplayFunc(display);
    glutReshapeFunc(onReshape);
    glutIdleFunc(onIdle);
    glutPassiveMotionFunc(onPassiveMotion);
    glutKeyboardFunc(onKeyboard);
    glutMouseFunc(onMouse);
    glutKeyboardUpFunc(onKeyboardUp);
    glutSpecialFunc(onSpecial);
    glutSpecialUpFunc(onSpecialUp);

    glutMainLoop();
    return 0;
}
