#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <set>
#include <cmath>
#include <string>
<<<<<<< Updated upstream
#include <ctime>
#include <queue>
=======
#include <map>
#include <cctype>

>>>>>>> Stashed changes
#include "resource.h"

using namespace std;

constexpr int MAX_UNDO_STACK = 50;
constexpr int PIN_SNAP_DISTANCE = 100;
constexpr int GATE_WIDTH = 70;
constexpr int GATE_HEIGHT = 35;
constexpr int GRID_SIZE = 20;
constexpr int CABLE_HIT_DISTANCE = 5;

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

    GDIGuard(const GDIGuard&) = delete;
    GDIGuard& operator=(const GDIGuard&) = delete;
};

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

<<<<<<< Updated upstream
=======
// Estructuras para Karnaugh

struct Literal {
    char var;     // 'A', 'B', 'C'
    bool negated; // true si es A'
};

struct Term {
    vector<Literal> literals;
};

struct BooleanExpression {
    vector<Term> terms; // OR entre términos
    set<char> vars;       // Variables detectadas
};

struct KarnaughMap {
    int numVars;                 // 2, 3 o 4
    vector<char> vars;           // {'A','B','C'}
    vector<int> cells;           // tamaño = 2^numVars
};

struct KGroup {
    vector<int> cells;   // índices de celdas agrupadas
    bool wraps = false;   // NUEVO: cruza bordes
};

// Vecinos validos

bool AreAdjacent(int a, int b, int numVars) {

    int diff = a ^ b;
    if (diff && ((diff & (diff - 1)) == 0))
        return true; // adyacencia normal

    // ---- WRAP HORIZONTAL ----
    int cols = 1 << ((numVars + 1) / 2);

    int rowA = a / cols;
    int colA = a % cols;
    int rowB = b / cols;
    int colB = b % cols;

    if (rowA == rowB) {
        if ((colA == 0 && colB == cols - 1) ||
            (colB == 0 && colA == cols - 1))
            return true;
    }

    return false;
}


int PopCount(int x) {
    int count = 0;
    while (x) {
        x &= (x - 1);
        count++;
    }
    return count;
}


// Pares
vector<KGroup> FindPairs(const KarnaughMap& km) {

    vector<KGroup> groups;

    int size = km.cells.size();
    for (int i = 0; i < size; i++) {
        if (km.cells[i] == 0) continue;

        for (int j = i + 1; j < size; j++) {
            if (km.cells[j] == 0) continue;

            if (AreAdjacent(i, j, km.numVars)) {
                KGroup g;
                g.cells.push_back(i);
                g.cells.push_back(j);
                groups.push_back(g);
            }
        }
    }
    int cols = 1 << ((km.numVars + 1) / 2);

    for (auto& g : groups) {
        bool left = false, right = false;

        for (int idx : g.cells) {
            int col = idx % cols;
            if (col == 0) left = true;
            if (col == cols - 1) right = true;
        }

        if (left && right)
            g.wraps = true;
    }


    return groups;
}

// Cuartetos
vector<KGroup> FindQuads(const KarnaughMap& km) {

    vector<KGroup> groups;
    int size = km.cells.size();

    for (int a = 0; a < size; a++) {
        if (km.cells[a] == 0) continue;

        for (int b = a + 1; b < size; b++) {
            if (km.cells[b] == 0) continue;
            if (!AreAdjacent(a, b, km.numVars)) continue;

            for (int c = b + 1; c < size; c++) {
                if (km.cells[c] == 0) continue;

                for (int d = c + 1; d < size; d++) {
                    if (km.cells[d] == 0) continue;

                    // Verificar que TODOS difieran solo en 2 bits
                    int mask = (a ^ b) | (a ^ c) | (a ^ d);
                    if (PopCount(mask) == 2) {
                        KGroup g;
                        g.cells = {a, b, c, d};
                        groups.push_back(g);
                    }
                }
            }
        }
    }
    return groups;
}

// Funcion maestra
vector<KGroup> GroupKarnaugh(const KarnaughMap& km) {

    vector<KGroup> result;

    // Prioridad: grupos grandes
    vector<KGroup> quads = FindQuads(km);
    vector<KGroup> pairs = FindPairs(km);

    result.insert(result.end(), quads.begin(), quads.end());
    result.insert(result.end(), pairs.begin(), pairs.end());

    // celdas solas
    for (int i = 0; i < km.cells.size(); i++) {
        if (km.cells[i] == 1) {
            bool used = false;
            for (const auto& g : result) {
                if (find(g.cells.begin(), g.cells.end(), i) != g.cells.end()) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                KGroup single;
                single.cells.push_back(i);
                result.push_back(single);
            }
        }
    }

    return result;
}

BooleanExpression GroupsToExpression(
    const vector<KGroup>& groups,
    const KarnaughMap& km
) {
    BooleanExpression result;

    for (const auto& group : groups) {
        Term term;

        // Tomamos la primera celda como referencia
        int baseIndex = group.cells[0];

        for (int var = 0; var < (int)km.vars.size(); var++) {
            bool same = true;
            int baseValue = (baseIndex >> var) & 1;

            for (int idx : group.cells) {
                int value = (idx >> var) & 1;
                if (value != baseValue) {
                    same = false;
                    break;
                }
            }

            // Si no cambia dentro del grupo, la variable permanece
            if (same) {
                Literal lit;
                lit.var = km.vars[var];
                lit.negated = (baseValue == 0);
                term.literals.push_back(lit);
            }
        }

        result.terms.push_back(term);
    }

    return result;
}

// Reducir expresiones
bool IsTermSubset(const Term& a, const Term& b) {
    // ¿a ⊆ b ?
    for (const auto& litA : a.literals) {
        bool found = false;
        for (const auto& litB : b.literals) {
            if (litA.var == litB.var &&
                litA.negated == litB.negated) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}
// Simplificar
BooleanExpression SimplifyExpression(const BooleanExpression& expr) {

    BooleanExpression simplified;
    simplified.vars = expr.vars;

    for (size_t i = 0; i < expr.terms.size(); i++) {
        bool redundant = false;

        for (size_t j = 0; j < expr.terms.size(); j++) {
            if (i == j) continue;

            // Si otro término más simple lo absorbe
            if (IsTermSubset(expr.terms[j], expr.terms[i]) &&
                expr.terms[j].literals.size() <= expr.terms[i].literals.size()) {
                redundant = true;
                break;
            }
        }

        if (!redundant) {
            simplified.terms.push_back(expr.terms[i]);
        }
    }

    return simplified;
}

string ExprToString(const BooleanExpression& expr) {
    string s;

    for (size_t i = 0; i < expr.terms.size(); i++) {
        if (i > 0) s += " + ";

        for (const auto& lit : expr.terms[i].literals) {
            s += lit.var;
            if (lit.negated) s += "'";
        }
    }
    return s;
}


// Para Karnaugh
BooleanExpression g_simplifiedExpression;
void BuildCircuitFromExpression(const BooleanExpression& expr);

vector<char> GetVariables(const BooleanExpression& expr) {
    vector<char> vars;

    for (const auto& term : expr.terms) {
        for (const auto& lit : term.literals) {
            if (find(vars.begin(), vars.end(), lit.var) == vars.end()) {
                vars.push_back(lit.var);
            }
        }
    }
    return vars;
}

bool EvaluateTerm(const Term& term, const vector<char>& vars, int binaryIndex) {
    // Para cada literal del término
    for (const auto& lit : term.literals) {
        // Encontrar la posición de esta variable en el vector ordenado
        int varPos = -1;
        for (size_t i = 0; i < vars.size(); i++) {
            if (vars[i] == lit.var) {
                varPos = i;
                break;
            }
        }

        if (varPos == -1) continue; // Variable no encontrada (error)

        // Extraer el bit correspondiente del índice binario
        // IMPORTANTE: El bit más significativo es la primera variable
        int bitPosition = vars.size() - 1 - varPos;
        bool bitValue = (binaryIndex >> bitPosition) & 1;

        // Aplicar negación si es necesario
        bool literalValue = lit.negated ? !bitValue : bitValue;

        // Si algún literal es falso, todo el término es falso (AND)
        if (!literalValue) {
            return false;
        }
    }

    // Todos los literales son verdaderos
    return true;
}






// CLASE PRINCIPAL DEL CIRCUITO

>>>>>>> Stashed changes
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
        if (gateA == gateB) return false;

        bool aIsOutput = (pinA == PIN_OUTPUT);
        bool bIsOutput = (pinB == PIN_OUTPUT);

        if (aIsOutput == bIsOutput) return false;

        int startGate = aIsOutput ? gateA : gateB;
        int startPin = aIsOutput ? pinA : pinB;
        int endGate = aIsOutput ? gateB : gateA;
        int endPin = aIsOutput ? pinB : pinA;

        if (gates[endGate].type == GATE_LED && endPin != PIN_INPUT1) return false;

        if (gates[startGate].type == GATE_LED) return false;

        if (gates[startGate].type == GATE_SWITCH && startPin != PIN_OUTPUT) return false;

        for (const auto& cable : cables) {
            if (cable.gateEnd == endGate && cable.pinEnd == endPin) return false;
        }

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
        for (auto& gate : gates) {
            if (gate.type != GATE_SWITCH) {
                gate.val_in1 = -1;
                gate.val_in2 = -1;
            }
        }

        bool changed = true;
        int iterations = 0;
        const int MAX_ITERATIONS = 100;

        while (changed && iterations < MAX_ITERATIONS) {
            changed = false;
            iterations++;

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

            for (auto& gate : gates) {
                if (gate.type == GATE_LED || gate.type == GATE_SWITCH) continue;

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

        return "";
    }
};

<<<<<<< Updated upstream
=======



// Limpiar expresión
string CleanExpression(const string& input) {
    string out;
    for (char c : input) {
        if (c != ' ' && c != '\t') {
            out += toupper(c);
        }
    }
    return out;
}

// Parsear expresión SOP
BooleanExpression ParseExpression(const string& input) {
    BooleanExpression expr;
    string s = CleanExpression(input);

    // Eliminar "F=" si existe
    size_t eqPos = s.find('=');
    if (eqPos != string::npos) {
        s = s.substr(eqPos + 1);
    }

    size_t i = 0;
    Term currentTerm;

    while (i < s.length()) {
        char c = s[i];

        if (c >= 'A' && c <= 'Z') {
            Literal lit;
            lit.var = c;
            lit.negated = false;

            if (i + 1 < s.length() && s[i + 1] == '\'') {
                lit.negated = true;
                i++;
            }

            currentTerm.literals.push_back(lit);
            expr.vars.insert(c);
        }
        else if (c == '+') {
            if (!currentTerm.literals.empty()) {
                expr.terms.push_back(currentTerm);
                currentTerm.literals.clear();
            }
        }

        i++;
    }

    if (!currentTerm.literals.empty()) {
        expr.terms.push_back(currentTerm);
    }

    return expr;
}


// FUNCIONES DE DIBUJO

>>>>>>> Stashed changes
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

class Renderer {
public:
    static void DrawCircuit(HDC hdc, const Circuit& circuit,
                           bool showTempCable, const POINT& tempStart, const POINT& tempEnd) {
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

        if (showTempCable) {
            GDIGuard tempPen(hdc, CreatePen(PS_DOT, 2, RGB(100, 100, 100)));
            MoveToEx(hdc, tempStart.x, tempStart.y, NULL);
            LineTo(hdc, tempEnd.x, tempEnd.y);
        }

        for (const auto& gate : circuit.gates) {
            if (gate.selected) {
                GDIGuard selectPen(hdc, CreatePen(PS_SOLID, 3, RGB(0, 100, 255)));
                HBRUSH nullBrush = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
                HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, nullBrush));
                Rectangle(hdc, gate.x - 20, gate.y - 5, gate.x + 75, gate.y + 40);
                SelectObject(hdc, oldBrush);
            }

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

<<<<<<< Updated upstream
INT_PTR CALLBACK DlgSimulator(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgMath(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgTrees(HWND, UINT, WPARAM, LPARAM);
=======
KarnaughMap g_lastKMap;
vector<KGroup> g_lastGroups;
string g_expressionForSimulation;

void BuildCircuitFromExpression(const BooleanExpression& expr) {
    circuit.Clear();

    if (expr.terms.empty()) {
        MessageBox(NULL, "Expresión vacía", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // 1. Crear SWITCH por cada variable
    map<char, int> switchIndex;
    int x = 50;
    int y = 100;

    vector<char> sortedVars(expr.vars.begin(), expr.vars.end());
    sort(sortedVars.begin(), sortedVars.end());

    for (char v : sortedVars) {
        GateInstance sw;
        sw.type = GATE_SWITCH;
        sw.x = x;
        sw.y = y;
        sw.val_out = 0;

        circuit.ComputePins(sw);
        circuit.gates.push_back(sw);

        switchIndex[v] = circuit.gates.size() - 1;
        y += 80;
    }

    // 2. Crear compuertas AND por cada término
    vector<int> andOrGates; // Guardar índices de compuertas de cada término
    int andX = 300;
    int andY = 100;

    for (const auto& term : expr.terms) {
        if (term.literals.empty()) continue;

        // Si el término tiene solo 1 literal
        if (term.literals.size() == 1) {
            int srcGate = switchIndex[term.literals[0].var];

            if (term.literals[0].negated) {
                // Crear NOT
                GateInstance notGate;
                notGate.type = GATE_NOT;
                notGate.x = andX;
                notGate.y = andY;
                circuit.ComputePins(notGate);
                circuit.gates.push_back(notGate);

                int notIndex = circuit.gates.size() - 1;

                // SWITCH → NOT
                Cable c;
                c.gateStart = srcGate;
                c.pinStart = PIN_OUTPUT;
                c.gateEnd = notIndex;
                c.pinEnd = PIN_INPUT1;
                circuit.cables.push_back(c);

                andOrGates.push_back(notIndex);
            } else {
                andOrGates.push_back(srcGate);
            }
        }
        // Si tiene 2 literales, usar AND directo
        else if (term.literals.size() == 2) {
            GateInstance andGate;
            andGate.type = GATE_AND;
            andGate.x = andX;
            andGate.y = andY;
            circuit.ComputePins(andGate);
            circuit.gates.push_back(andGate);

            int andIndex = circuit.gates.size() - 1;

            // Conectar primer literal
            int src1 = switchIndex[term.literals[0].var];
            if (term.literals[0].negated) {
                GateInstance notGate;
                notGate.type = GATE_NOT;
                notGate.x = andX - 120;
                notGate.y = andY - 20;
                circuit.ComputePins(notGate);
                circuit.gates.push_back(notGate);

                int notIndex = circuit.gates.size() - 1;

                Cable c1;
                c1.gateStart = src1;
                c1.pinStart = PIN_OUTPUT;
                c1.gateEnd = notIndex;
                c1.pinEnd = PIN_INPUT1;
                circuit.cables.push_back(c1);

                src1 = notIndex;
            }

            Cable c2;
            c2.gateStart = src1;
            c2.pinStart = PIN_OUTPUT;
            c2.gateEnd = andIndex;
            c2.pinEnd = PIN_INPUT1;
            circuit.cables.push_back(c2);

            // Conectar segundo literal
            int src2 = switchIndex[term.literals[1].var];
            if (term.literals[1].negated) {
                GateInstance notGate;
                notGate.type = GATE_NOT;
                notGate.x = andX - 120;
                notGate.y = andY + 20;
                circuit.ComputePins(notGate);
                circuit.gates.push_back(notGate);

                int notIndex = circuit.gates.size() - 1;

                Cable c3;
                c3.gateStart = src2;
                c3.pinStart = PIN_OUTPUT;
                c3.gateEnd = notIndex;
                c3.pinEnd = PIN_INPUT1;
                circuit.cables.push_back(c3);

                src2 = notIndex;
            }

            Cable c4;
            c4.gateStart = src2;
            c4.pinStart = PIN_OUTPUT;
            c4.gateEnd = andIndex;
            c4.pinEnd = PIN_INPUT2;
            circuit.cables.push_back(c4);

            andOrGates.push_back(andIndex);
        }
        // Si tiene más de 2 literales, crear cadena de ANDs
        else {
            int prevAndIndex = -1;

            for (size_t i = 0; i < term.literals.size(); i++) {
                if (i == 0) {
                    // Primer AND con dos primeros literales
                    GateInstance andGate;
                    andGate.type = GATE_AND;
                    andGate.x = andX;
                    andGate.y = andY;
                    circuit.ComputePins(andGate);
                    circuit.gates.push_back(andGate);

                    prevAndIndex = circuit.gates.size() - 1;

                    // Conectar primeros dos literales
                    for (int j = 0; j < 2 && j < (int)term.literals.size(); j++) {
                        int srcGate = switchIndex[term.literals[j].var];
                        int finalSrc = srcGate;

                        if (term.literals[j].negated) {
                            GateInstance notGate;
                            notGate.type = GATE_NOT;
                            notGate.x = andX - 120;
                            notGate.y = andY + j * 40 - 20;
                            circuit.ComputePins(notGate);
                            circuit.gates.push_back(notGate);

                            int notIndex = circuit.gates.size() - 1;

                            Cable c1;
                            c1.gateStart = srcGate;
                            c1.pinStart = PIN_OUTPUT;
                            c1.gateEnd = notIndex;
                            c1.pinEnd = PIN_INPUT1;
                            circuit.cables.push_back(c1);

                            finalSrc = notIndex;
                        }

                        Cable c2;
                        c2.gateStart = finalSrc;
                        c2.pinStart = PIN_OUTPUT;
                        c2.gateEnd = prevAndIndex;
                        c2.pinEnd = (j == 0) ? PIN_INPUT1 : PIN_INPUT2;
                        circuit.cables.push_back(c2);
                    }

                    i = 1; // Ya procesamos los dos primeros
                }
                else if (i < term.literals.size()) {
                    // ANDs adicionales
                    GateInstance andGate;
                    andGate.type = GATE_AND;
                    andGate.x = andX + 80;
                    andGate.y = andY;
                    circuit.ComputePins(andGate);
                    circuit.gates.push_back(andGate);

                    int newAndIndex = circuit.gates.size() - 1;

                    // Conectar resultado anterior
                    Cable c1;
                    c1.gateStart = prevAndIndex;
                    c1.pinStart = PIN_OUTPUT;
                    c1.gateEnd = newAndIndex;
                    c1.pinEnd = PIN_INPUT1;
                    circuit.cables.push_back(c1);

                    // Conectar nuevo literal
                    int srcGate = switchIndex[term.literals[i].var];
                    int finalSrc = srcGate;

                    if (term.literals[i].negated) {
                        GateInstance notGate;
                        notGate.type = GATE_NOT;
                        notGate.x = andX - 40;
                        notGate.y = andY + 40;
                        circuit.ComputePins(notGate);
                        circuit.gates.push_back(notGate);

                        int notIndex = circuit.gates.size() - 1;

                        Cable c2;
                        c2.gateStart = srcGate;
                        c2.pinStart = PIN_OUTPUT;
                        c2.gateEnd = notIndex;
                        c2.pinEnd = PIN_INPUT1;
                        circuit.cables.push_back(c2);

                        finalSrc = notIndex;
                    }

                    Cable c3;
                    c3.gateStart = finalSrc;
                    c3.pinStart = PIN_OUTPUT;
                    c3.gateEnd = newAndIndex;
                    c3.pinEnd = PIN_INPUT2;
                    circuit.cables.push_back(c3);

                    prevAndIndex = newAndIndex;
                    andX += 80;
                }
            }

            if (prevAndIndex >= 0) {
                andOrGates.push_back(prevAndIndex);
            }
        }

        andY += 100;
        andX = 300;
    }

    // 3. Crear OR final (si hay más de un término) o conectar directamente
    int finalOutputGate;

    if (andOrGates.size() == 1) {
        // Solo un término, conectar directamente
        finalOutputGate = andOrGates[0];
    }
    else if (andOrGates.size() == 2) {
        // Dos términos, un solo OR
        GateInstance orGate;
        orGate.type = GATE_OR;
        orGate.x = 550;
        orGate.y = 200;
        circuit.ComputePins(orGate);
        circuit.gates.push_back(orGate);

        int orIndex = circuit.gates.size() - 1;

        Cable c1;
        c1.gateStart = andOrGates[0];
        c1.pinStart = PIN_OUTPUT;
        c1.gateEnd = orIndex;
        c1.pinEnd = PIN_INPUT1;
        circuit.cables.push_back(c1);

        Cable c2;
        c2.gateStart = andOrGates[1];
        c2.pinStart = PIN_OUTPUT;
        c2.gateEnd = orIndex;
        c2.pinEnd = PIN_INPUT2;
        circuit.cables.push_back(c2);

        finalOutputGate = orIndex;
    }
    else {
        // Múltiples términos, crear cadena de ORs
        int prevOrIndex = -1;

        for (size_t i = 0; i < andOrGates.size(); i++) {
            if (i == 0) {
                // Primer OR
                GateInstance orGate;
                orGate.type = GATE_OR;
                orGate.x = 550;
                orGate.y = 200;
                circuit.ComputePins(orGate);
                circuit.gates.push_back(orGate);

                prevOrIndex = circuit.gates.size() - 1;

                // Conectar primeros dos términos
                Cable c1;
                c1.gateStart = andOrGates[0];
                c1.pinStart = PIN_OUTPUT;
                c1.gateEnd = prevOrIndex;
                c1.pinEnd = PIN_INPUT1;
                circuit.cables.push_back(c1);

                if (andOrGates.size() > 1) {
                    Cable c2;
                    c2.gateStart = andOrGates[1];
                    c2.pinStart = PIN_OUTPUT;
                    c2.gateEnd = prevOrIndex;
                    c2.pinEnd = PIN_INPUT2;
                    circuit.cables.push_back(c2);
                }

                i = 1;
            }
            else if (i < andOrGates.size()) {
                // ORs adicionales
                GateInstance orGate;
                orGate.type = GATE_OR;
                orGate.x = 550 + (i - 1) * 80;
                orGate.y = 200;
                circuit.ComputePins(orGate);
                circuit.gates.push_back(orGate);

                int newOrIndex = circuit.gates.size() - 1;

                Cable c1;
                c1.gateStart = prevOrIndex;
                c1.pinStart = PIN_OUTPUT;
                c1.gateEnd = newOrIndex;
                c1.pinEnd = PIN_INPUT1;
                circuit.cables.push_back(c1);

                Cable c2;
                c2.gateStart = andOrGates[i];
                c2.pinStart = PIN_OUTPUT;
                c2.gateEnd = newOrIndex;
                c2.pinEnd = PIN_INPUT2;
                circuit.cables.push_back(c2);

                prevOrIndex = newOrIndex;
            }
        }

        finalOutputGate = prevOrIndex;
    }

    // 4. LED de salida
    GateInstance led;
    led.type = GATE_LED;
    led.x = 750;
    led.y = 200;
    circuit.ComputePins(led);
    circuit.gates.push_back(led);

    int ledIndex = circuit.gates.size() - 1;

    Cable out;
    out.gateStart = finalOutputGate;
    out.pinStart = PIN_OUTPUT;
    out.gateEnd = ledIndex;
    out.pinEnd = PIN_INPUT1;
    circuit.cables.push_back(out);

    circuit.PropagateSignals();
}

// Función para convertir índice a código Gray
int ToGrayCode(int n) {
    return n ^ (n >> 1);
}

int FromGrayCode(int gray) {
    int binary = 0;
    while (gray) {
        binary ^= gray;
        gray >>= 1;
    }
    return binary;
}

int GetKarnaughCellIndex(int row, int col, int numVars) {
    int rowBits = numVars / 2;           // Bits controlados por filas
    int colBits = (numVars + 1) / 2;     // Bits controlados por columnas

    // Convertir fila y columna a código Gray
    int grayRow = ToGrayCode(row);
    int grayCol = ToGrayCode(col);

    // El índice binario se construye como: [bits_columna][bits_fila]
    // Para 3 variables: AB|C donde AB son columnas, C es fila
    // Para 4 variables: AB|CD donde AB son columnas, CD son filas
    int index = (grayCol << rowBits) | grayRow;

    return index;
}

// Función para obtener el índice real de la celda en el mapa
int GetKarnaughIndex(int row, int col, int numVars) {
    int rows = 1 << (numVars / 2);
    int cols = 1 << ((numVars + 1) / 2);

    int grayRow = ToGrayCode(row);
    int grayCol = ToGrayCode(col);

    return grayRow * cols + grayCol;
}

KarnaughMap BuildKarnaughMap(const BooleanExpression& expr) {
    KarnaughMap km;
    km.vars = GetVariables(expr);
    km.numVars = km.vars.size();

    if (km.numVars < 2 || km.numVars > 4) {
        // Error: solo soportamos 2-4 variables
        return km;
    }

    int totalCells = 1 << km.numVars;
    km.cells.resize(totalCells, 0);

    // IMPORTANTE: Evaluar cada término de la expresión
    for (int visualRow = 0; visualRow < (1 << (km.numVars / 2)); visualRow++) {
        for (int visualCol = 0; visualCol < (1 << ((km.numVars + 1) / 2)); visualCol++) {

            // Obtener el índice binario correcto para esta posición visual
            int binaryIndex = GetKarnaughCellIndex(visualRow, visualCol, km.numVars);

            // Evaluar la expresión para este índice binario
            bool cellValue = false;

            for (const auto& term : expr.terms) {
                if (EvaluateTerm(term, km.vars, binaryIndex)) {
                    cellValue = true;
                    break; // OR entre términos
                }
            }

            // Guardar el valor en la posición del mapa visual
            // USAR índice lineal: row * cols + col
            int cols = 1 << ((km.numVars + 1) / 2);
            int mapIndex = visualRow * cols + visualCol;

            km.cells[mapIndex] = cellValue ? 1 : 0;
        }
    }

    return km;
}

/*void BuildCircuitFromExpression(const BooleanExpression& expr) {

    circuit.Clear();

    // 1. Crear SWITCH por cada variable
    map<char, int> switchIndex;
    int x = 50;
    int y = 100;

    for (char v : expr.vars) {
        GateInstance sw;
        sw.type = GATE_SWITCH;
        sw.x = x;
        sw.y = y;
        sw.val_out = 0;

        circuit.ComputePins(sw);
        circuit.gates.push_back(sw);

        switchIndex[v] = circuit.gates.size() - 1;
        y += 80;
    }

    // 2. Crear AND por cada término
    vector<int> andGates;
    int andX = 300;
    int andY = 120;

    for (const auto& term : expr.terms) {
        GateInstance andGate;
        andGate.type = GATE_AND;
        andGate.x = andX;
        andGate.y = andY;

        circuit.ComputePins(andGate);
        circuit.gates.push_back(andGate);

        int andIndex = circuit.gates.size() - 1;
        andGates.push_back(andIndex);

        // Conectar literales
        int inputPin = PIN_INPUT1;

        for (const auto& lit : term.literals) {
            int srcGate = switchIndex[lit.var];

            int finalSrc = srcGate;

            if (lit.negated) {
                GateInstance notGate;
                notGate.type = GATE_NOT;
                notGate.x = andX - 120;
                notGate.y = andY;

                circuit.ComputePins(notGate);
                circuit.gates.push_back(notGate);

                int notIndex = circuit.gates.size() - 1;

                // SWITCH → NOT
                Cable c1;
                c1.gateStart = srcGate;
                c1.pinStart = PIN_OUTPUT;
                c1.gateEnd = notIndex;
                c1.pinEnd = PIN_INPUT1;
                circuit.cables.push_back(c1);

                finalSrc = notIndex;
            }

            // Fuente → AND
            Cable c2;
            c2.gateStart = finalSrc;
            c2.pinStart = PIN_OUTPUT;
            c2.gateEnd = andIndex;
            c2.pinEnd = inputPin;
            circuit.cables.push_back(c2);

            inputPin = PIN_INPUT2;
        }

        andY += 100;
    }

    // 3. OR final
    GateInstance orGate;
    orGate.type = GATE_OR;
    orGate.x = 550;
    orGate.y = 200;

    circuit.ComputePins(orGate);
    circuit.gates.push_back(orGate);

    int orIndex = circuit.gates.size() - 1;

    // Conectar ANDs al OR
    for (size_t i = 0; i < andGates.size(); i++) {
        Cable c;
        c.gateStart = andGates[i];
        c.pinStart = PIN_OUTPUT;
        c.gateEnd = orIndex;
        c.pinEnd = (i == 0) ? PIN_INPUT1 : PIN_INPUT2;
        circuit.cables.push_back(c);
    }

    // 4. LED de salida
    GateInstance led;
    led.type = GATE_LED;
    led.x = 750;
    led.y = 200;

    circuit.ComputePins(led);
    circuit.gates.push_back(led);

    int ledIndex = circuit.gates.size() - 1;

    Cable out;
    out.gateStart = orIndex;
    out.pinStart = PIN_OUTPUT;
    out.gateEnd = ledIndex;
    out.pinEnd = PIN_INPUT1;
    circuit.cables.push_back(out);

    circuit.PropagateSignals();

}*/

INT_PTR CALLBACK DlgSimulator(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgKarnaugh(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgKarnaughView(HWND, UINT, WPARAM, LPARAM);

>>>>>>> Stashed changes


INT_PTR CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;

        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case ID_MENU_SIMULATOR:
                    DialogBox(hInst,
                             MAKEINTRESOURCE(DLG_SIMULATOR),
                             hwndDlg,
                             DlgSimulator);
                    return TRUE;

                case ID_MENU_KARNAUGH:
                    DialogBox(hInst,
                        MAKEINTRESOURCE(DLG_KARNAUGH),
                        hwndDlg,
                        DlgKarnaugh);
                    return TRUE;

                case ID_MENU_MATH:
                    DialogBox(hInst,
                             MAKEINTRESOURCE(DLG_MATH),
                             hwndDlg,
                             DlgMath);
                    return TRUE;

                case ID_MENU_TREES:
                    DialogBox(hInst,
                        MAKEINTRESOURCE(DLG_TREES),
                        hwndDlg,
                        DlgTrees);
                    return TRUE;
            }
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK DlgKarnaugh(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;

        case WM_COMMAND: {
            switch(LOWORD(wParam)) {

                case ID_BTN_SIMPLIFICAR: { // Simplificar
                    char buffer[256];
                    GetDlgItemText(hwndDlg, 4001, buffer, 255);

                    BooleanExpression expr = ParseExpression(buffer);
                    KarnaughMap km = BuildKarnaughMap(expr);

                    vector<KGroup> groups = GroupKarnaugh(km);
                    BooleanExpression rawExpr = GroupsToExpression(groups, km);
                    g_simplifiedExpression = SimplifyExpression(rawExpr);
                    g_expressionForSimulation = ExprToString(g_simplifiedExpression);

                    g_lastKMap = km;
                    g_lastGroups = groups;

                    string debug = "Mapa Karnaugh:\n";
                    for (int i = 0; i < km.cells.size(); i++) {
                        debug += to_string(km.cells[i]) + " ";
                    }

                    MessageBox(hwndDlg, debug.c_str(), "DEBUG", MB_OK);

                    string result = "vars: ";
                    for (char v : expr.vars) {
                        result += v;
                        result += " ";
                    }

                    result += "\nTerminos:\n";

                    for (auto& term : expr.terms) {
                        for (auto& lit : term.literals) {
                            result += lit.var;
                            if (lit.negated) result += "'";
                        }
                        result += "\n";
                    }

                    SetDlgItemText(hwndDlg, 4004, result.c_str());

                    DialogBox(
                        hInst,
                        MAKEINTRESOURCE(DLG_KARNAUGH_VIEW),
                        hwndDlg,
                        DlgKarnaughView
                    );


                    return TRUE;
                }




                case ID_BTN_SEND_SIMULATOR: {

                    if (g_expressionForSimulation.empty()) {
                        MessageBox(hwndDlg,
                            "Primero simplifique la expresión con Karnaugh",
                            "Aviso",
                            MB_OK | MB_ICONWARNING);
                        return TRUE;
                    }

                    // Convertir string → BooleanExpression
                    g_simplifiedExpression = ParseExpression(g_expressionForSimulation);

                    DialogBox(hInst,
                        MAKEINTRESOURCE(DLG_SIMULATOR),
                        hwndDlg,
                        DlgSimulator);

                    return TRUE;
                }


            }
            return TRUE;
        }


    }
    return FALSE;
}

INT_PTR CALLBACK DlgKarnaughView(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwndDlg, &ps);

        int rows = 1 << (g_lastKMap.numVars / 2);
        int cols = 1 << ((g_lastKMap.numVars + 1) / 2);

        int cellW = 60;
        int cellH = 50;
        int offsetX = 100;
        int offsetY = 80;

        // Dibujar etiquetas de columnas (código Gray)
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        for (int j = 0; j < cols; j++) {
            int grayCode = ToGrayCode(j);
            char label[10];

            // Formato binario del código Gray
            string binary = "";
            int colBits = (g_lastKMap.numVars + 1) / 2;
            for (int b = colBits - 1; b >= 0; b--) {
                binary += ((grayCode >> b) & 1) ? '1' : '0';
            }

            strcpy(label, binary.c_str());

            RECT r{
                offsetX + j * cellW,
                offsetY - 30,
                offsetX + (j + 1) * cellW,
                offsetY
            };
            DrawTextA(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        // Dibujar etiquetas de filas (código Gray)
        for (int i = 0; i < rows; i++) {
            int grayCode = ToGrayCode(i);
            char label[10];

            string binary = "";
            int rowBits = g_lastKMap.numVars / 2;
            for (int b = rowBits - 1; b >= 0; b--) {
                binary += ((grayCode >> b) & 1) ? '1' : '0';
            }

            strcpy(label, binary.c_str());

            RECT r{
                offsetX - 70,
                offsetY + i * cellH,
                offsetX - 10,
                offsetY + (i + 1) * cellH
            };
            DrawTextA(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        // Dibujar nombres de variables
        string colVars = "";
        int colBits = (g_lastKMap.numVars + 1) / 2;
        for (int i = 0; i < colBits && i < (int)g_lastKMap.vars.size(); i++) {
            colVars += g_lastKMap.vars[i];
        }

        string rowVars = "";
        int rowBits = g_lastKMap.numVars / 2;
        for (int i = colBits; i < (int)g_lastKMap.vars.size(); i++) {
            rowVars += g_lastKMap.vars[i];
        }

        // Etiqueta superior (variables de columna)
        RECT rColLabel{offsetX, offsetY - 60, offsetX + cols * cellW, offsetY - 30};
        DrawTextA(hdc, colVars.c_str(), -1, &rColLabel, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Etiqueta izquierda (variables de fila)
        RECT rRowLabel{offsetX - 90, offsetY, offsetX - 70, offsetY + rows * cellH};
        DrawTextA(hdc, rowVars.c_str(), -1, &rRowLabel, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Dibujar celdas con valores
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                int mapIndex = i * cols + j;

                RECT r{
                    offsetX + j * cellW,
                    offsetY + i * cellH,
                    offsetX + (j + 1) * cellW,
                    offsetY + (i + 1) * cellH
                };

                // Fondo de la celda
                if (mapIndex < (int)g_lastKMap.cells.size() && g_lastKMap.cells[mapIndex] == 1) {
                    HBRUSH hBrush = CreateSolidBrush(RGB(200, 255, 200));
                    FillRect(hdc, &r, hBrush);
                    DeleteObject(hBrush);
                }

                Rectangle(hdc, r.left, r.top, r.right, r.bottom);

                // Valor de la celda
                if (mapIndex < (int)g_lastKMap.cells.size()) {
                    char text[2] = { char('0' + g_lastKMap.cells[mapIndex]), 0 };

                    HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                    SetTextColor(hdc, RGB(0, 0, 0));
                    DrawTextA(hdc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(hdc, hOldFont);
                    DeleteObject(hFont);
                }
            }
        }

        // Dibujar agrupaciones con colores
        COLORREF colors[] = {
            RGB(255, 100, 100),
            RGB(100, 100, 255),
            RGB(255, 255, 100),
            RGB(255, 100, 255),
            RGB(100, 255, 255)
        };

        for (size_t g = 0; g < g_lastGroups.size() && g < 5; g++) {
            HPEN hPen = CreatePen(PS_SOLID, 3, colors[g % 5]);
            HGDIOBJ oldPen = SelectObject(hdc, hPen);
            HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            HGDIOBJ oldBrush = SelectObject(hdc, hBrush);

            for (int idx : g_lastGroups[g].cells) {
                int i = idx / cols;
                int j = idx % cols;

                Rectangle(
                    hdc,
                    offsetX + j * cellW + 3,
                    offsetY + i * cellH + 3,
                    offsetX + (j + 1) * cellW - 3,
                    offsetY + (i + 1) * cellH - 3
                );
            }

            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(hPen);
        }

        EndPaint(hwndDlg, &ps);
        return TRUE;
    }

    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    }
    return FALSE;
}


INT_PTR CALLBACK DlgSimulator(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_INITDIALOG:

            circuit.Clear();

            if (!g_simplifiedExpression.terms.empty()) {
                BuildCircuitFromExpression(g_simplifiedExpression);
                InvalidateRect(hwndDlg, NULL, TRUE);
            }


            currentMode = MODE_NORMAL;
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
                SetWindowText(hwndDlg, "Simulador de Compuertas Logicas");
                InvalidateRect(hwndDlg, NULL, TRUE);
            } else if (wParam == VK_DELETE) {
                circuit.SaveState();
                for (int i = static_cast<int>(circuit.gates.size()) - 1; i >= 0; i--) {
                    if (circuit.gates[i].selected) {
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

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

            HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 245));
            FillRect(memDC, &clientRect, bgBrush);
            DeleteObject(bgBrush);

            DrawGrid(memDC, clientRect);

            POINT tempStart = {0, 0};
            if (showTempCable && cableDrawing && startGateIndex >= 0) {
                tempStart = circuit.GetPinPosition(circuit.gates[startGateIndex], startPinIndex);
            }
            Renderer::DrawCircuit(memDC, circuit, showTempCable, tempStart, tempCableEnd);

            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwndDlg, &ps);
            return TRUE;
        }

        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (currentMode == MODE_DELETE) {
                int gateIdx = circuit.HitTestGate(x, y);
                if (gateIdx != -1) {
                    circuit.SaveState();
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
                SetWindowText(hwndDlg, "Simulador de Compuertas Logicas");
                InvalidateRect(hwndDlg, NULL, TRUE);
                return TRUE;
            }

            if (currentMode == MODE_CABLE) {
                if (!cableDrawing) {
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
                    int gIndex, pIndex;
                    POINT snapped;
                    if (circuit.SnapToPin(x, y, gIndex, pIndex, snapped)) {
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

            int gateIndex = circuit.HitTestGate(x, y);
            if (gateIndex != -1) {
                GateInstance& gate = circuit.gates[gateIndex];

                if (gate.type == GATE_SWITCH) {
                    circuit.SaveState();
                    gate.val_out = (gate.val_out == 1) ? 0 : 1;
                    circuit.PropagateSignals();
                    InvalidateRect(hwndDlg, NULL, TRUE);
                    return TRUE;
                }

                draggingGate = true;
                draggedGateIndex = gateIndex;
                dragOffsetX = x - circuit.gates[gateIndex].x;
                dragOffsetY = y - circuit.gates[gateIndex].y;
                return TRUE;
            }
            return TRUE;
        }

        case WM_MOUSEMOVE: {
            if (showTempCable && cableDrawing) {
                tempCableEnd.x = LOWORD(lParam);
                tempCableEnd.y = HIWORD(lParam);
                InvalidateRect(hwndDlg, NULL, TRUE);
            }

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

string ConvertBase(const string& number, int fromBase, int toBase) {
    if (fromBase < 2 || fromBase > 16 || toBase < 2 || toBase > 16) {
        return "Error: Base fuera de rango (2-16)";
    }

    size_t dotPos = number.find('.');
    string integerPart, fractionalPart;

    if (dotPos != string::npos) {
        integerPart = number.substr(0, dotPos);
        fractionalPart = number.substr(dotPos + 1);

        if (integerPart.empty()) integerPart = "0";
        if (fractionalPart.empty()) fractionalPart = "0";
    } else {
        integerPart = number;
        fractionalPart = "0";
    }

    long long decimalInteger = 0;
    long long power = 1;

    for (int i = static_cast<int>(integerPart.length()) - 1; i >= 0; i--) {
        char digit = toupper(integerPart[i]);
        int value;

        if (digit >= '0' && digit <= '9') {
            value = digit - '0';
        } else if (digit >= 'A' && digit <= 'F') {
            value = 10 + (digit - 'A');
        } else {
            return "Error: Digito invalido '" + string(1, digit) + "'";
        }

        if (value >= fromBase) {
            return "Error: Digito '" + string(1, digit) + "' fuera de rango para base " + to_string(fromBase);
        }

        decimalInteger += value * power;
        power *= fromBase;
    }

    double decimalFraction = 0.0;
    double divisor = fromBase;

    for (size_t i = 0; i < fractionalPart.length(); i++) {
        char digit = toupper(fractionalPart[i]);
        int value;

        if (digit >= '0' && digit <= '9') {
            value = digit - '0';
        } else if (digit >= 'A' && digit <= 'F') {
            value = 10 + (digit - 'A');
        } else {
            return "Error: Digito fraccionario invalido";
        }

        if (value >= fromBase) {
            return "Error: Digito fraccionario fuera de rango";
        }

        decimalFraction += value / divisor;
        divisor *= fromBase;
    }

    double decimalTotal = decimalInteger + decimalFraction;

    if (toBase == 10) {
        if (fractionalPart == "0") {
            return to_string(decimalInteger);
        } else {
            char buffer[256];
            sprintf(buffer, "%.10f", decimalTotal);

            string result = buffer;
            size_t lastNonZero = result.find_last_not_of('0');
            if (lastNonZero != string::npos && result[lastNonZero] == '.') {
                result = result.substr(0, lastNonZero);
            } else if (lastNonZero != string::npos) {
                result = result.substr(0, lastNonZero + 1);
            }
            return result;
        }
    }

    string resultInteger;
    const char digits[] = "0123456789ABCDEF";

    if (decimalInteger == 0) {
        resultInteger = "0";
    } else {
        long long temp = decimalInteger;
        while (temp > 0) {
            int remainder = temp % toBase;
            resultInteger = digits[remainder] + resultInteger;
            temp /= toBase;
        }
    }

    string resultFraction;
    if (decimalFraction > 0) {
        double fraction = decimalFraction;
        const int MAX_FRACTION_DIGITS = 10;

        for (int i = 0; i < MAX_FRACTION_DIGITS && fraction > 0; i++) {
            fraction *= toBase;
            int digit = static_cast<int>(fraction);
            resultFraction += digits[digit];
            fraction -= digit;
        }

        while (!resultFraction.empty() && resultFraction.back() == '0') {
            resultFraction.pop_back();
        }
    }

    if (resultFraction.empty()) {
        return resultInteger;
    } else {
        return resultInteger + "." + resultFraction;
    }
}

unsigned long long CalculateFactorial(int n, string& steps) {
    if (n < 0) return 0;
    if (n == 0 || n == 1) {
        steps = "0! = 1 (por definicion)";
        return 1;
    }

    unsigned long long result = 1;
    steps = "1";

    for (int i = 2; i <= n; i++) {
        result *= i;
        steps += " x " + to_string(i);
    }

    steps += " = " + to_string(result);
    return result;
}

unsigned long long CalculatePermutations(int n, int k, string& steps) {
    if (n < 0 || k < 0 || k > n) return 0;

    string factorialNSteps, factorialNKSteps;
    unsigned long long factorialN = CalculateFactorial(n, factorialNSteps);
    unsigned long long factorialNK = CalculateFactorial(n - k, factorialNKSteps);

    if (factorialNK == 0) {
        steps = "Error: (n-k)! no puede ser calculado";
        return 0;
    }

    unsigned long long result = factorialN / factorialNK;
    steps = "P(" + to_string(n) + "," + to_string(k) + ") = " +
            to_string(n) + "! / (" + to_string(n) + "-" + to_string(k) + ")! = " +
            to_string(factorialN) + " / " + to_string(factorialNK) + " = " +
            to_string(result);

    return result;
}

unsigned long long CalculateCombinations(int n, int k, string& steps) {
    if (n < 0 || k < 0 || k > n) return 0;

    string factorialNSteps, factorialKSteps, factorialNKSteps;
    unsigned long long factorialN = CalculateFactorial(n, factorialNSteps);
    unsigned long long factorialK = CalculateFactorial(k, factorialKSteps);
    unsigned long long factorialNK = CalculateFactorial(n - k, factorialNKSteps);

    if (factorialK == 0 || factorialNK == 0) {
        steps = "Error: No se puede calcular factorial";
        return 0;
    }

    unsigned long long denominator = factorialK * factorialNK;
    if (denominator == 0) {
        steps = "Error: Denominador cero";
        return 0;
    }

    unsigned long long result = factorialN / denominator;
    steps = "C(" + to_string(n) + "," + to_string(k) + ") = " +
            to_string(n) + "! / (" + to_string(k) + "! x (" +
            to_string(n) + "-" + to_string(k) + ")!) = " +
            to_string(factorialN) + " / (" + to_string(factorialK) +
            " x " + to_string(factorialNK) + ") = " +
            to_string(factorialN) + " / " + to_string(denominator) +
            " = " + to_string(result);

    return result;
}

INT_PTR CALLBACK DlgMath(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_INITDIALOG: {
            HWND hComboOrigen = GetDlgItem(hwndDlg, IDC_MATH_BASE_ORIGEN);
            HWND hComboDestino = GetDlgItem(hwndDlg, IDC_MATH_BASE_DESTINO);

            SendMessage(hComboOrigen, CB_ADDSTRING, 0, (LPARAM)"Binario (2)");
            SendMessage(hComboOrigen, CB_ADDSTRING, 0, (LPARAM)"Octal (8)");
            SendMessage(hComboOrigen, CB_ADDSTRING, 0, (LPARAM)"Decimal (10)");
            SendMessage(hComboOrigen, CB_ADDSTRING, 0, (LPARAM)"Hexadecimal (16)");
            SendMessage(hComboOrigen, CB_SETCURSEL, 2, 0);

            SendMessage(hComboDestino, CB_ADDSTRING, 0, (LPARAM)"Binario (2)");
            SendMessage(hComboDestino, CB_ADDSTRING, 0, (LPARAM)"Octal (8)");
            SendMessage(hComboDestino, CB_ADDSTRING, 0, (LPARAM)"Decimal (10)");
            SendMessage(hComboDestino, CB_ADDSTRING, 0, (LPARAM)"Hexadecimal (16)");
            SendMessage(hComboDestino, CB_SETCURSEL, 0, 0);

            SetDlgItemText(hwndDlg, IDC_MATH_N, "5");
            SetDlgItemText(hwndDlg, IDC_MATH_K, "2");

            return TRUE;
        }

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;

        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case IDC_MATH_CONVERTIR: {
                    char numero[256];
                    GetDlgItemText(hwndDlg, IDC_MATH_NUMERO, numero, 255);

                    if (strlen(numero) == 0) {
                        MessageBox(hwndDlg, "Por favor ingrese un numero", "Error", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    int dotCount = 0;
                    for (int i = 0; numero[i] != '\0'; i++) {
                        if (numero[i] == '.') {
                            dotCount++;
                            if (dotCount > 1) {
                                MessageBox(hwndDlg, "Solo se permite un punto decimal", "Error", MB_OK | MB_ICONERROR);
                                return TRUE;
                            }
                        }
                    }

                    HWND hComboOrigen = GetDlgItem(hwndDlg, IDC_MATH_BASE_ORIGEN);
                    HWND hComboDestino = GetDlgItem(hwndDlg, IDC_MATH_BASE_DESTINO);

                    int idxOrigen = (int)SendMessage(hComboOrigen, CB_GETCURSEL, 0, 0);
                    int idxDestino = (int)SendMessage(hComboDestino, CB_GETCURSEL, 0, 0);

                    int baseOrigen, baseDestino;

                    switch(idxOrigen) {
                        case 0: baseOrigen = 2; break;
                        case 1: baseOrigen = 8; break;
                        case 2: baseOrigen = 10; break;
                        case 3: baseOrigen = 16; break;
                        default: baseOrigen = 10;
                    }

                    switch(idxDestino) {
                        case 0: baseDestino = 2; break;
                        case 1: baseDestino = 8; break;
                        case 2: baseDestino = 10; break;
                        case 3: baseDestino = 16; break;
                        default: baseDestino = 2;
                    }

                    string resultado = ConvertBase(numero, baseOrigen, baseDestino);
                    SetDlgItemText(hwndDlg, IDC_MATH_RESULTADO_CONV, resultado.c_str());

                    string resolucion = "CONVERSION DE BASES NUMERICAS\n";
                    resolucion += "================================\n\n";
                    resolucion += "Numero ingresado: " + string(numero) + " (base " + to_string(baseOrigen) + ")\n";
                    resolucion += "Base destino: " + to_string(baseDestino) + "\n\n";

                    if (resultado.find("Error") == string::npos) {
                        resolucion += "RESULTADO: " + resultado + "\n\n";
                        resolucion += "PROCESO DE CONVERSION:\n";
                        resolucion += "1. Validacion de digitos para base " + to_string(baseOrigen) + "\n";

                        size_t dotPos = string(numero).find('.');
                        if (dotPos != string::npos) {
                            resolucion += "2. Separacion en parte entera y fraccionaria\n";
                            string integerPart = string(numero).substr(0, dotPos);
                            string fractionalPart = string(numero).substr(dotPos + 1);
                            if (integerPart.empty()) integerPart = "0";
                            resolucion += "   - Parte entera: " + integerPart + "\n";
                            resolucion += "   - Parte fraccionaria: " + fractionalPart + "\n";
                        }

                        resolucion += "3. Conversion a valor decimal\n";

                        if (baseDestino == 10) {
                            resolucion += "4. Resultado decimal obtenido directamente\n";
                        } else {
                            resolucion += "4. Conversion a base " + to_string(baseDestino) + "\n";
                            resolucion += "   - Division sucesiva (parte entera)\n";
                            if (dotPos != string::npos) {
                                resolucion += "   - Multiplicacion sucesiva (parte fraccionaria)\n";
                            }
                        }
                    } else {
                        resolucion += "ERROR EN LA CONVERSION:\n";
                        resolucion += resultado + "\n\n";
                        resolucion += "POSIBLES CAUSAS:\n";
                        resolucion += "1. Digitos no validos para la base origen\n";
                        resolucion += "2. Caracteres no permitidos\n";
                        resolucion += "3. Multiples puntos decimales\n";
                        resolucion += "4. Base fuera del rango permitido (2-16)\n";
                    }

                    SetDlgItemText(hwndDlg, IDC_MATH_RESOLUCION, resolucion.c_str());
                    return TRUE;
                }

                case IDC_MATH_CALCULAR: {
                    char strN[32], strK[32];
                    GetDlgItemText(hwndDlg, IDC_MATH_N, strN, 31);
                    GetDlgItemText(hwndDlg, IDC_MATH_K, strK, 31);

                    int n = atoi(strN);
                    int k = atoi(strK);

                    if (n < 0 || k < 0) {
                        MessageBox(hwndDlg, "n y k deben ser numeros no negativos", "Error", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    if (k > n) {
                        MessageBox(hwndDlg, "k no puede ser mayor que n", "Error", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    string factorialSteps;
                    unsigned long long factorial = CalculateFactorial(n, factorialSteps);
                    SetDlgItemInt(hwndDlg, IDC_MATH_FACTORIAL, (UINT)factorial, FALSE);

                    string permSteps;
                    unsigned long long permutaciones = CalculatePermutations(n, k, permSteps);
                    SetDlgItemInt(hwndDlg, IDC_MATH_PERMUTACIONES, (UINT)permutaciones, FALSE);

                    string combSteps;
                    unsigned long long combinaciones = CalculateCombinations(n, k, combSteps);
                    SetDlgItemInt(hwndDlg, IDC_MATH_COMBINACIONES, (UINT)combinaciones, FALSE);

                    string resolucion = "CALCULOS COMBINATORIOS\n";
                    resolucion += "======================\n\n";
                    resolucion += "Datos: n = " + to_string(n) + ", k = " + to_string(k) + "\n\n";
                    resolucion += "1. FACTORIAL (n!)\n";
                    resolucion += factorialSteps + "\n\n";
                    resolucion += "2. PERMUTACIONES P(n,k)\n";
                    resolucion += permSteps + "\n\n";
                    resolucion += "3. COMBINACIONES C(n,k)\n";
                    resolucion += combSteps + "\n\n";
                    resolucion += "Nota: Si los resultados son 0, puede deberse a:\n";
                    resolucion += "- Desbordamiento numerico (numeros muy grandes)\n";
                    resolucion += "- Valores invalidos de n o k";

                    SetDlgItemText(hwndDlg, IDC_MATH_RESOLUCION, resolucion.c_str());
                    return TRUE;
                }

                case IDC_MATH_SALIR:
                    EndDialog(hwndDlg, 0);
                    return TRUE;
            }
            break;
        }
    }
    return FALSE;
}
struct TreeNode {
    int value;
    TreeNode* left;
    TreeNode* right;
    int x, y;
    bool highlighted;

    TreeNode(int val) : value(val), left(nullptr), right(nullptr),
                        x(0), y(0), highlighted(false) {}
};

class BinarySearchTree {
private:
    TreeNode* root;
    vector<int> traversalResult;

    TreeNode* InsertRec(TreeNode* node, int value) {
        if (node == nullptr) {
            return new TreeNode(value);
        }

        if (value < node->value) {
            node->left = InsertRec(node->left, value);
        } else if (value > node->value) {
            node->right = InsertRec(node->right, value);
        }

        return node;
    }

    TreeNode* FindMin(TreeNode* node) {
        while (node && node->left != nullptr) {
            node = node->left;
        }
        return node;
    }

    TreeNode* DeleteRec(TreeNode* node, int value, bool& found) {
        if (node == nullptr) {
            found = false;
            return nullptr;
        }

        if (value < node->value) {
            node->left = DeleteRec(node->left, value, found);
        } else if (value > node->value) {
            node->right = DeleteRec(node->right, value, found);
        } else {
            found = true;

            if (node->left == nullptr && node->right == nullptr) {
                delete node;
                return nullptr;
            }
            else if (node->left == nullptr) {
                TreeNode* temp = node->right;
                delete node;
                return temp;
            } else if (node->right == nullptr) {
                TreeNode* temp = node->left;
                delete node;
                return temp;
            }
            else {
                TreeNode* temp = FindMin(node->right);
                node->value = temp->value;
                node->right = DeleteRec(node->right, temp->value, found);
            }
        }

        return node;
    }

    bool SearchRec(TreeNode* node, int value) {
        if (node == nullptr) return false;
        if (node->value == value) return true;

        if (value < node->value) {
            return SearchRec(node->left, value);
        } else {
            return SearchRec(node->right, value);
        }
    }

    void InorderRec(TreeNode* node) {
        if (node == nullptr) return;
        InorderRec(node->left);
        traversalResult.push_back(node->value);
        InorderRec(node->right);
    }

    void PreorderRec(TreeNode* node) {
        if (node == nullptr) return;
        traversalResult.push_back(node->value);
        PreorderRec(node->left);
        PreorderRec(node->right);
    }

    void PostorderRec(TreeNode* node) {
        if (node == nullptr) return;
        PostorderRec(node->left);
        PostorderRec(node->right);
        traversalResult.push_back(node->value);
    }

    void CalculatePositions(TreeNode* node, int x, int y, int offset) {
        if (node == nullptr) return;

        node->x = x;
        node->y = y;

        if (node->left) {
            CalculatePositions(node->left, x - offset, y + 60, offset / 2);
        }
        if (node->right) {
            CalculatePositions(node->right, x + offset, y + 60, offset / 2);
        }
    }

    int CountNodes(TreeNode* node) {
        if (node == nullptr) return 0;
        return 1 + CountNodes(node->left) + CountNodes(node->right);
    }

    int GetHeight(TreeNode* node) {
        if (node == nullptr) return 0;
        return 1 + max(GetHeight(node->left), GetHeight(node->right));
    }

    void ClearRec(TreeNode* node) {
        if (node == nullptr) return;
        ClearRec(node->left);
        ClearRec(node->right);
        delete node;
    }

    void DrawTreeRec(HDC hdc, TreeNode* node) {
        if (node == nullptr) return;

        if (node->left) {
            MoveToEx(hdc, node->x, node->y, NULL);
            LineTo(hdc, node->left->x, node->left->y);
            DrawTreeRec(hdc, node->left);
        }

        if (node->right) {
            MoveToEx(hdc, node->x, node->y, NULL);
            LineTo(hdc, node->right->x, node->right->y);
            DrawTreeRec(hdc, node->right);
        }

        COLORREF nodeColor = node->highlighted ? RGB(255, 200, 0) : RGB(100, 150, 255);
        HBRUSH brush = CreateSolidBrush(nodeColor);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

        Ellipse(hdc, node->x - 20, node->y - 20, node->x + 20, node->y + 20);

        SelectObject(hdc, oldBrush);
        DeleteObject(brush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        char valueStr[20];
        sprintf(valueStr, "%d", node->value);

        SIZE textSize;
        GetTextExtentPoint32(hdc, valueStr, strlen(valueStr), &textSize);
        TextOut(hdc, node->x - textSize.cx/2, node->y - textSize.cy/2,
                valueStr, strlen(valueStr));
    }

public:
    BinarySearchTree() : root(nullptr) {}

    ~BinarySearchTree() {
        Clear();
    }

    void Insert(int value) {
        root = InsertRec(root, value);
    }

    bool Delete(int value) {
        bool found = false;
        root = DeleteRec(root, value, found);
        return found;
    }

    bool Search(int value) {
        return SearchRec(root, value);
    }

    vector<int> InorderTraversal() {
        traversalResult.clear();
        InorderRec(root);
        return traversalResult;
    }

    vector<int> PreorderTraversal() {
        traversalResult.clear();
        PreorderRec(root);
        return traversalResult;
    }

    vector<int> PostorderTraversal() {
        traversalResult.clear();
        PostorderRec(root);
        return traversalResult;
    }

    void Clear() {
        ClearRec(root);
        root = nullptr;
    }

    bool IsEmpty() {
        return root == nullptr;
    }

    void UpdatePositions(int startX, int startY) {
    if (root == nullptr) return;
    int nodeCount = CountNodes(root);
    int offset = 80 + (nodeCount * 8);
    if (offset > 300) offset = 300;
    CalculatePositions(root, startX, startY, offset);
}

    void DrawTree(HDC hdc) {
        if (root == nullptr) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(100, 100, 100));
            const char* msg = "El árbol está vacío. Inserta valores para comenzar.";
            TextOut(hdc, 400, 400, msg, strlen(msg));
            return;
        }

        DrawTreeRec(hdc, root);

    }

    string GetStats() {
        char buffer[200];
        sprintf(buffer, "Nodos: %d  |  Altura: %d",
                CountNodes(root), GetHeight(root));
        return string(buffer);
    }
};

BinarySearchTree bst;
enum EdgeState { EDGE_NONE, EDGE_CONSIDERED, EDGE_SELECTED };

struct Edge {
    int u, v, weight;
    EdgeState state;
    Edge(int u, int v, int w) : u(u), v(v), weight(w), state(EDGE_NONE) {}
};

class Graph {
private:
    int V; 
    vector<vector<pair<int, int>>> adj; 
    vector<Edge> edges; 
    vector<pair<int, int>> positions;

    void CalculatePositions(int centerX, int centerY, int radius) {
        positions.resize(V);
        for (int i = 0; i < V; i++) {
            double angle = 2 * M_PI * i / V;
            positions[i].first = centerX + (int)(radius * cos(angle));
            positions[i].second = centerY + (int)(radius * sin(angle));
        }
    }

public:
    Graph(int v) : V(v), adj(v) {
        CalculatePositions(500, 400, 150);
    }

    void AddEdge(int u, int v, int w) {
        adj[u].push_back({v, w});
        adj[v].push_back({u, w});
        edges.push_back(Edge(u, v, w));
    }

    vector<Edge> PrimMST() {
        vector<Edge> mst;
        vector<bool> visited(V, false);
        priority_queue<pair<int, pair<int, int>>, vector<pair<int, pair<int, int>>>, greater<pair<int, pair<int, int>>>> pq;

        visited[0] = true;
        for (auto& p : adj[0]) {
            pq.push({p.second, {0, p.first}});
        }

        while (!pq.empty()) {
            auto top = pq.top(); pq.pop();
            int weight = top.first;
            auto nodes = top.second;
            int u = nodes.first, v = nodes.second;

            if (visited[v]) continue;
            visited[v] = true;
            mst.push_back(Edge(u, v, weight));

            for (auto& p : adj[v]) {
                if (!visited[p.first]) {
                    pq.push({p.second, {v, p.first}});
                }
            }
        }
        return mst;
    }

    vector<Edge> KruskalMST() {
        vector<Edge> mst;
        sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
            return a.weight < b.weight;
        });

        vector<int> parent(V);
        for (int i = 0; i < V; i++) parent[i] = i;

        auto find = [&](auto& self, int x) -> int {
            return parent[x] == x ? x : parent[x] = self(self, parent[x]);
        };

        for (auto& e : edges) {
            int pu = find(find, e.u), pv = find(find, e.v);
            if (pu != pv) {
                parent[pu] = pv;
                mst.push_back(e);
            }
        }
        return mst;
    }

    void DrawGraph(HDC hdc, const vector<Edge>& mstEdges = {}) {
        for (auto& e : edges) {
            int x1 = positions[e.u].first, y1 = positions[e.u].second;
            int x2 = positions[e.v].first, y2 = positions[e.v].second;

            COLORREF color = RGB(100, 100, 100); 
            if (e.state == EDGE_CONSIDERED) color = RGB(255, 255, 0); 
            else if (e.state == EDGE_SELECTED) color = RGB(0, 255, 0); 

            GDIGuard pen(hdc, CreatePen(PS_SOLID, 2, color));
            MoveToEx(hdc, x1, y1, NULL);
            LineTo(hdc, x2, y2);

            int midX = (x1 + x2) / 2, midY = (y1 + y2) / 2;
            char buf[10];
            sprintf(buf, "%d", e.weight);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));
            TextOut(hdc, midX, midY, buf, strlen(buf));
        }

        for (int i = 0; i < V; i++) {
            int x = positions[i].first, y = positions[i].second;
            GDIGuard brush(hdc, CreateSolidBrush(RGB(100, 150, 255)));
            Ellipse(hdc, x - 20, y - 20, x + 20, y + 20);

            char buf[10];
            sprintf(buf, "%d", i);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOut(hdc, x - 5, y - 7, buf, strlen(buf));
        }
    }

    void ResetEdges() {
        for (auto& e : edges) e.state = EDGE_NONE;
    }

    void SetMSTEdges(const vector<Edge>& mst) {
        ResetEdges();
        for (auto& e : mst) {
            for (auto& ge : edges) {
                if ((ge.u == e.u && ge.v == e.v) || (ge.u == e.v && ge.v == e.u)) {
                    ge.state = EDGE_SELECTED;
                    break;
                }
            }
        }
    }
};
Graph* currentGraph = nullptr;
bool isGraphMode = false;
INT_PTR CALLBACK DlgTrees(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_INITDIALOG:
            bst.Clear();
            if (currentGraph) delete currentGraph;
            currentGraph = nullptr;
            isGraphMode = false;
            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Listo para operaciones");
            return TRUE;

        case WM_CLOSE:
            if (currentGraph) delete currentGraph;
            EndDialog(hwndDlg, 0);
            return TRUE;

        case WM_COMMAND: {
            if (isGraphMode) {
                switch(LOWORD(wParam)) {
                    case ID_BTN_TOGGLE_MODE:
                        isGraphMode = false;
                        if (currentGraph) delete currentGraph;
                        currentGraph = nullptr;
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Modo Arbol activado");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;

                    case ID_BTN_GRAPH_SELECT1: {
                        if (currentGraph) delete currentGraph;
                        currentGraph = new Graph(5);
                        currentGraph->AddEdge(0, 1, 2);
                        currentGraph->AddEdge(0, 3, 6);
                        currentGraph->AddEdge(1, 2, 3);
                        currentGraph->AddEdge(1, 3, 8);
                        currentGraph->AddEdge(1, 4, 5);
                        currentGraph->AddEdge(2, 4, 7);
                        currentGraph->AddEdge(3, 4, 9);
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Grafo 1 cargado (5 nodos)");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;

                    case ID_BTN_GRAPH_SELECT2: {
                        if (currentGraph) delete currentGraph;
                        currentGraph = new Graph(8);
                        currentGraph->AddEdge(0, 1, 4);
                        currentGraph->AddEdge(0, 7, 8);
                        currentGraph->AddEdge(1, 2, 8);
                        currentGraph->AddEdge(1, 7, 11);
                        currentGraph->AddEdge(2, 3, 7);
                        currentGraph->AddEdge(2, 5, 4);
                        currentGraph->AddEdge(2, 7, 2); 
                        currentGraph->AddEdge(3, 4, 9);
                        currentGraph->AddEdge(3, 5, 14);
                        currentGraph->AddEdge(4, 5, 10);
                        currentGraph->AddEdge(5, 6, 2);
                        currentGraph->AddEdge(6, 7, 1);
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Grafo 2 cargado (8 nodos)");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;
                        }
                    }

                    case ID_BTN_GRAPH_PRIM: {
                        if (!currentGraph) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Selecciona un grafo primero");
                            break;
                        }
                        auto mst = currentGraph->PrimMST();
                        currentGraph->SetMSTEdges(mst);
                        char buf[100];
                        sprintf(buf, "MST con Prim completado - %d aristas", (int)mst.size());
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, buf);
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;
                    }

                    case ID_BTN_GRAPH_KRUSKAL: {
                        if (!currentGraph) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Selecciona un grafo primero");
                            break;
                        }
                        auto mst = currentGraph->KruskalMST();
                        currentGraph->SetMSTEdges(mst);
                        char buf[100];
                        sprintf(buf, "MST con Kruskal completado - %d aristas", (int)mst.size());
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, buf);
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;
                    }

                    case ID_BTN_GRAPH_RESET: {
                        if (currentGraph) currentGraph->ResetEdges();
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Grafo reseteado");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;
                    }
                }
            } else {
                switch(LOWORD(wParam)) {
                    case ID_BTN_TOGGLE_MODE:
                        isGraphMode = true;
                        bst.Clear();
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Modo Grafo activado");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;

                    case ID_BTN_TREE_INSERT: {
                        char buffer[50];
                        GetDlgItemText(hwndDlg, ID_EDIT_TREE_VALUE, buffer, 50);

                        if (strlen(buffer) == 0) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Error: Ingrese un valor");
                            break;
                        }

                        int value = atoi(buffer);
                        bst.Insert(value);
                        bst.UpdatePositions(500, 300 );

                        sprintf(buffer, "Valor %d insertado | %s", value, bst.GetStats().c_str());
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, buffer);
                        SetDlgItemText(hwndDlg, ID_EDIT_TREE_VALUE, "");

                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;
                    }

                    case ID_BTN_TREE_DELETE: {
                        char buffer[50];
                        GetDlgItemText(hwndDlg, ID_EDIT_TREE_VALUE, buffer, 50);

                        if (strlen(buffer) == 0) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Error: Ingrese un valor");
                            break;
                        }

                        int value = atoi(buffer);
                        bool deleted = bst.Delete(value);

                        if (deleted) {
                            bst.UpdatePositions(500, 300);
                            sprintf(buffer, "Valor %d eliminado | %s", value, bst.GetStats().c_str());
                        } else {
                            sprintf(buffer, "Valor %d no encontrado", value);
                        }

                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, buffer);
                        SetDlgItemText(hwndDlg, ID_EDIT_TREE_VALUE, "");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;
                    }

                    case ID_BTN_TREE_SEARCH: {
                        char buffer[50];
                        GetDlgItemText(hwndDlg, ID_EDIT_TREE_VALUE, buffer, 50);

                        if (strlen(buffer) == 0) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Error: Ingrese un valor");
                            break;
                        }

                        int value = atoi(buffer);
                        bool found = bst.Search(value);

                        sprintf(buffer, "Valor %d %s en el arbol", value, found ? "SI esta" : "NO esta");
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, buffer);
                        break;
                    }

                    case ID_BTN_TREE_RANDOM: {
                        srand(time(NULL));
                        int value = rand() % 100 + 1;
                        bst.Insert(value);
                        bst.UpdatePositions(500, 300);

                        char buffer[100];
                        sprintf(buffer, "Valor aleatorio %d insertado | %s", value, bst.GetStats().c_str());
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, buffer);
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;
                    }

                    case ID_BTN_TREE_INORDER: {
                        if (bst.IsEmpty()) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "El arbol esta vacio");
                            break;
                        }

                        vector<int> result = bst.InorderTraversal();
                        string resultStr = "Inorden: ";
                        for (size_t i = 0; i < result.size(); i++) {
                            char num[20];
                            sprintf(num, "%d", result[i]);
                            resultStr += num;
                            if (i < result.size() - 1) resultStr += ", ";
                        }
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, resultStr.c_str());
                        break;
                    }

                    case ID_BTN_TREE_PREORDER: {
                        if (bst.IsEmpty()) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "El arbol esta vacio");
                            break;
                        }

                        vector<int> result = bst.PreorderTraversal();
                        string resultStr = "Preorden: ";
                        for (size_t i = 0; i < result.size(); i++) {
                            char num[20];
                            sprintf(num, "%d", result[i]);
                            resultStr += num;
                            if (i < result.size() - 1) resultStr += ", ";
                        }
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, resultStr.c_str());
                        break;
                    }

                    case ID_BTN_TREE_POSTORDER: {
                        if (bst.IsEmpty()) {
                            SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "El arbol esta vacio");
                            break;
                        }

                        vector<int> result = bst.PostorderTraversal();
                        string resultStr = "Postorden: ";
                        for (size_t i = 0; i < result.size(); i++) {
                            char num[20];
                            sprintf(num, "%d", result[i]);
                            resultStr += num;
                            if (i < result.size() - 1) resultStr += ", ";
                        }
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, resultStr.c_str());
                        break;
                    }

                    case ID_BTN_TREE_CLEAR:
                        bst.Clear();
                        SetDlgItemText(hwndDlg, ID_TEXT_TREE_RESULT, "Arbol limpiado");
                        InvalidateRect(hwndDlg, NULL, TRUE);
                        break;

                    case ID_BTN_TREE_EXIT:
                        EndDialog(hwndDlg, 0);
                        break;
                }
            }
            return TRUE;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwndDlg, &ps);

            RECT clientRect;
            GetClientRect(hwndDlg, &clientRect);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            HBRUSH bgBrush = CreateSolidBrush(RGB(245, 245, 250));
            FillRect(memDC, &clientRect, bgBrush);
            DeleteObject(bgBrush);

            if (isGraphMode && currentGraph) {
                currentGraph->DrawGraph(memDC);
            } else {
                bst.DrawTree(memDC);
            }

            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwndDlg, &ps);
            return TRUE;
        }
    }
    return FALSE;
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nShowCmd) {
    hInst = hInstance;
    InitCommonControls();
    return DialogBox(hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, DlgMain);
}

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
