#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <vector>
#include "resource.h"

void DrawAND(HDC hdc, int x, int y);
void DrawOR(HDC hdc, int x, int y);
void DrawXOR(HDC hdc, int x, int y);
void DrawNOT(HDC hdc, int x, int y);
void DrawNAND(HDC hdc, int x, int y);
void DrawNOR(HDC hdc, int x, int y);
void DrawXNOR(HDC hdc, int x, int y);

bool cableMode = false;     // ¿El usuario está colocando un cable?
bool cableDrawing = false;  // ¿Ya se hizo el primer clic?
POINT cableStart;           // Punto inicial del cable
POINT cableEnd;             // Punto final del cable

// Para el sistema de cables basados en pines
int startGateIndex = -1;
int startPinIndex  = -1;

struct GatePin {
    int x;
    int y;
};

struct Gate {
    int x, y;          // posición de la compuerta
    GatePin in1;
    GatePin in2;
    GatePin out;
};

Gate gates[7];

bool IsNear(int mx, int my, GatePin pin)
{
    int dx = mx - pin.x;
    int dy = my - pin.y;
    return (dx*dx + dy*dy) < 100; // radio 10 px
}


struct Cable {
    int gateStart;   // índice de compuerta origen
    int pinStart;    // 0 = in1, 1 = in2, 2 = out

    int gateEnd;     // índice de compuerta destino
    int pinEnd;      // 0 = in1, 1 = in2, 2 = out

    int value = -1; // valor lógico transportado

};



std::vector<Cable> cables;

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

struct GateInstance {
    GateType type;
    int x;
    int y;
    POINT in1;
    POINT in2;
    POINT out;

    // valores lógicos
    int val_in1 = -1;   // -1 = sin conexión
    int val_in2 = -1;
    int val_out = -1;

};

POINT GetPinPosition(const GateInstance &g, int pin)
{
    switch(pin)
    {
        case 0: return g.in1;
        case 1: return g.in2;
        case 2: return g.out;
    }
    POINT p = {0,0};
    return p;
}

std::vector<GateInstance> placedGates;

bool placingGate = false;
GateType selectedGate;

void ComputePins(GateInstance &g)
{
    switch (g.type)
    {
        case GATE_AND:
        case GATE_NAND:
            g.in1 = { g.x - 15, g.y + 10 };
            g.in2 = { g.x - 15, g.y + 20 };
            g.out = { g.x + 60, g.y + 15 };
            break;

        case GATE_OR:
        case GATE_NOR:
            g.in1 = { g.x - 10, g.y + 10 };
            g.in2 = { g.x - 10, g.y + 20 };
            g.out = { g.x + 60, g.y + 15 };
            break;

        case GATE_XOR:
        case GATE_XNOR:
            g.in1 = { g.x - 10, g.y + 10 };
            g.in2 = { g.x - 10, g.y + 20 };
            g.out = { g.x + 60, g.y + 15 };
            break;

        case GATE_NOT:
            g.in1 = { g.x - 15, g.y + 15 };
            g.out = { g.x + 50, g.y + 15 };
            break;

        case GATE_SWITCH:
            g.in1 = {0,0};
            g.in2 = {0,0};
            g.out = { g.x + 40, g.y + 15 };
            break;

        case GATE_LED:
            g.in1 = { g.x - 10, g.y + 15 };
            g.in2 = {0,0};
            g.out = {0,0};
            break;

    }
}

void DrawSwitch(HDC hdc, const GateInstance &g) // Switch ON/OFF
{
    Rectangle(hdc, g.x, g.y, g.x + 40, g.y + 30);

    const char *txt = (g.val_out == 1) ? "ON" : "OFF";
    TextOut(hdc, g.x + 10, g.y + 8, txt, strlen(txt));

    // salida
    MoveToEx(hdc, g.out.x, g.out.y, NULL);
    LineTo(hdc, g.out.x + 15, g.out.y);
}

void DrawLED(HDC hdc, const GateInstance &g) // Led rojo/verde
{
    HBRUSH brush;

    if (g.val_in1 == 1)
        brush = CreateSolidBrush(RGB(0, 255, 0)); // verde
    else
        brush = CreateSolidBrush(RGB(255, 0, 0)); // rojo

    HBRUSH old = (HBRUSH)SelectObject(hdc, brush);

    Ellipse(hdc, g.x, g.y, g.x + 30, g.y + 30);

    SelectObject(hdc, old);
    DeleteObject(brush);

    // entrada
    MoveToEx(hdc, g.in1.x, g.in1.y, NULL);
    LineTo(hdc, g.in1.x - 15, g.in1.y);
}

bool SnapToPin(int mx, int my, int &gateIndex, int &pinIndex, POINT &result)
{
    for (int i = 0; i < placedGates.size(); i++)
    {
        GateInstance &g = placedGates[i];

        // LED primero (solo entrada)
        if (g.type == GATE_LED)
        {
            int dx = mx - g.in1.x;
            int dy = my - g.in1.y;
            if (dx*dx + dy*dy < 100)
            {
                gateIndex = i;
                pinIndex = 0;
                result = g.in1;
                return true;
            }
            continue;
        }

        // SWITCH después (solo salida)
        if (g.type == GATE_SWITCH)
        {
            int dx = mx - g.out.x;
            int dy = my - g.out.y;
            if (dx*dx + dy*dy < 100)
            {
                gateIndex = i;
                pinIndex = 2;
                result = g.out;
                return true;
            }
            continue;
        }

        // Compuertas normales al final

        // Entrada 1
        int dx = mx - g.in1.x;
        int dy = my - g.in1.y;
        if (dx*dx + dy*dy < 100)
        {
            gateIndex = i;
            pinIndex = 0;
            result = g.in1;
            return true;
        }

        // Entrada 2
        if (g.type != GATE_NOT)
        {
            dx = mx - g.in2.x;
            dy = my - g.in2.y;
            if (dx*dx + dy*dy < 100)
            {
                gateIndex = i;
                pinIndex = 1;
                result = g.in2;
                return true;
            }
        }

        // Salida
        dx = mx - g.out.x;
        dy = my - g.out.y;
        if (dx*dx + dy*dy < 100)
        {
            gateIndex = i;
            pinIndex = 2;
            result = g.out;
            return true;
        }
    }

    return false;
}

bool draggingGate = false;
int draggedGateIndex = -1;
int dragOffsetX = 0;
int dragOffsetY = 0;

int HitTestGate(int mx, int my)
{
    for (int i = 0; i < placedGates.size(); i++)
    {
        GateInstance &g = placedGates[i];

        // Tamaño aproximado de cada compuerta
        int w = 60;
        int h = 30;

        if (mx >= g.x && mx <= g.x + w &&
            my >= g.y && my <= g.y + h)
        {
            return i; // índice de la compuerta
        }
    }
    return -1; // no se tocó ninguna
}

int EvaluateGate(const GateInstance &g) // Evaluar una compuerta
{
    switch (g.type)
    {
        case GATE_AND:
            return (g.val_in1 == 1 && g.val_in2 == 1) ? 1 : 0;

        case GATE_OR:
            return (g.val_in1 == 1 || g.val_in2 == 1) ? 1 : 0;

        case GATE_XOR:
            return (g.val_in1 != g.val_in2) ? 1 : 0;

        case GATE_NOT:
            return (g.val_in1 == 1) ? 0 : 1;

        case GATE_NAND:
            return !((g.val_in1 == 1 && g.val_in2 == 1) ? 1 : 0);

        case GATE_NOR:
            return !((g.val_in1 == 1 || g.val_in2 == 1) ? 1 : 0);

        case GATE_XNOR:
            return (g.val_in1 == g.val_in2) ? 1 : 0;
    }
    return -1;
}

void PropagateSignals()
{
    // Primero, limpiar todas las entradas
    for (auto &g : placedGates)
    {
        g.val_in1 = -1;
        g.val_in2 = -1;
    }

    // SWITCHES: mantener su valor de salida
    // (este es el código del paso 8, parte 1)
    for (auto &g : placedGates)
    {
        if (g.type == GATE_SWITCH)
        {
            // El switch ya tiene val_out asignado por el usuario
            // No se recalcula, solo se conserva
            g.val_out = g.val_out;
        }
    }

    // Propagar valores desde las salidas hacia las entradas
    for (auto &c : cables)
    {
        GateInstance &src = placedGates[c.gateStart];
        GateInstance &dst = placedGates[c.gateEnd];

        // Obtener valor de salida del gate origen
        int val = src.val_out;
        c.value = val;

        // Asignar al pin destino
        if (c.pinEnd == 0) dst.val_in1 = val;
        if (c.pinEnd == 1) dst.val_in2 = val;
    }

    // Recalcular salidas
    for (auto &g : placedGates)
    {
        if (g.type == GATE_LED)
        {
            // El LED solo recibe val_in1
            // No produce salida
            continue;
        }

        if (g.type == GATE_SWITCH)
        {
            // El switch ya tiene val_out asignado por el usuario
            continue;
        }

        // Compuertas lógicas normales
        g.val_out = EvaluateGate(g);
    }
}

HINSTANCE hInst;

BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        // AND en (50,100)
        gates[0].x = 50; gates[0].y = 100;
        gates[0].in1 = {50 - 15, 100 + 10};
        gates[0].in2 = {50 - 15, 100 + 20};
        gates[0].out = {50 + 60, 100 + 15};

        // NAND en (175,100)
        gates[1].x = 175; gates[1].y = 100;
        gates[1].in1 = {175 - 15, 100 + 10};
        gates[1].in2 = {175 - 15, 100 + 20};
        gates[1].out = {175 + 65, 100 + 15};

        // OR en (290,100)
        gates[2].x = 290; gates[2].y = 100;
        gates[2].in1 = {290 - 10, 100 + 10};
        gates[2].in2 = {290 - 10, 100 + 20};
        gates[2].out = {290 + 60, 100 + 15};

        // NOR en (410,100)
        gates[3].x = 410; gates[3].y = 100;
        gates[3].in1 = {410 - 10, 100 + 10};
        gates[3].in2 = {410 - 10, 100 + 20};
        gates[3].out = {410 + 65, 100 + 15};

        // NOT en (540,100)
        gates[4].x = 540; gates[4].y = 100;
        gates[4].in1 = {540 - 15, 100 + 15};
        gates[4].out = {540 + 50, 100 + 15};

        // XOR en (650,100)
        gates[5].x = 650; gates[5].y = 100;
        gates[5].in1 = {650 - 10, 100 + 10};
        gates[5].in2 = {650 - 10, 100 + 20};
        gates[5].out = {650 + 60, 100 + 15};

        // XNOR en (770,100)
        gates[6].x = 770; gates[6].y = 100;
        gates[6].in1 = {770 - 10, 100 + 10};
        gates[6].in2 = {770 - 10, 100 + 20};
        gates[6].out = {770 + 65, 100 + 15};

    }
    return TRUE;

    case WM_CLOSE:
    {
        EndDialog(hwndDlg, 0);
    }
    return TRUE;

    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
            case ID_BTN_AND:
                placingGate = true;
                selectedGate = GATE_AND;
                MessageBox(hwndDlg, "Compuerta AND seleccionada", "Simulador", MB_OK);
                break;

            case ID_BTN_NAND:
                placingGate = true;
                selectedGate = GATE_NAND;
                MessageBox(hwndDlg, "Compuerta NAND seleccionada", "Simulador", MB_OK);
                break;

            case ID_BTN_OR:
                placingGate = true;
                selectedGate = GATE_OR;
                MessageBox(hwndDlg, "Compuerta OR selecionada", "Simulador", MB_OK);
                break;

            case ID_BTN_NOR:
                placingGate = true;
                selectedGate = GATE_NOR;
                MessageBox(hwndDlg, "Compuerta NOR selecionada", "Simulador", MB_OK);
                break;

            case ID_BTN_NOT:
                placingGate = true;
                selectedGate = GATE_NOT;
                MessageBox(hwndDlg, "Compuerta NOT selecionada", "Simulador", MB_OK);
                break;

            case ID_BTN_XOR:
                placingGate = true;
                selectedGate = GATE_XOR;
                MessageBox(hwndDlg, "Compuerta XOR selecionada", "Simulador", MB_OK);
                break;

            case ID_BTN_XNOR:
                placingGate = true;
                selectedGate = GATE_XNOR;
                MessageBox(hwndDlg, "Compuerta XNOR selecionada", "Simulador", MB_OK);
                break;

            case ID_BTN_CABLE:
                cableMode = true;
                cableDrawing = false;
                MessageBox(hwndDlg, "Modo cable activado.\nHaga clic en un punto de salida.", "Cable", MB_OK);
                break;

            case ID_BTN_SWITCH:
                placingGate = true;
                selectedGate = GATE_SWITCH;
                MessageBox(hwndDlg, "Haga clic para colocar un SWITCH", "Simulador", MB_OK);
                break;

            case ID_BTN_LED:
                placingGate = true;
                selectedGate = GATE_LED;
                MessageBox(hwndDlg, "Haga clic para colocar un LED", "Simulador", MB_OK);
                break;

            case ID_BTN_EXIT:
                EndDialog(hwndDlg, 0);
                break;
        }
    }
    return TRUE;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwndDlg, &ps);

        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0,0,0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        DrawAND(hdc, 50, 100);
        DrawOR(hdc, 290, 100);
        DrawXOR(hdc, 650, 100);
        DrawNOT(hdc, 540, 100);
        DrawNAND(hdc, 175, 100);
        DrawNOR(hdc, 410, 100);
        DrawXNOR(hdc, 770, 100);

        for (const GateInstance &g : placedGates)
        {
            switch (g.type)
            {
                case GATE_AND:  DrawAND(hdc, g.x, g.y); break;
                case GATE_OR:   DrawOR(hdc, g.x, g.y); break;
                case GATE_XOR:  DrawXOR(hdc, g.x, g.y); break;
                case GATE_NOT:  DrawNOT(hdc, g.x, g.y); break;
                case GATE_NAND: DrawNAND(hdc, g.x, g.y); break;
                case GATE_NOR:  DrawNOR(hdc, g.x, g.y); break;
                case GATE_XNOR: DrawXNOR(hdc, g.x, g.y); break;

                case GATE_SWITCH: DrawSwitch(hdc, g); break;
                case GATE_LED:    DrawLED(hdc, g); break;

            }

            // Valor de las compuertas
            char buf[10];
            sprintf(buf, "%d", g.val_out);
            TextOut(hdc, g.out.x + 5, g.out.y - 10, buf, strlen(buf));
        }



        // Dibujar todos los cables guardados
        for (const Cable &c : cables)
        {
            POINT p1 = GetPinPosition(placedGates[c.gateStart], c.pinStart);
            POINT p2 = GetPinPosition(placedGates[c.gateEnd], c.pinEnd);

            MoveToEx(hdc, p1.x, p1.y, NULL);
            LineTo(hdc, p2.x, p2.y);
        }

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        EndPaint(hwndDlg, &ps);
    }
    return TRUE;

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        int gateIndex = HitTestGate(x, y);

        if (gateIndex != -1)
        {
            GateInstance &g = placedGates[gateIndex];

            if (g.type == GATE_SWITCH)
            {
                g.val_out = (g.val_out == 1 ? 0 : 1); // toggle
                PropagateSignals();
                InvalidateRect(hwndDlg, NULL, TRUE);
                return TRUE;
            }

            draggingGate = true;
            draggedGateIndex = gateIndex;

            // Guardar offset para que no salte
            dragOffsetX = x - placedGates[gateIndex].x;
            dragOffsetY = y - placedGates[gateIndex].y;

            return TRUE;
        }

        if (placingGate)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            GateInstance g;
            g.type = selectedGate;
            g.x = x;
            g.y = y;

            ComputePins(g);  // Calcular pines dinámicamente

            placedGates.push_back(g);

            PropagateSignals();

            placingGate = false;

            InvalidateRect(hwndDlg, NULL, TRUE);
            return TRUE;
        }

        if (!cableMode)
            break;



        POINT snapped;

        if (!cableDrawing)
        {
            int gIndex, pIndex;
            POINT snapped;

            if (SnapToPin(x, y, gIndex, pIndex, snapped))
            {
                startGateIndex = gIndex;
                startPinIndex = pIndex;
            }
            else
            {
                // clic inválido: no está sobre un pin
                return TRUE;
            }

            cableDrawing = true;
            return TRUE;
        }

        else
        {
            int gIndex, pIndex;
            POINT snapped;

            if (!SnapToPin(x, y, gIndex, pIndex, snapped))
                return TRUE; // clic inválido

            Cable c;
            c.gateStart = startGateIndex;
            c.pinStart = startPinIndex;
            c.gateEnd = gIndex;
            c.pinEnd = pIndex;

            cables.push_back(c);

            PropagateSignals();

            cableDrawing = false;
            cableMode = false;

            InvalidateRect(hwndDlg, NULL, TRUE);
            return TRUE;
        }
    }
    return TRUE;

    case WM_MOUSEMOVE:
    {
        if (draggingGate)
        {
            int mx = LOWORD(lParam);
            int my = HIWORD(lParam);

            GateInstance &g = placedGates[draggedGateIndex];

            // Nueva posición
            g.x = mx - dragOffsetX;
            g.y = my - dragOffsetY;

            // Recalcular pines
            ComputePins(g);

            PropagateSignals();

            // Redibujar
            InvalidateRect(hwndDlg, NULL, TRUE);
        }
    }
    return TRUE;

    case WM_LBUTTONUP:
    {
        if (draggingGate)
        {
            draggingGate = false;
            draggedGateIndex = -1;
            return TRUE;
        }
    }
    break;

    }
    return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    hInst=hInstance;
    InitCommonControls();
    return DialogBox(hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)DlgMain);
}

void DrawAND(HDC hdc, int x, int y)
{
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x + 30, y);

    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x, y + 30);

    MoveToEx(hdc, x, y + 30, NULL);
    LineTo(hdc, x + 30, y + 30);

    // Semicírculo derecho
    Arc(hdc, x + 15, y, x + 45, y + 30, x + 30, y + 30, x + 30, y);

    // Entradas
    MoveToEx(hdc, x - 15, y + 10, NULL);
    LineTo(hdc, x, y + 10);

    MoveToEx(hdc, x - 15, y + 20, NULL);
    LineTo(hdc, x, y + 20);

    // Salida
    MoveToEx(hdc, x + 45, y + 15, NULL);
    LineTo(hdc, x + 60, y + 15);
}

void DrawOR(HDC hdc, int x, int y)
{

    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x + 30, y);

    MoveToEx(hdc, x, y + 30, NULL);
    LineTo(hdc, x + 30, y + 30);

    // Semicírculo izquierdo
    Arc(hdc, x - 10, y, x + 10, y + 30, x, y + 30, x, y);

    // Semicírculo derecho
    Arc(hdc, x + 15, y, x + 45, y + 30, x + 30, y + 30, x + 30, y);

    // Entradas
    MoveToEx(hdc, x - 10, y + 10, NULL);
    LineTo(hdc, x + 7, y + 10);

    MoveToEx(hdc, x - 10, y + 20, NULL);
    LineTo(hdc, x + 7, y + 20);

    // Salida
    MoveToEx(hdc, x + 45, y + 15, NULL);
    LineTo(hdc, x + 60, y + 15);
}

void DrawXOR(HDC hdc, int x, int y)
{
    DrawOR(hdc, x, y);

    // Semicírculo izquierdo
    Arc(hdc, x - 17, y, x + 3, y + 30, x - 6, y + 30, x - 6, y);

}

void DrawNOT(HDC hdc, int x, int y)
{
    // Triángulo
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x, y + 30);
    LineTo(hdc, x + 30, y + 15);
    LineTo(hdc, x, y);

    // Círculo de negación
    Ellipse(hdc, x + 30, y + 12, x + 36, y + 18);

    // Entrada
    MoveToEx(hdc, x - 15, y + 15, NULL);
    LineTo(hdc, x, y + 15);

    // Salida
    MoveToEx(hdc, x + 36, y + 15, NULL);
    LineTo(hdc, x + 50, y + 15);
}

void DrawNAND(HDC hdc, int x, int y)
{
    DrawAND(hdc, x, y);

    // Círculo de negación
    Ellipse(hdc, x + 45, y + 12, x + 51, y + 18);

    // Salida corregida
    MoveToEx(hdc, x + 51, y + 15, NULL);
    LineTo(hdc, x + 65, y + 15);
}

void DrawNOR(HDC hdc, int x, int y)
{
    DrawOR(hdc, x, y);

    // Círculo de negación
    Ellipse(hdc, x + 45, y + 12, x + 51, y + 18);

    // Salida corregida
    MoveToEx(hdc, x + 51, y + 15, NULL);
    LineTo(hdc, x + 65, y + 15);
}

void DrawXNOR(HDC hdc, int x, int y)
{
    DrawXOR(hdc, x, y);

    // Círculo de negación
    Ellipse(hdc, x + 45, y + 12, x + 51, y + 18);

    // Salida corregida
    MoveToEx(hdc, x + 51, y + 15, NULL);
    LineTo(hdc, x + 65, y + 15);
}
