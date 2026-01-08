#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <set>
#include <cmath>
#include <string>
#include "resource.h"


using namespace std;

// CONSTANTES Y CONFIGURACIÓN

constexpr int MAX_UNDO_STACK = 50;
constexpr int PIN_SNAP_DISTANCE = 100;
constexpr int GATE_WIDTH = 70;
constexpr int GATE_HEIGHT = 35;
constexpr int GRID_SIZE = 20;
constexpr int CABLE_HIT_DISTANCE = 5;

// ENUMERACIONES
enum GateType {
    GATE_AND,
    GATE_OR,
    GATE_XOR,
    GATE_NOT,
    GATE_NAND,
    GATE_NOR,
    GATE_XNOR,
    GATE_SWITCH,
    GATE_LED
};

enum Mode {
    MODE_NORMAL,
    MODE_PLACING,
    MODE_CABLE,
    MODE_DELETE
};

enum PinType {
    PIN_INPUT1 = 0,
    PIN_INPUT2 = 1,
    PIN_OUTPUT = 2
};


// CLASE PARA GESTIÓN AUTOMÁTICA DE RECURSOS GDI

class GDIGuard {
private:
    HDC hdc;
    HGDIOBJ oldObj;
    HGDIOBJ newObj;

public:
    GDIGuard(HDC h, HGDIOBJ obj) : hdc(h), newObj(obj) {
        oldObj = SelectObject(hdc, newObj);
    }

    ~GDIGuard() {
        if (hdc && oldObj) {
            SelectObject(hdc, oldObj);
        }
        if (newObj) {
            DeleteObject(newObj);
        }
    }

    // Prevenir copia
    GDIGuard(const GDIGuard&) = delete;
    GDIGuard& operator=(const GDIGuard&) = delete;
};


// ESTRUCTURAS

struct GateInstance {
    GateType type;
    int x;
    int y;
    POINT in1;
    POINT in2;
    POINT out;
    int val_in1;
    int val_in2;
    int val_out;
    bool selected;

    GateInstance() : type(GATE_AND), x(0), y(0),
                     val_in1(-1), val_in2(-1), val_out(0), selected(false) {
        in1.x = in1.y = 0;
        in2.x = in2.y = 0;
        out.x = out.y = 0;
    }

    bool HasInput1() const {
        return type != GATE_SWITCH;
    }

    bool HasInput2() const {
        return type != GATE_NOT && type != GATE_SWITCH && type != GATE_LED;
    }

    bool HasOutput() const {
        return type != GATE_LED;
    }
};

struct Cable {
    int gateStart;
    int pinStart;
    int gateEnd;
    int pinEnd;
    int value;
    bool selected;

    Cable() : gateStart(-1), pinStart(-1), gateEnd(-1), pinEnd(-1),
              value(-1), selected(false) {}

    bool IsValid() const {
        return gateStart >= 0 && gateEnd >= 0;
    }
};

struct HistoryState {
    vector<GateInstance> gates;
    vector<Cable> cables;
};


// CLASE PRINCIPAL DEL CIRCUITO

class Circuit {
public:
    vector<GateInstance> gates;
    vector<Cable> cables;
    vector<HistoryState> undoStack;
    vector<HistoryState> redoStack;

    void SaveState() {
        HistoryState state;
        state.gates = gates;
        state.cables = cables;
        undoStack.push_back(state);

        if (undoStack.size() > MAX_UNDO_STACK) {
            undoStack.erase(undoStack.begin());
        }
        redoStack.clear();
    }

    bool Undo() {
        if (undoStack.empty()) return false;

        HistoryState current;
        current.gates = gates;
        current.cables = cables;
        redoStack.push_back(current);

        HistoryState prev = undoStack.back();
        undoStack.pop_back();

        gates = prev.gates;
        cables = prev.cables;

        return true;
    }

    bool Redo() {
        if (redoStack.empty()) return false;

        HistoryState current;
        current.gates = gates;
        current.cables = cables;
        undoStack.push_back(current);

        HistoryState next = redoStack.back();
        redoStack.pop_back();

        gates = next.gates;
        cables = next.cables;

        return true;
    }

    void Clear() {
        gates.clear();
        cables.clear();
    }

    void ComputePins(GateInstance &gate) const {
        switch (gate.type) {
            case GATE_AND:
            case GATE_NAND:
                gate.in1.x = gate.x - 15; gate.in1.y = gate.y + 10;
                gate.in2.x = gate.x - 15; gate.in2.y = gate.y + 20;
                gate.out.x = gate.x + 60; gate.out.y = gate.y + 15;
                break;

            case GATE_OR:
            case GATE_NOR:
                gate.in1.x = gate.x - 10; gate.in1.y = gate.y + 10;
                gate.in2.x = gate.x - 10; gate.in2.y = gate.y + 20;
                gate.out.x = gate.x + 60; gate.out.y = gate.y + 15;
                break;

            case GATE_XOR:
            case GATE_XNOR:
                gate.in1.x = gate.x - 10; gate.in1.y = gate.y + 10;
                gate.in2.x = gate.x - 10; gate.in2.y = gate.y + 20;
                gate.out.x = gate.x + 60; gate.out.y = gate.y + 15;
                break;

            case GATE_NOT:
                gate.in1.x = gate.x - 15; gate.in1.y = gate.y + 15;
                gate.in2.x = 0; gate.in2.y = 0;
                gate.out.x = gate.x + 50; gate.out.y = gate.y + 15;
                break;

            case GATE_SWITCH:
                gate.in1.x = 0; gate.in1.y = 0;
                gate.in2.x = 0; gate.in2.y = 0;
                gate.out.x = gate.x + 55; gate.out.y = gate.y + 15;
                break;

            case GATE_LED:
                gate.in1.x = gate.x - 15; gate.in1.y = gate.y + 15;
                gate.in2.x = 0; gate.in2.y = 0;
                gate.out.x = 0; gate.out.y = 0;
                break;
        }
    }

    POINT GetPinPosition(const GateInstance &gate, int pin) const {
        switch(pin) {
            case PIN_INPUT1: return gate.in1;
            case PIN_INPUT2: return gate.in2;
            case PIN_OUTPUT: return gate.out;
        }
        POINT p = {0, 0};
        return p;
    }

    bool IsValidConnection(int gateA, int pinA, int gateB, int pinB) const {

        // No conectar consigo mismo
        if (gateA == gateB) return false;

        // Identificar roles: salida - entrada
        bool aIsOutput = (pinA == PIN_OUTPUT);
        bool bIsOutput = (pinB == PIN_OUTPUT);

        // Debe haber exactamente una salida y una entrada
        if (aIsOutput == bIsOutput) return false; // ambos salida o ambos entrada → inválido

        int startGate = aIsOutput ? gateA : gateB;
        int startPin  = aIsOutput ? pinA  : pinB;
        int endGate   = aIsOutput ? gateB : gateA;
        int endPin    = aIsOutput ? pinB  : pinA;

        // LED solo acepta entrada 1
        if (gates[endGate].type == GATE_LED && endPin != PIN_INPUT1) return false;

        // LED no tiene salida
        if (gates[startGate].type == GATE_LED) return false;

        // SWITCH solo puede conectar desde su salida
        if (gates[startGate].type == GATE_SWITCH && startPin != PIN_OUTPUT) return false;

        // Verificar que la entrada no esté ya conectada
        for (const auto& cable : cables) {
            if (cable.gateEnd == endGate && cable.pinEnd == endPin) return false;
        }

        // Verificar que no exista ya esta conexión exacta
        for (const auto& cable : cables) {
            if (cable.gateStart == startGate && cable.pinStart == startPin &&
                cable.gateEnd == endGate && cable.pinEnd == endPin) return false;
        }

        return true;
    }

    bool DetectLoop() const {
        vector<set<int>> graph(gates.size());

        for (const auto& cable : cables) {
            graph[cable.gateStart].insert(cable.gateEnd);
        }

        vector<int> color(gates.size(), 0);

        for (size_t start = 0; start < gates.size(); start++) {
            if (color[start] != 0) continue;

            vector<int> stack;
            stack.push_back(static_cast<int>(start));

            while (!stack.empty()) {
                int u = stack.back();

                if (color[u] == 0) {
                    color[u] = 1;
                    for (int v : graph[u]) {
                        if (color[v] == 1) return true;
                        if (color[v] == 0) {
                            stack.push_back(v);
                        }
                    }
                } else {
                    color[u] = 2;
                    stack.pop_back();
                }
            }
        }
        return false;
    }

    int EvaluateGate(const GateInstance &gate) const {
        switch (gate.type) {
            case GATE_AND:
                return (gate.val_in1 == 1 && gate.val_in2 == 1) ? 1 : 0;
            case GATE_OR:
                return (gate.val_in1 == 1 || gate.val_in2 == 1) ? 1 : 0;
            case GATE_XOR:
                return (gate.val_in1 != gate.val_in2) ? 1 : 0;
            case GATE_NOT:
                return (gate.val_in1 == 1) ? 0 : 1;
            case GATE_NAND:
                return (gate.val_in1 == 1 && gate.val_in2 == 1) ? 0 : 1;
            case GATE_NOR:
                return (gate.val_in1 == 1 || gate.val_in2 == 1) ? 0 : 1;
            case GATE_XNOR:
                return (gate.val_in1 == gate.val_in2) ? 1 : 0;
            default:
                return -1;
        }
    }

    void PropagateSignals() {
        // Resetear entradas de todas las compuertas (excepto switches)
        for (auto& gate : gates) {
            if (gate.type != GATE_SWITCH) {
                gate.val_in1 = -1;
                gate.val_in2 = -1;
            }
        }

        // Propagación topológica mejorada
        bool changed = true;
        int iterations = 0;
        const int MAX_ITERATIONS = 100;

        while (changed && iterations < MAX_ITERATIONS) {
            changed = false;
            iterations++;

            // Propagar valores a través de los cables
            for (auto& cable : cables) {
                if (cable.gateStart < 0 || cable.gateStart >= static_cast<int>(gates.size()) ||
                    cable.gateEnd < 0 || cable.gateEnd >= static_cast<int>(gates.size())) {
                    continue;
                }

                GateInstance& src = gates[cable.gateStart];
                GateInstance& dst = gates[cable.gateEnd];

                int val = src.val_out;
                cable.value = val;

                int* target = (cable.pinEnd == PIN_INPUT1) ? &dst.val_in1 : &dst.val_in2;

                if (*target != val) {
                    *target = val;
                    changed = true;
                }
            }

            // Evaluar compuertas
            for (auto& gate : gates) {
                if (gate.type == GATE_LED || gate.type == GATE_SWITCH) continue;

                // Solo evaluar si las entradas están listas
                if (gate.HasInput1() && gate.val_in1 == -1) continue;
                if (gate.HasInput2() && gate.val_in2 == -1) continue;

                int newOut = EvaluateGate(gate);

                if (gate.val_out != newOut) {
                    gate.val_out = newOut;
                    changed = true;
                }
            }
        }
    }

    bool SnapToPin(int mx, int my, int& gateIndex, int& pinIndex, POINT& result) const {
        for (size_t i = 0; i < gates.size(); i++) {
            const GateInstance& gate = gates[i];

            // LED - solo entrada
            if (gate.type == GATE_LED) {
                int dx = mx - gate.in1.x;
                int dy = my - gate.in1.y;
                if (dx*dx + dy*dy < PIN_SNAP_DISTANCE) {
                    gateIndex = static_cast<int>(i);
                    pinIndex = PIN_INPUT1;
                    result = gate.in1;
                    return true;
                }
                continue;
            }

            // SWITCH - solo salida
            if (gate.type == GATE_SWITCH) {
                int dx = mx - gate.out.x;
                int dy = my - gate.out.y;
                if (dx*dx + dy*dy < PIN_SNAP_DISTANCE) {
                    gateIndex = static_cast<int>(i);
                    pinIndex = PIN_OUTPUT;
                    result = gate.out;
                    return true;
                }
                continue;
            }

            // Entrada 1
            if (gate.HasInput1()) {
                int dx = mx - gate.in1.x;
                int dy = my - gate.in1.y;
                if (dx*dx + dy*dy < PIN_SNAP_DISTANCE) {
                    gateIndex = static_cast<int>(i);
                    pinIndex = PIN_INPUT1;
                    result = gate.in1;
                    return true;
                }
            }

            // Entrada 2
            if (gate.HasInput2()) {
                int dx = mx - gate.in2.x;
                int dy = my - gate.in2.y;
                if (dx*dx + dy*dy < PIN_SNAP_DISTANCE) {
                    gateIndex = static_cast<int>(i);
                    pinIndex = PIN_INPUT2;
                    result = gate.in2;
                    return true;
                }
            }

            // Salida
            if (gate.HasOutput()) {
                int dx = mx - gate.out.x;
                int dy = my - gate.out.y;
                if (dx*dx + dy*dy < PIN_SNAP_DISTANCE) {
                    gateIndex = static_cast<int>(i);
                    pinIndex = PIN_OUTPUT;
                    result = gate.out;
                    return true;
                }
            }
        }
        return false;
    }

    int HitTestGate(int mx, int my) const {
        for (int i = static_cast<int>(gates.size()) - 1; i >= 0; i--) {
            const GateInstance& gate = gates[i];
            if (mx >= gate.x - 15 && mx <= gate.x + GATE_WIDTH &&
                my >= gate.y && my <= gate.y + GATE_HEIGHT) {
                return i;
            }
        }
        return -1;
    }

    int HitTestCable(int mx, int my) const {
        for (size_t i = 0; i < cables.size(); i++) {
            const Cable& cable = cables[i];

            if (cable.gateStart < 0 || cable.gateStart >= static_cast<int>(gates.size()) ||
                cable.gateEnd < 0 || cable.gateEnd >= static_cast<int>(gates.size())) {
                continue;
            }

            POINT p1 = GetPinPosition(gates[cable.gateStart], cable.pinStart);
            POINT p2 = GetPinPosition(gates[cable.gateEnd], cable.pinEnd);

            float dx = static_cast<float>(p2.x - p1.x);
            float dy = static_cast<float>(p2.y - p1.y);
            float len = sqrt(dx*dx + dy*dy);

            if (len < 1) continue;

            float t = ((mx - p1.x) * dx + (my - p1.y) * dy) / (len * len);
            t = max(0.0f, min(1.0f, t));

            float projX = p1.x + t * dx;
            float projY = p1.y + t * dy;
            float dist = sqrt((mx - projX)*(mx - projX) + (my - projY)*(my - projY));

            if (dist < CABLE_HIT_DISTANCE) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void InvalidateGateRegion(HWND hwnd, const GateInstance& gate) const {
        RECT rect;
        rect.left = gate.x - 25;
        rect.top = gate.y - 10;
        rect.right = gate.x + 80;
        rect.bottom = gate.y + 45;
        InvalidateRect(hwnd, &rect, FALSE);
    }

    string ValidateCircuit() const {
        // Verificar LEDs sin entrada
        for (size_t i = 0; i < gates.size(); i++) {
            if (gates[i].type == GATE_LED) {
                bool hasInput = false;
                for (const auto& cable : cables) {
                    if (cable.gateEnd == static_cast<int>(i)) {
                        hasInput = true;
                        break;
                    }
                }
                if (!hasInput) {
                    return "Advertencia: Hay LEDs sin conexión de entrada";
                }
            }
        }

        // Verificar compuertas sin salidas conectadas
        for (size_t i = 0; i < gates.size(); i++) {
            if (gates[i].type != GATE_LED && gates[i].type != GATE_SWITCH) {
                bool hasOutput = false;
                for (const auto& cable : cables) {
                    if (cable.gateStart == static_cast<int>(i)) {
                        hasOutput = true;
                        break;
                    }
                }
                // Esta es solo una advertencia, no un error crítico
            }
        }

        return "";
    }
};


// FUNCIONES DE DIBUJO

void DrawAND(HDC hdc, int x, int y);
void DrawOR(HDC hdc, int x, int y);
void DrawXOR(HDC hdc, int x, int y);
void DrawNOT(HDC hdc, int x, int y);
void DrawNAND(HDC hdc, int x, int y);
void DrawNOR(HDC hdc, int x, int y);
void DrawXNOR(HDC hdc, int x, int y);

void DrawSwitch(HDC hdc, const GateInstance& gate);
void DrawLED(HDC hdc, const GateInstance& gate);
void DrawGrid(HDC hdc, const RECT& clientRect);

// CLASE PARA RENDERIZADO

class Renderer {
public:
    static void DrawCircuit(HDC hdc, const Circuit& circuit,
                           bool showTempCable, const POINT& tempStart, const POINT& tempEnd) {
        // Dibujar cables
        for (const auto& cable : circuit.cables) {
            if (cable.gateStart < 0 || cable.gateStart >= static_cast<int>(circuit.gates.size()) ||
                cable.gateEnd < 0 || cable.gateEnd >= static_cast<int>(circuit.gates.size())) {
                continue;
            }

            POINT p1 = circuit.GetPinPosition(circuit.gates[cable.gateStart], cable.pinStart);
            POINT p2 = circuit.GetPinPosition(circuit.gates[cable.gateEnd], cable.pinEnd);

            COLORREF color;
            int width;

            if (cable.value == 1) {
                color = RGB(0, 150, 0);
                width = 3;
            } else if (cable.value == 0) {
                color = RGB(150, 0, 0);
                width = 3;
            } else {
                color = RGB(100, 100, 100);
                width = 2;
            }

            GDIGuard cablePen(hdc, CreatePen(PS_SOLID, width, color));
            MoveToEx(hdc, p1.x, p1.y, NULL);
            LineTo(hdc, p2.x, p2.y);
        }

        // Dibujar cable temporal
        if (showTempCable) {
            GDIGuard tempPen(hdc, CreatePen(PS_DOT, 2, RGB(100, 100, 100)));
            MoveToEx(hdc, tempStart.x, tempStart.y, NULL);
            LineTo(hdc, tempEnd.x, tempEnd.y);
        }

        // Dibujar compuertas
        for (const auto& gate : circuit.gates) {
            // Dibujar selección
            if (gate.selected) {
                GDIGuard selectPen(hdc, CreatePen(PS_SOLID, 3, RGB(0, 100, 255)));
                HBRUSH nullBrush = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
                HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, nullBrush));

                Rectangle(hdc, gate.x - 20, gate.y - 5, gate.x + 75, gate.y + 40);

                SelectObject(hdc, oldBrush);
            }

            // Dibujar la compuerta
            switch (gate.type) {
                case GATE_AND: DrawAND(hdc, gate.x, gate.y); break;
                case GATE_OR: DrawOR(hdc, gate.x, gate.y); break;
                case GATE_XOR: DrawXOR(hdc, gate.x, gate.y); break;
                case GATE_NOT: DrawNOT(hdc, gate.x, gate.y); break;
                case GATE_NAND: DrawNAND(hdc, gate.x, gate.y); break;
                case GATE_NOR: DrawNOR(hdc, gate.x, gate.y); break;
                case GATE_XNOR: DrawXNOR(hdc, gate.x, gate.y); break;
                case GATE_SWITCH: DrawSwitch(hdc, gate); break;
                case GATE_LED: DrawLED(hdc, gate); break;
            }

            // Mostrar valor de salida
            if (gate.type != GATE_LED && gate.type != GATE_SWITCH) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0, 0, 200));
                char buf[10];
                sprintf(buf, "%d", gate.val_out);
                TextOut(hdc, gate.out.x + 5, gate.out.y - 15, buf, static_cast<int>(strlen(buf)));
            }
        }
    }
};


// VARIABLES GLOBALES

HINSTANCE hInst;
Circuit circuit;
Mode currentMode = MODE_NORMAL;
GateType selectedGate;
bool cableDrawing = false;
int startGateIndex = -1;
int startPinIndex = -1;
bool draggingGate = false;
int draggedGateIndex = -1;
int dragOffsetX = 0;
int dragOffsetY = 0;
POINT tempCableEnd;
bool showTempCable = false;


// PROCEDIMIENTO DEL DIÁLOGO PRINCIPAL

BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;

        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case ID_BTN_AND:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_AND;
                    SetWindowText(hwndDlg, "Simulador - Colocando AND");
                    break;

                case ID_BTN_NAND:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_NAND;
                    SetWindowText(hwndDlg, "Simulador - Colocando NAND");
                    break;

                case ID_BTN_OR:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_OR;
                    SetWindowText(hwndDlg, "Simulador - Colocando OR");
                    break;

                case ID_BTN_NOR:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_NOR;
                    SetWindowText(hwndDlg, "Simulador - Colocando NOR");
                    break;

                case ID_BTN_NOT:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_NOT;
                    SetWindowText(hwndDlg, "Simulador - Colocando NOT");
                    break;

                case ID_BTN_XOR:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_XOR;
                    SetWindowText(hwndDlg, "Simulador - Colocando XOR");
                    break;

                case ID_BTN_XNOR:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_XNOR;
                    SetWindowText(hwndDlg, "Simulador - Colocando XNOR");
                    break;

                case ID_BTN_CABLE:
                    currentMode = MODE_CABLE;
                    cableDrawing = false;
                    SetWindowText(hwndDlg, "Simulador - Modo Cable");
                    break;

                case ID_BTN_SWITCH:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_SWITCH;
                    SetWindowText(hwndDlg, "Simulador - Colocando SWITCH");
                    break;

                case ID_BTN_LED:
                    currentMode = MODE_PLACING;
                    selectedGate = GATE_LED;
                    SetWindowText(hwndDlg, "Simulador - Colocando LED");
                    break;

                case ID_BTN_DELETE:
                    currentMode = MODE_DELETE;
                    SetWindowText(hwndDlg, "Simulador - Modo Eliminar");
                    break;

                case ID_BTN_CLEAR:
                    circuit.SaveState();
                    circuit.Clear();
                    InvalidateRect(hwndDlg, NULL, TRUE);
                    SetWindowText(hwndDlg, "Simulador de Compuertas Lógicas");
                    break;

                case ID_BTN_EXIT:
                    EndDialog(hwndDlg, 0);
                    break;
            }
            return TRUE;
        }

        case WM_KEYDOWN: {
            if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                if (circuit.Undo()) {
                    InvalidateRect(hwndDlg, NULL, TRUE);
                }
            } else if (wParam == 'Y' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                if (circuit.Redo()) {
                    InvalidateRect(hwndDlg, NULL, TRUE);
                }
            } else if (wParam == VK_ESCAPE) {
                currentMode = MODE_NORMAL;
                cableDrawing = false;
                showTempCable = false;
                SetWindowText(hwndDlg, "Simulador de Compuertas Lógicas");
                InvalidateRect(hwndDlg, NULL, TRUE);
            } else if (wParam == VK_DELETE) {
                circuit.SaveState();

                // Eliminar compuertas seleccionadas
                for (int i = static_cast<int>(circuit.gates.size()) - 1; i >= 0; i--) {
                    if (circuit.gates[i].selected) {
                        // Eliminar cables conectados a esta compuerta
                        for (int j = static_cast<int>(circuit.cables.size()) - 1; j >= 0; j--) {
                            if (circuit.cables[j].gateStart == i || circuit.cables[j].gateEnd == i) {
                                circuit.cables.erase(circuit.cables.begin() + j);
                            }
                        }
                        circuit.gates.erase(circuit.gates.begin() + i);
                    }
                }

                circuit.PropagateSignals();
                InvalidateRect(hwndDlg, NULL, TRUE);
            }
            return TRUE;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwndDlg, &ps);

            RECT clientRect;
            GetClientRect(hwndDlg, &clientRect);

            // Double buffering mejorado
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

            // Fondo
            HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 245));
            FillRect(memDC, &clientRect, bgBrush);
            DeleteObject(bgBrush);

            // Cuadrícula
            DrawGrid(memDC, clientRect);

            // Dibujar circuito
            POINT tempStart = {0, 0};
            if (showTempCable && cableDrawing && startGateIndex >= 0) {
                tempStart = circuit.GetPinPosition(circuit.gates[startGateIndex], startPinIndex);
            }

            Renderer::DrawCircuit(memDC, circuit, showTempCable, tempStart, tempCableEnd);

            // Copiar al HDC principal
            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

            // Limpiar
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwndDlg, &ps);
            return TRUE;
        }

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // Modo eliminar
            if (currentMode == MODE_DELETE) {
                int gateIdx = circuit.HitTestGate(x, y);
                if (gateIdx != -1) {
                    circuit.SaveState();

                    // Eliminar cables conectados
                    for (int i = static_cast<int>(circuit.cables.size()) - 1; i >= 0; i--) {
                        if (circuit.cables[i].gateStart == gateIdx || circuit.cables[i].gateEnd == gateIdx) {
                            circuit.cables.erase(circuit.cables.begin() + i);
                        }
                    }

                    circuit.gates.erase(circuit.gates.begin() + gateIdx);
                    circuit.PropagateSignals();
                    InvalidateRect(hwndDlg, NULL, TRUE);
                    return TRUE;
                }

                int cableIdx = circuit.HitTestCable(x, y);
                if (cableIdx != -1) {
                    circuit.SaveState();
                    circuit.cables.erase(circuit.cables.begin() + cableIdx);
                    circuit.PropagateSignals();
                    InvalidateRect(hwndDlg, NULL, TRUE);
                    return TRUE;
                }
                return TRUE;
            }

            // Modo colocar compuerta
            if (currentMode == MODE_PLACING) {
                circuit.SaveState();

                GateInstance gate;
                gate.type = selectedGate;
                gate.x = x;
                gate.y = y;

                circuit.ComputePins(gate);
                circuit.gates.push_back(gate);
                circuit.PropagateSignals();

                currentMode = MODE_NORMAL;
                SetWindowText(hwndDlg, "Simulador de Compuertas Lógicas");
                InvalidateRect(hwndDlg, NULL, TRUE);
                return TRUE;
            }

            // Modo cable
            if (currentMode == MODE_CABLE) {
                if (!cableDrawing) {
                    // Iniciar cable
                    int gIndex, pIndex;
                    POINT snapped;

                    if (circuit.SnapToPin(x, y, gIndex, pIndex, snapped)) {
                        startGateIndex = gIndex;
                        startPinIndex = pIndex;
                        cableDrawing = true;
                        showTempCable = true;
                        tempCableEnd.x = x;
                        tempCableEnd.y = y;
                    }
                    return TRUE;
                } else {
                    // Finalizar cable
                    int gIndex, pIndex;
                    POINT snapped;

                    if (circuit.SnapToPin(x, y, gIndex, pIndex, snapped)) {
                        // Validar conexión
                        if (circuit.IsValidConnection(startGateIndex, startPinIndex, gIndex, pIndex)) {
                            circuit.SaveState();

                            Cable cable;
                            cable.gateStart = startGateIndex;
                            cable.pinStart = startPinIndex;
                            cable.gateEnd = gIndex;
                            cable.pinEnd = pIndex;

                            circuit.cables.push_back(cable);

                            if (circuit.DetectLoop()) {
                                MessageBox(hwndDlg,
                                    "Advertencia: Se detecto un ciclo en el circuito.\n"
                                    "Esto puede causar un comportamiento inestable.",
                                    "Bucle Detectado",
                                    MB_OK | MB_ICONWARNING);
                            }

                            circuit.PropagateSignals();
                        } else {
                            MessageBox(hwndDlg,
                                "Conexion invalida:\n"
                                "- Las salidas solo pueden conectarse a entradas\n"
                                "- No se puede conectar una entrada ya ocupada\n"
                                "- No se pueden crear conexiones duplicadas",
                                "Conexion Invalida",
                                MB_OK | MB_ICONEXCLAMATION);
                        }

                        cableDrawing = false;
                        showTempCable = false;
                        currentMode = MODE_NORMAL;
                        SetWindowText(hwndDlg, "Simulador de Compuertas Logicas");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                    }
                    return TRUE;
                }
            }

            // Modo normal - interacción con compuertas
            int gateIndex = circuit.HitTestGate(x, y);
            if (gateIndex != -1) {
                GateInstance& gate = circuit.gates[gateIndex];

                // Toggle switch
                if (gate.type == GATE_SWITCH) {
                    circuit.SaveState();
                    gate.val_out = (gate.val_out == 1) ? 0 : 1;
                    circuit.PropagateSignals();
                    InvalidateRect(hwndDlg, NULL, TRUE);
                    return TRUE;
                }

                // Iniciar arrastre
                draggingGate = true;
                draggedGateIndex = gateIndex;
                dragOffsetX = x - circuit.gates[gateIndex].x;
                dragOffsetY = y - circuit.gates[gateIndex].y;
                return TRUE;
            }

            return TRUE;
        }

        case WM_MOUSEMOVE: {
            // Actualizar cable temporal
            if (showTempCable && cableDrawing) {
                tempCableEnd.x = LOWORD(lParam);
                tempCableEnd.y = HIWORD(lParam);
                InvalidateRect(hwndDlg, NULL, TRUE);
            }

            // Arrastrar compuerta
            if (draggingGate && draggedGateIndex >= 0) {
                int mx = LOWORD(lParam);
                int my = HIWORD(lParam);

                GateInstance& gate = circuit.gates[draggedGateIndex];
                gate.x = mx - dragOffsetX;
                gate.y = my - dragOffsetY;

                circuit.ComputePins(gate);
                circuit.PropagateSignals();
                InvalidateRect(hwndDlg, NULL, TRUE);
            }

            return TRUE;
        }

        case WM_LBUTTONUP: {
            if (draggingGate) {
                draggingGate = false;
                draggedGateIndex = -1;
                return TRUE;
            }
            return TRUE;
        }
    }
    return FALSE;
}


// PUNTO DE ENTRADA

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nShowCmd) {
    hInst = hInstance;
    InitCommonControls();
    return DialogBox(hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)DlgMain);
}


// IMPLEMENTACIÓN DE FUNCIONES DE DIBUJO

void DrawAND(HDC hdc, int x, int y) {
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x + 30, y);
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x, y + 30);
    MoveToEx(hdc, x, y + 30, NULL);
    LineTo(hdc, x + 30, y + 30);
    Arc(hdc, x + 15, y, x + 45, y + 30, x + 30, y + 30, x + 30, y);

    MoveToEx(hdc, x - 15, y + 10, NULL);
    LineTo(hdc, x, y + 10);
    MoveToEx(hdc, x - 15, y + 20, NULL);
    LineTo(hdc, x, y + 20);
    MoveToEx(hdc, x + 45, y + 15, NULL);
    LineTo(hdc, x + 60, y + 15);
}

void DrawOR(HDC hdc, int x, int y) {
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x + 30, y);
    MoveToEx(hdc, x, y + 30, NULL);
    LineTo(hdc, x + 30, y + 30);

    Arc(hdc, x - 10, y, x + 10, y + 30, x, y + 30, x, y);
    Arc(hdc, x + 15, y, x + 45, y + 30, x + 30, y + 30, x + 30, y);

    MoveToEx(hdc, x - 10, y + 10, NULL);
    LineTo(hdc, x + 7, y + 10);
    MoveToEx(hdc, x - 10, y + 20, NULL);
    LineTo(hdc, x + 7, y + 20);
    MoveToEx(hdc, x + 45, y + 15, NULL);
    LineTo(hdc, x + 60, y + 15);
}

void DrawXOR(HDC hdc, int x, int y) {
    DrawOR(hdc, x, y);
    Arc(hdc, x - 17, y, x + 3, y + 30, x - 6, y + 30, x - 6, y);
}

void DrawNOT(HDC hdc, int x, int y) {
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x, y + 30);
    LineTo(hdc, x + 30, y + 15);
    LineTo(hdc, x, y);

    Ellipse(hdc, x + 30, y + 12, x + 36, y + 18);

    MoveToEx(hdc, x - 15, y + 15, NULL);
    LineTo(hdc, x, y + 15);
    MoveToEx(hdc, x + 36, y + 15, NULL);
    LineTo(hdc, x + 50, y + 15);
}

void DrawNAND(HDC hdc, int x, int y) {
    DrawAND(hdc, x, y);
    Ellipse(hdc, x + 45, y + 12, x + 51, y + 18);
    MoveToEx(hdc, x + 51, y + 15, NULL);
    LineTo(hdc, x + 65, y + 15);
}

void DrawNOR(HDC hdc, int x, int y) {
    DrawOR(hdc, x, y);
    Ellipse(hdc, x + 45, y + 12, x + 51, y + 18);
    MoveToEx(hdc, x + 51, y + 15, NULL);
    LineTo(hdc, x + 65, y + 15);
}

void DrawXNOR(HDC hdc, int x, int y) {
    DrawXOR(hdc, x, y);
    Ellipse(hdc, x + 45, y + 12, x + 51, y + 18);
    MoveToEx(hdc, x + 51, y + 15, NULL);
    LineTo(hdc, x + 65, y + 15);
}

void DrawSwitch(HDC hdc, const GateInstance& gate) {
    COLORREF color = (gate.val_out == 1) ? RGB(0, 200, 0) : RGB(150, 150, 150);
    GDIGuard brush(hdc, CreateSolidBrush(color));

    Rectangle(hdc, gate.x, gate.y, gate.x + 40, gate.y + 30);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    const char* txt = (gate.val_out == 1) ? "ON" : "OFF";
    TextOut(hdc, gate.x + 8, gate.y + 8, txt, static_cast<int>(strlen(txt)));

    MoveToEx(hdc, gate.out.x, gate.out.y, NULL);
    LineTo(hdc, gate.out.x - 15, gate.out.y);
}

void DrawLED(HDC hdc, const GateInstance& gate) {
    COLORREF color = (gate.val_in1 == 1) ? RGB(0, 255, 0) : RGB(255, 0, 0);
    GDIGuard brush(hdc, CreateSolidBrush(color));

    Ellipse(hdc, gate.x, gate.y, gate.x + 30, gate.y + 30);

    MoveToEx(hdc, gate.in1.x + 15, gate.in1.y, NULL);
    LineTo(hdc, gate.in1.x, gate.in1.y);
}

void DrawGrid(HDC hdc, const RECT& clientRect) {
    GDIGuard gridPen(hdc, CreatePen(PS_DOT, 1, RGB(220, 220, 220)));

    for (int x = 0; x < clientRect.right; x += GRID_SIZE) {
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, clientRect.bottom);
    }

    for (int y = 0; y < clientRect.bottom; y += GRID_SIZE) {
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, clientRect.right, y);
    }
}
