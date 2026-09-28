import re, os

def patch(filepath, old, new, desc):
    if not os.path.exists(filepath):
        print(f"[SKIP] No existe: {filepath}")
        return
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    if old not in content:
        print(f"[SKIP] Ya parchado o no encontrado: {desc}")
        return
    content = content.replace(old, new, 1)
    with open(filepath, 'w', encoding='utf-8', errors='ignore') as f:
        f.write(content)
    print(f"[OK] {desc}")

# ─────────────────────────────────────────────
# FIX 1: Rasterizer D3D11 invalido (overlay.h)
# ─────────────────────────────────────────────
patch(
    "source/source/overlay.h",
    "D3D11_RASTERIZER_DESC rasterizer_desc = {};\n        rasterizer_desc.MultisampleEnable = FALSE;\n        rasterizer_desc.AntialiasedLineEnable = FALSE;",
    "D3D11_RASTERIZER_DESC rasterizer_desc = {};\n        rasterizer_desc.FillMode = D3D11_FILL_SOLID;\n        rasterizer_desc.CullMode = D3D11_CULL_NONE;\n        rasterizer_desc.DepthClipEnable = TRUE;\n        rasterizer_desc.MultisampleEnable = FALSE;\n        rasterizer_desc.AntialiasedLineEnable = FALSE;",
    "FIX 1: Rasterizer D3D11 FillMode/CullMode invalidos"
)

# ─────────────────────────────────────────────
# FIX 2: ZeroMemory redundante (overlay.h)
# ─────────────────────────────────────────────
patch(
    "source/source/overlay.h",
    "MSG msg = { NULL };\n        ZeroMemory(&msg, sizeof(MSG));",
    "MSG msg = {};\n        ZeroMemory(&msg, sizeof(MSG));",
    "FIX 2: MSG init limpio"
)

# ─────────────────────────────────────────────
# FIX 3: CloseOSK loop infinito (overlay.h)
# ─────────────────────────────────────────────
patch(
    "source/source/overlay.h",
    "        HWND hOskWindow = nullptr;\n        do {\n            hOskWindow = FindWindow(OBF(L\"OSKMainClass\"), nullptr);\n            if (hOskWindow)\n            {\n                PostMessage(hOskWindow, WM_CLOSE, 0, 0);\n                Sleep(100);\n            }\n        } while (hOskWindow);",
    "        HWND hOskWindow = nullptr;\n        int _osk_attempts = 0;\n        do {\n            hOskWindow = FindWindow(OBF(L\"OSKMainClass\"), nullptr);\n            if (hOskWindow)\n            {\n                PostMessage(hOskWindow, WM_CLOSE, 0, 0);\n                Sleep(100);\n            }\n            _osk_attempts++;\n        } while (hOskWindow && _osk_attempts < 30);",
    "FIX 3: CloseOSK timeout anti loop infinito"
)

# ─────────────────────────────────────────────
# FIX 4: Division por cero en gradient (draw.cpp)
# ─────────────────────────────────────────────
patch(
    "source/source/slate/src/draw.cpp",
    "            float X = ImClamp((vert->pos.x - start.x) / (end.x - start.x), 0.0f, 1.0f);",
    "            float _dX = (end.x - start.x); float X = (_dX != 0.f) ? ImClamp((vert->pos.x - start.x) / _dX, 0.0f, 1.0f) : 0.f;",
    "FIX 4a: Division por cero X en gradient()"
)

patch(
    "source/source/slate/src/draw.cpp",
    "            float Y = ImClamp((vert->pos.y - start.y) / (end.y - start.y), 0.0f, 1.0f);",
    "            float _dY = (end.y - start.y); float Y = (_dY != 0.f) ? ImClamp((vert->pos.y - start.y) / _dY, 0.0f, 1.0f) : 0.f;",
    "FIX 4b: Division por cero Y en gradient()"
)

# ─────────────────────────────────────────────
# FIX 5: rotateend() offset incorrecto (draw.cpp)
# ─────────────────────────────────────────────
patch(
    "source/source/slate/src/draw.cpp",
    "    float s = sin( rad ), c = cos( rad );\n    center = ImRotate( center, s, c) - center;\n\n    auto& buf = GetWindowDrawList()->VtxBuffer;\n    for ( int i = rotation_start_index; i < buf.Size; i++ )\n        buf[i].pos = ImRotate( buf[i].pos, s, c ) - center;",
    "    float s = sin( rad ), c = cos( rad );\n    ImVec2 _rotated_center = ImRotate( center, s, c );\n    ImVec2 _delta = _rotated_center - center;\n\n    auto& buf = GetWindowDrawList()->VtxBuffer;\n    for ( int i = rotation_start_index; i < buf.Size; i++ )\n        buf[i].pos = ImRotate( buf[i].pos, s, c ) - _delta;",
    "FIX 5: rotateend() delta calculo correcto"
)

# ─────────────────────────────────────────────
# FIX 6: Double negativo en Yaw (sdk.h)
# ─────────────────────────────────────────────
patch(
    "source/source/sdk.h",
    "view_point.rotation.y = ((atan2(fnrot.a * -1, fnrot.b) * (180.0 / M_PI)) * -1) * -1;",
    "view_point.rotation.y = atan2(fnrot.a * -1, fnrot.b) * (180.0 / M_PI);",
    "FIX 6: Doble negativo en calculo de Yaw"
)

# ─────────────────────────────────────────────
# FIX 7: dot() y distance() tipo float->double (sdk.h)
# ─────────────────────────────────────────────
patch(
    "source/source/sdk.h",
    "inline float dot(fvector v) { return x * v.x + y * v.y + z * v.z; }",
    "inline double dot(fvector v) { return x * v.x + y * v.y + z * v.z; }",
    "FIX 7a: dot() retorna double"
)

patch(
    "source/source/sdk.h",
    "inline float distance(fvector v) { return double(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0))); }",
    "inline double distance(fvector v) { return sqrt(pow(v.x - x, 2.0) + pow(v.y - y, 2.0) + pow(v.z - z, 2.0)); }",
    "FIX 7b: distance() usa sqrt double"
)

# ─────────────────────────────────────────────
# FIX 8: Bools sin inicializar (settings.hpp)
# ─────────────────────────────────────────────
patch(
    "source/source/settings_new/settings.hpp",
    "bool custom_box_color;",
    "bool custom_box_color = false;",
    "FIX 8a: custom_box_color init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool custom_skeleton_color;",
    "bool custom_skeleton_color = false;",
    "FIX 8b: custom_skeleton_color init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool default_outline_colors;",
    "bool default_outline_colors = false;",
    "FIX 8c: default_outline_colors init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool custom_outline;",
    "bool custom_outline = false;",
    "FIX 8d: custom_outline init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool rank;",
    "bool rank = true;",
    "FIX 8e: rank init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool level;",
    "bool level = false;",
    "FIX 8f: level init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool kills;",
    "bool kills = false;",
    "FIX 8g: kills init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool weapon;",
    "bool weapon = false;",
    "FIX 8h: weapon init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool ammo;",
    "bool ammo = false;",
    "FIX 8i: ammo init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool rarity;",
    "bool rarity = false;",
    "FIX 8j: rarity init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool distance;",
    "bool distance = false;",
    "FIX 8k: distance init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool username;",
    "bool username = false;",
    "FIX 8l: username init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool teamid;",
    "bool teamid = false;",
    "FIX 8m: teamid init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool teamsize;",
    "bool teamsize = false;",
    "FIX 8n: teamsize init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool platform;",
    "bool platform = true;",
    "FIX 8o: platform init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "bool stream_proof;",
    "bool stream_proof = false;",
    "FIX 8p: stream_proof init"
)
patch(
    "source/source/settings_new/settings.hpp",
    "float snapline_thickness;",
    "float snapline_thickness = 1.0f;",
    "FIX 8q: snapline_thickness init"
)

print("\n[DONE] Todos los fixes aplicados. Ahora haz git push.")
