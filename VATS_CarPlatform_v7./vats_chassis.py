"""
VATS CarPlatform v7 - parametryczny model podwozia (CadQuery 2.x)
Uklad wspolrzednych: X = przod auta, Y = lewa strona, Z = gora.
X = 0 -> os tylna, Z = 0 -> spod plyty.
Uruchomienie:  pip install cadquery ; python vats_chassis.py
Wynik: katalog out/ z plikami STEP (do CAD) i STL (do Bambu Studio).

Wymiary oznaczone [ZMIERZ] sa przyjete z typowych wartosci katalogowych
i trzeba je sprawdzic suwmiarka na wlasnych czesciach przed drukiem.
"""
import math
import os
import cadquery as cq

# ----------------------------------------------------------------- parametry
T = 3.0                 # grubosc plyty

# --- kola / os tylna (N20 + DFRobot FIT0494 43x9)
WHEEL_D = 43.0
WHEEL_W = 9.0
MOTOR_W = 12.0          # przekroj N20: 12 (X) x 10 (Z)
MOTOR_H = 10.0
MOTOR_FIT = 0.3         # luz kanalu na szerokosci
MOTOR_SQUEEZE = 0.2     # o tyle kanal jest nizszy niz silnik (docisk obejmy)
MOTOR_PAD = 2.0         # podkladka pod silnikiem -> os kola na Z = T+PAD+H/2
GEARBOX_FACE_Y = 41.0   # |Y| czola przekladni (wyjscie walka)
CRADLE_LEN = 24.0       # dlugosc kanalu: przekladnia 9 + silnik 15 [ZMIERZ]
CRADLE_WALL = 7.0
CAP_T = 3.0

# --- obrys plyty
HALF_W_REAR = 41.0      # polowa szerokosci miedzy tylnymi kolami
HALF_W_MID = 44.0       # polowa szerokosci czesci srodkowej
X_REAR = -15.0
X_WIDE_START = 24.0     # od tego X plyta ma HALF_W_MID

# --- koszyk 2x18650 (Botland AKC-05240), dluzszy bok w poprzek auta
BAT_L = 77.0            # [ZMIERZ]
BAT_W = 41.0            # [ZMIERZ]
BAT_FIT = 0.5           # luz na strone
BAT_RIM_T = 1.6
BAT_RIM_H = 5.0
BAT_X0 = 15.0           # tylna zewnetrzna krawedz ramki

# --- pas uniwersalny (LM2596, TB6612FNG, HC-05): siatka otworow
GRID_D = 3.2
GRID_PITCH = 6.5
GRID_OPEN_COLS = 4      # kolumny siatki przed Nucleo (ESP32 devkit)

# --- Nucleo-64: 70 x 82.5 mm, 70 wzdluz X, 82.5 w poprzek
NUC_X = 70.0
NUC_Y = 82.5
NUC_FIT = 0.3
NUC_STANDOFF = 22.0     # Nucleo jako pietro: pod spodem LM2596, BMS, TB6612
NUC_LEDGE = 3.5         # ile polka wchodzi pod naroznik PCB
NUC_WALL_X = 4.5        # sciana pozycjonujaca przy krotszej krawedzi
NUC_WALL_Y = 2.0
NUC_LIP = 1.5           # scianka nizsza niz PCB 1.6 -> podkladka dociska plytke

# --- gniazda modulow (ramki wg obrysu PCB, bez zaleznosci od otworow)
SLOT_FIT = 0.3          # luz na strone
SLOT_RIM_T = 1.6
SLOT_RIM_H = 3.0
IMU_L = 25.0            # GY-6500 (MPU-6500): 25 x 15 mm, 25 w poprzek auta
IMU_W = 15.0
IMU_PAD_H = 1.5         # podkladka pod srodkiem PCB (luz na luty zlacza)
PCB_T = 1.6
LM_L = 44.0             # LM2596 Botland: 44 x 20 x 13 mm
LM_W = 20.0
DRV_L = 20.3            # Pololu 713 TB6612FNG: 20.3 x 15.2 mm
DRV_W = 15.2
ESP_L = 55.0            # ESP32 devkit [ZMIERZ]: 30 pin ~51.5x28, 38 pin ~55x28
ESP_W = 28.0
ESP_RAIL_H = 5.0        # podniesienie plytki (miejsce na wtyk USB)

# --- przednia os
TRACK = 94.0
SCRUB = 22.5            # odleglosc srodka kola od osi zwrotnicy
YK = TRACK / 2 - SCRUB  # |Y| osi zwrotnicy (kingpin)
TRAIL = 9.0             # os kola za osia zwrotnicy
NECK_HALF_W = 20.0
MAIN_TO_KINGPIN = 41.0  # miejsce na skret przednich kol
ARM_R = 16.0            # ramie zwrotnicy
HORN_R = 13.0           # promien otworu orczyka uzytego do ciegla [ZMIERZ]
LINK_LEN = 18.0         # cieglo orczyk -> drazek

# --- zwrotnica
USE_BEARINGS = False    # True: gniazda pod 2x MR63ZZ (3x6x2.5) na kolo
BRG_OD = 6.0
BRG_W = 2.5
BRG_FIT = 0.15
KN_Z0_GAP = 1.5         # pierscien 1.0 + podkladka 0.5 pod zwrotnica
KN_BODY_H = 11.0
KN_PIN_H = 16.5
KN_PIN_R = 5.0
ARM_T = 5.0
ROD_T = 4.0
ROD_W = 7.0

# --- serwo SG90 / MG90S
SRV_L = 23.0            # [ZMIERZ]
SRV_W = 12.4            # [ZMIERZ]
SRV_FIT = 0.3
SRV_SHAFT_OFF = 5.5     # walek wzgledem srodka korpusu [ZMIERZ]
SRV_HOLE_SPACING = 27.8  # rozstaw otworow w uszach [ZMIERZ]
SRV_TAB_Z = 16.0        # spod uszu nad spodem serwa [ZMIERZ]
SRV_HORN_Z = 28.0       # spod orczyka nad spodem serwa [ZMIERZ]

# --- prowadnice przewodow: otwarte haki na zewnatrz krawedzi plyty
HOOK_X = (30.0, 52.0, 100.0, 125.0, 150.0)   # po obu stronach
HOOK_X_RIGHT_ONLY = (75.0,)                  # lewa strona zajeta przez wtyk USB ESP32
HOOK_CH_W = 5.0         # szerokosc kanalu (miedzy zebrem krawedziowym a hakiem)
HOOK_LEN = 6.0          # dlugosc haka wzdluz auta
HOOK_H = 9.0            # wysokosc haka nad plyta
HOOK_GAP = 3.0          # szczelina do wciskania przewodow

# --- wylacznik zasilania: kolyskowy KCD11 (wciskany w panel), tyl auta
SW_CUT_W = 13.2         # szerokosc wyciecia [ZMIERZ korpus wylacznika]
SW_CUT_H = 8.8          # wysokosc wyciecia [ZMIERZ]
SW_PANEL_T = 2.0        # grubosc panelu (zatrzaski KCD11 trzymaja do ok. 2 mm)
SW_Z = 15.0             # dolna krawedz wyciecia nad plyta (nad silnikami)

# --- otwory
M3_CLR = 3.3
M3_PILOT = 2.9          # M3 wkrecane wprost w PLA
SRV_PILOT = 1.6

# ----------------------------------------------------------------- pochodne
AXLE_Z = T + MOTOR_PAD + MOTOR_H / 2
CH_W = MOTOR_W + MOTOR_FIT
CH_TOP = T + MOTOR_PAD + MOTOR_H - MOTOR_SQUEEZE
CRADLE_X = CH_W / 2 + CRADLE_WALL

BAT_IN_X = BAT_W + 2 * BAT_FIT
BAT_IN_Y = BAT_L + 2 * BAT_FIT
BAT_X1 = BAT_X0 + BAT_IN_X + 2 * BAT_RIM_T
BAT_CX = (BAT_X0 + BAT_X1) / 2

GRID_X0 = BAT_X1 + 5.0
GRID_X1 = GRID_X0 + (GRID_OPEN_COLS - 1) * GRID_PITCH

NUC_X0 = GRID_X1 + 4.0 + NUC_WALL_X
NUC_X1 = NUC_X0 + NUC_X
X_MAIN_END = NUC_X1 + NUC_WALL_X + 1.5
GRID_COLS = int((NUC_X1 - 5.0 - GRID_X0) // GRID_PITCH) + 1

XK = X_MAIN_END + MAIN_TO_KINGPIN          # X osi zwrotnic
WHEELBASE = XK - TRAIL
ACK = math.atan2(YK, XK)                   # kat ramion (Ackermann)
ARM_TIP = (ARM_R * math.cos(ACK), ARM_R * math.sin(ACK))  # lokalnie, lewa
X_ROD = XK + ARM_TIP[0]
XS = X_ROD - HORN_R                        # os walka serwa
SRV_CX = XS - SRV_SHAFT_OFF                # srodek korpusu serwa
X_FRONT = max(SRV_CX + SRV_HOLE_SPACING / 2 + 5.0, X_ROD) + 0.5

KN_Z0 = T + KN_Z0_GAP
ARM_Z0 = KN_Z0 + KN_PIN_H - ARM_T
ROD_Z0 = ARM_Z0 + ARM_T + 0.2
LINK_Z0 = ROD_Z0 + ROD_T + 0.2
SRV_Z0 = LINK_Z0 + ROD_T + 0.3 - SRV_HORN_Z   # spod serwa
SRV_POST_TOP = SRV_Z0 + SRV_TAB_Z


def box(x0, x1, y0, y1, z0, z1):
    return (cq.Workplane("XY")
            .box(x1 - x0, y1 - y0, z1 - z0, centered=False)
            .translate((x0, y0, z0)))


def cyl(x, y, z0, z1, r):
    return cq.Workplane("XY").circle(r).extrude(z1 - z0).translate((x, y, z0))


# ------------------------------------------------------------------- plyta
def chassis():
    ch = 8.0
    outline = [
        (X_REAR + ch, -HALF_W_REAR), (X_WIDE_START - 3, -HALF_W_REAR),
        (X_WIDE_START, -HALF_W_MID), (X_MAIN_END, -HALF_W_MID),
        (X_MAIN_END + 4, -NECK_HALF_W), (X_FRONT - 5, -NECK_HALF_W),
        (X_FRONT, -NECK_HALF_W + 5), (X_FRONT, NECK_HALF_W - 5),
        (X_FRONT - 5, NECK_HALF_W), (X_MAIN_END + 4, NECK_HALF_W),
        (X_MAIN_END, HALF_W_MID), (X_WIDE_START, HALF_W_MID),
        (X_WIDE_START - 3, HALF_W_REAR), (X_REAR + ch, HALF_W_REAR),
        (X_REAR, HALF_W_REAR - ch), (X_REAR, -HALF_W_REAR + ch),
    ]
    p = cq.Workplane("XY").polyline(outline).close().extrude(T)
    # belka przednia + gniazda zwrotnic
    p = p.union(box(XK - 6, XK + 6, -YK, YK, 0, T))
    for s in (1, -1):
        p = p.union(cyl(XK, s * YK, 0, T, 6.5))
        p = p.union(cyl(XK, s * YK, T, T + 1.0, 5.0))

    # lozyska silnikow
    for s in (1, -1):
        y0, y1 = sorted((s * (GEARBOX_FACE_Y - CRADLE_LEN), s * GEARBOX_FACE_Y))
        p = p.union(box(-CRADLE_X, CRADLE_X, y0, y1, T, CH_TOP))
        p = p.cut(box(-CH_W / 2, CH_W / 2, y0 - 1, y1 + 1, T + MOTOR_PAD, CH_TOP + 1))
        ym = (y0 + y1) / 2
        for sx in (1, -1):
            xh = sx * (CH_W / 2 + CRADLE_WALL / 2)
            p = p.cut(cyl(xh, ym, CH_TOP - 9.5, CH_TOP + 1, M3_PILOT / 2))

    # ramka koszyka
    yb = BAT_IN_Y / 2 + BAT_RIM_T
    rim = box(BAT_X0, BAT_X1, -yb, yb, T, T + BAT_RIM_H)
    rim = rim.cut(box(BAT_X0 + BAT_RIM_T, BAT_X1 - BAT_RIM_T,
                      -BAT_IN_Y / 2, BAT_IN_Y / 2, T, T + BAT_RIM_H + 1))
    # przerwy na przewody (narozniki) i na palce
    rim = rim.cut(box(BAT_X0 - 1, BAT_X1 + 1, -12, 12, T, T + BAT_RIM_H + 1))
    for s in (1, -1):
        rim = rim.cut(box(BAT_CX - 8, BAT_CX + 8, s * yb - 3, s * yb + 3,
                          T, T + BAT_RIM_H + 1))
    p = p.union(rim)
    # szczelina na srube M3 koszyka (dowolna pozycja otworu w dnie koszyka)
    p = p.cut(box(BAT_CX - M3_CLR / 2, BAT_CX + M3_CLR / 2, -25, 25, -1, T + 1))

    # siatka otworow
    ny = int((HALF_W_MID - 4.5) // GRID_PITCH)
    for i in range(GRID_COLS):
        for j in range(-ny, ny + 1):
            p = p.cut(cyl(GRID_X0 + i * GRID_PITCH, j * GRID_PITCH, -1, T + 1, GRID_D / 2))

    # zebra krawedziowe (sztywnosc plyty oslabionej siatka)
    for s in (1, -1):
        y0, y1 = sorted((s * (HALF_W_MID - 3.0), s * HALF_W_MID))
        p = p.union(box(X_WIDE_START + 2, X_MAIN_END, y0, y1, T, T + 5.0))

    # narozniki Nucleo
    zl = T + NUC_STANDOFF
    for sx, xe in ((-1, NUC_X0), (1, NUC_X1)):
        for sy in (1, -1):
            ye = sy * NUC_Y / 2
            xa, xb = sorted((xe - sx * NUC_LEDGE, xe + sx * NUC_WALL_X))
            ya, yb2 = sorted((ye - sy * NUC_LEDGE, ye + sy * (NUC_WALL_Y + NUC_FIT)))
            post = box(xa, xb, ya, yb2, T, zl + NUC_LIP)
            # wybranie na PCB (z luzem)
            bx0, bx1 = sorted((xe - sx * 10, xe + sx * NUC_FIT))
            by0, by1 = sorted((ye - sy * 10, ye + sy * NUC_FIT))
            post = post.cut(box(bx0, bx1, by0, by1, zl, zl + NUC_LIP + 1))
            # otwor pod M3 + podkladke dociskajaca PCB
            post = post.cut(cyl(xe + sx * 2.2, ye - sy * 0.6, zl - 10.0, zl + NUC_LIP + 1,   # M3x10 + podkladka
                                M3_PILOT / 2))
            p = p.union(post)

    # zebra szyjki
    for s in (1, -1):
        y0, y1 = sorted((s * (NECK_HALF_W - 3.0), s * NECK_HALF_W))
        p = p.union(box(X_MAIN_END - 45, XK - 22, y0, y1, T, T + 5.0))

    # ---------------- gniazda modulow
    def frame(x0, x1, y0, y1, h, open_sides=""):
        t = SLOT_RIM_T
        f = box(x0 - t, x1 + t, y0 - t, y1 + t, T, T + h)
        f = f.cut(box(x0, x1, y0, y1, T - 0.01, T + h + 1))
        if "x+" in open_sides:
            f = f.cut(box(x1 - 0.1, x1 + t + 1, y0, y1, T - 0.01, T + h + 1))
        return f

    # ESP32 devkit: w poprzek auta, na 4 szynach, USB w strone +Y
    ex0 = BAT_X1 + 0.8
    ex1 = ex0 + ESP_W + 2 * SLOT_FIT
    ey = ESP_L / 2 + SLOT_FIT
    ecx = (ex0 + ex1) / 2
    esp = frame(ex0, ex1, -ey, ey, ESP_RAIL_H + SLOT_RIM_H)
    esp = esp.cut(box(ecx - 7, ecx + 7, ey - 0.1, ey + SLOT_RIM_T + 1,
                      T + ESP_RAIL_H, T + ESP_RAIL_H + SLOT_RIM_H + 1))
    for yr in (-21.0, -7.0, 7.0, 21.0):
        esp = esp.union(box(ex0, ex1, yr - 1.5, yr + 1.5, T, T + ESP_RAIL_H))
    p = p.union(esp)
    # przerwa w zebrze krawedziowym na wtyk USB
    p = p.cut(box(ecx - 9, ecx + 9, HALF_W_MID - 3.01, HALF_W_MID + 1, T, T + 6))

    # LM2596: pod Nucleo, przy lewej krawedzi (dostep do potencjometru z boku)
    lx0 = NUC_X0 + 6.0
    lx1 = lx0 + LM_L + 2 * SLOT_FIT
    ly1 = HALF_W_MID - 3.0
    ly0 = ly1 - LM_W - 2 * SLOT_FIT
    p = p.union(frame(lx0, lx1, ly0, ly1, SLOT_RIM_H))

    # TB6612FNG Pololu: pod Nucleo, przy prawej krawedzi, blisko silnikow
    dx0 = NUC_X0 + 6.0
    dx1 = dx0 + DRV_L + 2 * SLOT_FIT
    dy0 = -(HALF_W_MID - 3.0)
    dy1 = dy0 + DRV_W + 2 * SLOT_FIT
    p = p.union(frame(dx0, dx1, dy0, dy1, SLOT_RIM_H))

    # IMU GY-6500: na szyjce, w osi auta; docisk 2x M3 + podkladka
    ix0 = X_MAIN_END + 0.5
    ix1 = ix0 + IMU_W + 2 * SLOT_FIT
    iy = IMU_L / 2 + SLOT_FIT
    icx = (ix0 + ix1) / 2
    ztop = T + IMU_PAD_H + PCB_T
    imu = frame(ix0, ix1, -iy, iy, IMU_PAD_H + PCB_T, open_sides="x+")
    imu = imu.union(box(icx - 4.0, icx + 4.0, -iy, iy, T, T + IMU_PAD_H))
    for s2 in (1, -1):
        b = cyl(icx, s2 * (iy + 1.9), T, ztop, 3.4)
        b = b.cut(box(ix0, ix1, -iy, iy, T + IMU_PAD_H, ztop + 1))
        imu = imu.union(b)
    p = p.union(imu)
    for s2 in (1, -1):
        p = p.cut(cyl(icx, s2 * (iy + 1.9), -1, ztop + 1, M3_PILOT / 2))   # przelotowo: M3x10

    # ---------------- haki na przewody
    def hook(x, s):
        y0 = HALF_W_MID
        y1 = y0 + HOOK_CH_W
        y2 = y1 + 2.0
        zt = T + HOOK_H
        lip = HOOK_CH_W - HOOK_GAP
        h = box(x - HOOK_LEN / 2, x + HOOK_LEN / 2, y0 - 0.5, y2, 0, T)
        h = h.union(box(x - HOOK_LEN / 2, x + HOOK_LEN / 2, y1, y2, T, zt))
        w = (cq.Workplane("YZ")
             .polyline([(y1, zt - 1 - lip), (y1 - lip, zt - 1), (y1 - lip, zt), (y1, zt)])
             .close().extrude(HOOK_LEN).translate((x - HOOK_LEN / 2, 0, 0)))
        h = h.union(w)
        return h if s == 1 else h.mirror("XZ")

    for x in HOOK_X:
        for s in (1, -1):
            p = p.union(hook(x, s))
    for x in HOOK_X_RIGHT_ONLY:
        p = p.union(hook(x, -1))

    # ---------------- panel wylacznika (tyl, w osi auta, nad silnikami)
    pw = SW_CUT_W / 2 + 4.5
    zc0 = T + SW_Z
    ztop = zc0 + SW_CUT_H + 2.5
    sw = box(X_REAR, X_REAR + SW_PANEL_T, -pw, pw, T, ztop)
    sw = sw.cut(box(X_REAR - 1, X_REAR + SW_PANEL_T + 1, -SW_CUT_W / 2, SW_CUT_W / 2,
                    zc0, zc0 + SW_CUT_H))
    for s2 in (1, -1):
        g = (cq.Workplane("XZ")
             .polyline([(X_REAR + SW_PANEL_T, T), (-6.5, T), (X_REAR + SW_PANEL_T, zc0 - 1)])
             .close().extrude(-1.6))
        ya = pw - 1.6 if s2 == 1 else -pw
        sw = sw.union(g.translate((0, ya, 0)))
    p = p.union(sw)

    # serwo: okno + slupki
    hw = SRV_W / 2 + SRV_FIT
    hl = SRV_L / 2 + SRV_FIT
    for s in (1, -1):
        xc = SRV_CX + s * SRV_HOLE_SPACING / 2
        xa, xb = sorted((SRV_CX + s * hl, xc + s * 2.6))
        p = p.union(box(xa, xb, -hw, hw, T, SRV_POST_TOP))
        p = p.cut(cyl(xc, 0, SRV_POST_TOP - 7, SRV_POST_TOP + 1, SRV_PILOT / 2))
        # wyciecie na przewod serwa (w obu slupkach, bo strona wyjscia [ZMIERZ])
        p = p.cut(box(xa - 1, xb + 1, -2.75, 2.75, T - 0.01, SRV_Z0 + 8.5))
    if SRV_Z0 < T:
        p = p.cut(box(SRV_CX - hl, SRV_CX + hl, -hw, hw, -1, T + 1))
    # wyjscie przewodu serwa
    p = p.cut(box(SRV_CX - hl - 9.5, SRV_CX - hl - 6.5, -4, 4, -1, T + 1))

    # otwory zwrotnic
    for s in (1, -1):
        p = p.cut(cyl(XK, s * YK, -1, T + 2, M3_CLR / 2))

    # przepusty przewodow przy silnikach
    for s in (1, -1):
        p = p.cut(box(-4, 4, s * 6 - 2, s * 6 + 2, -1, T + 1))
    return p


def motor_cap():
    c = box(-CRADLE_X, CRADLE_X, -CRADLE_LEN / 2, CRADLE_LEN / 2, 0, CAP_T)
    for sx in (1, -1):
        c = c.cut(cyl(sx * (CH_W / 2 + CRADLE_WALL / 2), 0, -1, CAP_T + 1, 3.4 / 2))
    return c


def knuckle(side=1):
    """Zwrotnica w ukladzie lokalnym: os zwrotnicy w (0,0), +Y na zewnatrz.
    side=1 lewa, side=-1 prawa (lustro)."""
    z0 = 0.0
    k = cyl(0, 0, z0, KN_PIN_H, KN_PIN_R)
    k = k.union(box(-TRAIL - 5.5, 2.0, 1.0, 13.0, z0, KN_BODY_H))
    # ramie
    ax, ay = ARM_TIP
    ang = math.degrees(math.atan2(ay, ax))
    arm = box(0, ARM_R, -ROD_W / 2, ROD_W / 2, KN_PIN_H - ARM_T, KN_PIN_H)
    arm = arm.union(cyl(ARM_R, 0, KN_PIN_H - ARM_T, KN_PIN_H, ROD_W / 2))
    arm = arm.cut(cyl(ARM_R, 0, KN_PIN_H - ARM_T - 1, KN_PIN_H + 1, M3_PILOT / 2))
    arm = arm.rotate((0, 0, 0), (0, 0, 1), ang)
    k = k.union(arm)
    # os zwrotnicy
    k = k.cut(cyl(0, 0, -1, KN_PIN_H + 1, M3_PILOT / 2))   # sworzen wkrecany od spodu plyty
    # os kola + gniazda lozysk
    za = AXLE_Z - KN_Z0
    bore = (cq.Workplane("XZ").circle(3.4 / 2).extrude(-14.0)
            .translate((-TRAIL, 0, za)))
    k = k.cut(bore)
    for y in ((1.0 - 0.01, 13.0 - BRG_W - 0.1) if USE_BEARINGS else ()):
        pocket = (cq.Workplane("XZ").circle((BRG_OD + BRG_FIT) / 2)
                  .extrude(-(BRG_W + 0.11)).translate((-TRAIL, y, za)))
        k = k.cut(pocket)
    if side == -1:
        k = k.mirror("XZ")
    return k


def bar(length, holes):
    """Plaski drazek wzdluz Y, otwory: lista (y, srednica)."""
    b = box(-ROD_W / 2, ROD_W / 2, 0, length, 0, ROD_T)
    b = b.union(cyl(0, 0, 0, ROD_T, ROD_W / 2)).union(cyl(0, length, 0, ROD_T, ROD_W / 2))
    for y, d in holes:
        b = b.cut(cyl(0, y, -1, ROD_T + 1, d / 2))
    return b


ROD_LEN = 2 * (YK + ARM_TIP[1])


def tie_rod():
    return bar(ROD_LEN, [(0, M3_CLR), (ROD_LEN, M3_CLR),
                         (ROD_LEN / 2 - LINK_LEN, M3_PILOT)])


def drag_link():
    return bar(LINK_LEN, [(0, M3_CLR), (LINK_LEN, SRV_PILOT)])


# ------------------------------------------------------- atrapy do podgladu
def mockups():
    m = {}
    wheels = None
    for s in (1, -1):
        for x, y in ((0, s * (GEARBOX_FACE_Y + 1.5 + WHEEL_W / 2)),
                     (WHEELBASE, s * TRACK / 2)):
            w = (cq.Workplane("XZ").circle(WHEEL_D / 2).extrude(WHEEL_W)
                 .translate((x, y + WHEEL_W / 2, AXLE_Z)))
            wheels = w if wheels is None else wheels.union(w)
    m["wheels"] = wheels
    mot = None
    for s in (1, -1):
        y0, y1 = sorted((s * (GEARBOX_FACE_Y - 34), s * GEARBOX_FACE_Y))
        b = box(-6, 6, y0, y1, T + MOTOR_PAD, T + MOTOR_PAD + 10)
        ys0, ys1 = sorted((s * GEARBOX_FACE_Y, s * (GEARBOX_FACE_Y + 10)))
        b = b.union(cq.Workplane("XZ").circle(1.5).extrude(-(ys1 - ys0))
                    .translate((0, ys0, AXLE_Z)))
        mot = b if mot is None else mot.union(b)
    m["motors"] = mot
    m["battery"] = box(BAT_CX - BAT_W / 2, BAT_CX + BAT_W / 2, -BAT_L / 2, BAT_L / 2,
                       T, T + 20)
    zl = T + NUC_STANDOFF
    m["nucleo"] = box(NUC_X0, NUC_X1, -NUC_Y / 2, NUC_Y / 2, zl, zl + 1.6)
    srv = box(SRV_CX - SRV_L / 2, SRV_CX + SRV_L / 2, -SRV_W / 2, SRV_W / 2,
              SRV_Z0, SRV_Z0 + 22.7)
    srv = srv.union(box(SRV_CX - 16.2, SRV_CX + 16.2, -SRV_W / 2, SRV_W / 2,
                        SRV_Z0 + SRV_TAB_Z, SRV_Z0 + SRV_TAB_Z + 2.5))
    srv = srv.union(cyl(XS, 0, SRV_Z0 + 22.7, SRV_Z0 + SRV_HORN_Z, 5.9))
    srv = srv.union(box(XS - 3, XS + HORN_R + 2.5, -3, 3,
                        SRV_Z0 + SRV_HORN_Z, SRV_Z0 + SRV_HORN_Z + 1.5))
    m["servo"] = srv
    ix0 = X_MAIN_END + 0.5 + SLOT_FIT
    m["imu"] = box(ix0, ix0 + IMU_W, -IMU_L / 2, IMU_L / 2,
                   T + IMU_PAD_H, T + IMU_PAD_H + PCB_T)
    ex0 = BAT_X1 + 0.8 + SLOT_FIT
    m["esp32"] = box(ex0, ex0 + ESP_W, -ESP_L / 2, ESP_L / 2,
                     T + ESP_RAIL_H, T + ESP_RAIL_H + 5.0)
    lx0 = NUC_X0 + 6.0 + SLOT_FIT
    m["lm2596"] = box(lx0, lx0 + LM_L, HALF_W_MID - 3.0 - SLOT_FIT - LM_W,
                      HALF_W_MID - 3.0 - SLOT_FIT, T + 0.5, T + 13.5)
    m["tb6612"] = box(lx0, lx0 + DRV_L, -(HALF_W_MID - 3.0) + SLOT_FIT,
                      -(HALF_W_MID - 3.0) + SLOT_FIT + DRV_W, T + 0.5, T + 3.5)
    return m


def build():
    parts = {
        "chassis": chassis(),
        "motor_cap": motor_cap(),
        "knuckle_L": knuckle(1),
        "knuckle_R": knuckle(-1),
        "tie_rod": tie_rod(),
        "drag_link": drag_link(),
    }
    return parts


def assembly(parts):
    asm = cq.Assembly(name="VATS_CarPlatform")
    asm.add(parts["chassis"], name="chassis", color=cq.Color(0.2, 0.45, 0.8))
    for s, n in ((1, "L"), (-1, "R")):
        ym = s * (GEARBOX_FACE_Y - CRADLE_LEN / 2)
        asm.add(parts["motor_cap"], name=f"motor_cap_{n}",
                loc=cq.Location((0, ym, CH_TOP + MOTOR_SQUEEZE)),
                color=cq.Color(0.9, 0.5, 0.1))
        asm.add(parts[f"knuckle_{n}"], name=f"knuckle_{n}",
                loc=cq.Location((XK, s * YK, KN_Z0)), color=cq.Color(0.9, 0.5, 0.1))
    asm.add(parts["tie_rod"], name="tie_rod",
            loc=cq.Location((X_ROD, -ROD_LEN / 2, ROD_Z0)), color=cq.Color(0.8, 0.1, 0.1))
    asm.add(parts["drag_link"], name="drag_link",
            loc=cq.Location((X_ROD, -LINK_LEN, LINK_Z0)), color=cq.Color(0.1, 0.7, 0.2))
    for n, shp in mockups().items():
        asm.add(shp, name="REF_" + n, color=cq.Color(0.6, 0.6, 0.6, 0.5))
    return asm


if __name__ == "__main__":
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
    os.makedirs(out, exist_ok=True)
    parts = build()
    for name, shp in parts.items():
        cq.exporters.export(shp, os.path.join(out, f"{name}.step"))
        cq.exporters.export(shp, os.path.join(out, f"{name}.stl"), tolerance=0.02,
                            angularTolerance=0.1)
    assembly(parts).save(os.path.join(out, "VATS_assembly.step"))
    bb = parts["chassis"].val().BoundingBox()
    print(f"plyta: {bb.xlen:.1f} x {bb.ylen:.1f} x {bb.zlen:.1f} mm")
    print(f"rozstaw osi {WHEELBASE:.1f}  rozstaw kol {TRACK:.1f}  XK {XK:.1f}  YK {YK:.1f}")
    print(f"os kola Z={AXLE_Z:.1f}  przeswit {WHEEL_D/2-AXLE_Z:.1f}")
    print(f"serwo: spod Z={SRV_Z0:.2f}  slupki top Z={SRV_POST_TOP:.2f}  XS={XS:.1f}")
    print(f"drazek L={ROD_LEN:.2f} Z={ROD_Z0:.1f}  cieglo Z={LINK_Z0:.1f}")
    print(f"kat Ackermanna ramion {math.degrees(ACK):.2f} deg")
