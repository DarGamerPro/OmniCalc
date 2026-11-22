#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cwchar>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

using namespace std;

enum {
    ID_MODE = 100,
    ID_LABEL = 200,
    ID_FIELD = 210,
    ID_ACTION = 300,
    ID_OUTPUT = 400,
    ID_TITLE = 401,
    ID_HINT = 402,
    ID_EXPR = 500,
    ID_DISPLAY = 501,
    ID_KEY = 600
};

enum { GRADES, CALCULATOR, EVEN_ODD, TEMPERATURE, ELECTRICITY, TABLE, GUESS, ATTENDANCE, MODE_COUNT };

const int CLIENT_W = 818, CLIENT_H = 614;
const int SIDE_X = 16, SIDE_W = 208, MAIN_X = 244, MAIN_W = 558, TOP = 92, BOTTOM = 598;
const int ROW_PITCH = 48, ROW_H = 38, LABEL_W = 200;
const int SIDE_PITCH = 66, SIDE_H = 58;

const COLORREF C_BG = RGB(22, 24, 31);
const COLORREF C_PANEL = RGB(14, 16, 21);
const COLORREF C_FIELD = RGB(40, 44, 56);
const COLORREF C_SIDE = RGB(33, 36, 46);
const COLORREF C_KEY = RGB(48, 52, 65);
const COLORREF C_FN = RGB(76, 82, 100);
const COLORREF C_ACCENT = RGB(255, 149, 0);
const COLORREF C_TEXT = RGB(236, 239, 245);
const COLORREF C_MUTED = RGB(140, 148, 168);
const COLORREF C_ERR = RGB(255, 110, 110);

struct Mode {
    const wchar_t* name;
    const wchar_t* hint;
    const wchar_t* fields[4];
    const wchar_t* defaults[4];
    const wchar_t* actions[4];
};

const Mode MODES[MODE_COUNT] = {
    {L"Grade Calculator", L"Test 30%, class activity 30%, project 20%, overall progress 20%.",
     {L"Test (30%)", L"Class activity (30%)", L"Project (20%)", L"Overall progress (20%)"}, {}, {L"Calculate"}},
    {L"Simple Calculator", L"Click the keys or type on your keyboard. Enter = equals, Esc = clear.", {}, {}, {}},
    {L"Even or Odd", L"Checks whether a whole number is even or odd.", {L"Number"}, {}, {L"Check"}},
    {L"Temperature", L"Converts between Celsius and Fahrenheit.", {L"Temperature"}, {},
     {L"\u00B0C \u2192 \u00B0F", L"\u00B0F \u2192 \u00B0C"}},
    {L"Electricity Cost", L"Cost = consumption \u00D7 price per unit.", {L"Consumption (kWh)", L"Price per kWh", L"Days in period"},
     {nullptr, nullptr, L"30"}, {L"Calculate"}},
    {L"Multiplication Table", L"Shows the multiplication table of a number.", {L"Number", L"Up to"}, {nullptr, L"10"}, {L"Show table"}},
    {L"Guess the Number", L"Easy 1-50 (unlimited), Normal 1-100 (10 tries), Hard 1-500 (9 tries).", {L"Your guess"}, {},
     {L"Guess", L"Easy", L"Normal", L"Hard"}},
    {L"Attendance", L"You need at least 75% attendance.", {L"Total classes", L"Absences"}, {}, {L"Calculate"}},
};

struct Key {
    wchar_t code;
    const wchar_t* label;
};

const Key KEYS[20] = {
    {L'C', L"C"}, {L'B', L"\u2190"}, {L'^', L"^"}, {L'/', L"\u00F7"},
    {L'7', L"7"}, {L'8', L"8"}, {L'9', L"9"}, {L'*', L"\u00D7"},
    {L'4', L"4"}, {L'5', L"5"}, {L'6', L"6"}, {L'-', L"\u2212"},
    {L'1', L"1"}, {L'2', L"2"}, {L'3', L"3"}, {L'+', L"+"},
    {L'N', L"\u00B1"}, {L'0', L"0"}, {L'.', L"."}, {L'=', L"="},
};

HWND hMain, hTitle, hHint, hOutput, hExpr, hDisplay;
HWND hModeBtn[MODE_COUNT], hLabel[4], hField[4], hAction[4], hKey[20];
HFONT fontUI, fontSmall, fontTitle, fontDisplay, fontKey, fontMono;
HBRUSH brBG, brPanel, brField;
int dpi = 96;
int currentMode = CALCULATOR;
bool outputError = false;

int S(int v) { return MulDiv(v, dpi, 96); }

RECT rectS(int x, int y, int w, int h) { return RECT{S(x), S(y), S(x + w), S(y + h)}; }

void place(HWND h, int x, int y, int w, int height) { MoveWindow(h, S(x), S(y), S(w), S(height), TRUE); }

HFONT makeFont(int size, int weight, const wchar_t* face) {
    return CreateFontW(-S(size), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, face);
}

void roundRect(HDC dc, RECT r, COLORREF color, int radius) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

COLORREF lighten(COLORREF c, int amount) {
    return RGB(min(255, GetRValue(c) + amount), min(255, GetGValue(c) + amount), min(255, GetBValue(c) + amount));
}

wstring textOf(HWND h) {
    int length = GetWindowTextLengthW(h);
    wstring s(length + 1, L'\0');
    GetWindowTextW(h, &s[0], length + 1);
    s.resize(length);
    return s;
}

wstring trim(const wstring& t) {
    size_t first = t.find_first_not_of(L" \t\r\n");
    if (first == wstring::npos) return L"";
    size_t last = t.find_last_not_of(L" \t\r\n");
    return t.substr(first, last - first + 1);
}

wstring num(double v) {
    if (v == 0) v = 0;
    wostringstream out;
    out << setprecision(12) << v;
    return out.str();
}

wstring fixed2(double v, int decimals = 2) {
    wostringstream out;
    out << fixed << setprecision(decimals) << v;
    wstring s = out.str();
    if (s[0] == L'-' && s.find_first_not_of(L"-0.") == wstring::npos) s.erase(0, 1);
    return s;
}

int fieldCount(int m) {
    int n = 0;
    while (n < 4 && MODES[m].fields[n]) n++;
    return n;
}

int actionCount(int m) {
    int n = 0;
    while (n < 4 && MODES[m].actions[n]) n++;
    return n;
}

int actionsY(int m) {
    int n = fieldCount(m);
    return TOP + n * ROW_PITCH + (n ? 6 : 0);
}

int outputY(int m) { return actionsY(m) + 58; }

void setOutput(const wstring& text, bool error = false) {
    outputError = error;
    SetWindowTextW(hOutput, text.c_str());
    InvalidateRect(hOutput, nullptr, TRUE);
}

void scrollOutputToEnd() {
    int length = GetWindowTextLengthW(hOutput);
    SendMessageW(hOutput, EM_SETSEL, length, length);
    SendMessageW(hOutput, EM_SCROLLCARET, 0, 0);
}

bool parseNumber(wstring t, bool whole, double& out) {
    t = trim(t);
    for (wchar_t& c : t)
        if (c == L',') c = L'.';
    size_t i = (!t.empty() && (t[0] == L'+' || t[0] == L'-')) ? 1 : 0;
    int digits = 0, dots = 0;
    for (; i < t.size(); i++) {
        if (t[i] >= L'0' && t[i] <= L'9') digits++;
        else if (t[i] == L'.') dots++;
        else return false;
    }
    if (digits == 0 || dots > 1 || (whole && dots > 0)) return false;
    out = wcstod(t.c_str(), nullptr);
    return isfinite(out);
}

bool fail(int field, const wstring& message) {
    setOutput(L"Error: " + message, true);
    SetFocus(hField[field]);
    SendMessageW(hField[field], EM_SETSEL, 0, -1);
    return false;
}

bool readField(int i, double low, double high, bool whole, double& out) {
    wstring label = MODES[currentMode].fields[i];
    if (!parseNumber(textOf(hField[i]), whole, out))
        return fail(i, label + (whole ? L" must be a whole number." : L" must be a number."));
    if (out < low || out > high) return fail(i, label + L" must be from " + num(low) + L" to " + num(high) + L".");
    return true;
}

wstring letterGrade(double score) {
    int s = (int)floor(score + 1e-9);
    if (s < 60) return L"F";
    if (s > 100) s = 100;
    const wchar_t letters[] = L"DCBA";
    int band = s == 100 ? 3 : s / 10 - 6;
    int digit = s == 100 ? 9 : s % 10;
    wstring grade(1, letters[band]);
    if (digit <= 3) grade += L'-';
    else if (digit >= 7) grade += L'+';
    return grade;
}

void gradeTool() {
    const wchar_t* names[4] = {L"Test", L"Class activity", L"Project", L"Overall progress"};
    const int weights[4] = {30, 30, 20, 20};
    double scores[4];
    for (int i = 0; i < 4; i++)
        if (!readField(i, 0, 100, false, scores[i])) return;

    double total = (scores[0] * 30 + scores[1] * 30 + scores[2] * 20 + scores[3] * 20) / 100;
    total = round(total * 100) / 100;
    wstring grade = letterGrade(total);

    wostringstream o;
    o << left << setw(22) << L"Component" << right << setw(8) << L"Score" << setw(8) << L"Weight" << setw(9) << L"Points"
      << L"\r\n" << wstring(47, L'-') << L"\r\n";
    for (int i = 0; i < 4; i++)
        o << left << setw(22) << names[i] << right << setw(8) << fixed2(scores[i]) << setw(7) << weights[i] << L"%"
          << setw(9) << fixed2(scores[i] * weights[i] / 100) << L"\r\n";
    o << wstring(47, L'-') << L"\r\n"
      << left << setw(22) << L"Final grade" << right << setw(8) << fixed2(total) << L"   ->   " << grade << L"  ("
      << (grade == L"F" ? L"FAILED" : L"PASSED") << L")\r\n\r\n"
      << L"Scale: 0-59 F, 60-69 D, 70-79 C, 80-89 B, 90-100 A\r\n"
      << L"Last digit 0-3 = minus, 4-6 = plain, 7-9 = plus\r\n"
      << L"Example: 90-93 A-, 94-96 A, 97-100 A+";
    setOutput(o.str());
}

struct Calc {
    wstring entry = L"0";
    wstring expr;
    double acc = 0;
    wchar_t op = 0;
    bool fresh = true;
    bool error = false;
} calc;

wchar_t opSymbol(wchar_t op) {
    if (op == L'-') return L'\u2212';
    if (op == L'*') return L'\u00D7';
    if (op == L'/') return L'\u00F7';
    return op;
}

void refreshDisplay() {
    SetWindowTextW(hExpr, calc.expr.c_str());
    SetWindowTextW(hDisplay, calc.entry.c_str());
}

void calcError(const wstring& message) {
    calc = Calc();
    calc.entry = message;
    calc.error = true;
    refreshDisplay();
}

bool calculate(double a, wchar_t op, double b, double& result) {
    if (op == L'+') result = a + b;
    else if (op == L'-') result = a - b;
    else if (op == L'*') result = a * b;
    else if (op == L'/') {
        if (b == 0) {
            calcError(L"Cannot divide by zero");
            return false;
        }
        result = a / b;
    } else result = pow(a, b);
    if (!isfinite(result)) {
        calcError(L"Error");
        return false;
    }
    return true;
}

int digitCount(const wstring& s) {
    int n = 0;
    for (wchar_t c : s)
        if (c >= L'0' && c <= L'9') n++;
    return n;
}

void calcPress(wchar_t k) {
    if (calc.error) {
        calc = Calc();
        if (k == L'C' || k == L'B') {
            refreshDisplay();
            return;
        }
    }
    double value = wcstod(calc.entry.c_str(), nullptr);

    if (k >= L'0' && k <= L'9') {
        if (calc.fresh || calc.entry == L"0") calc.entry = wstring(1, k);
        else if (calc.entry == L"-0") calc.entry = wstring(L"-") + k;
        else if (digitCount(calc.entry) < 15) calc.entry += k;
        calc.fresh = false;
    } else if (k == L'.') {
        if (calc.fresh) calc.entry = L"0.";
        else if (calc.entry.find(L'.') == wstring::npos) calc.entry += L'.';
        calc.fresh = false;
    } else if (k == L'N') {
        if (calc.entry[0] == L'-') calc.entry.erase(0, 1);
        else if (calc.entry != L"0") calc.entry.insert(0, L"-");
    } else if (k == L'B') {
        if (!calc.fresh) {
            calc.entry.pop_back();
            if (calc.entry.empty() || calc.entry == L"-") calc.entry = L"0";
        }
    } else if (k == L'C') {
        calc = Calc();
    } else if (k == L'=') {
        if (calc.op) {
            double result;
            if (!calculate(calc.acc, calc.op, value, result)) return;
            calc.expr = num(calc.acc) + L" " + opSymbol(calc.op) + L" " + num(value) + L" =";
            calc.entry = num(result);
            calc.acc = result;
            calc.op = 0;
        }
        calc.fresh = true;
    } else {
        if (calc.op && !calc.fresh) {
            double result;
            if (!calculate(calc.acc, calc.op, value, result)) return;
            calc.acc = result;
            calc.entry = num(result);
        } else if (!calc.op) {
            calc.acc = value;
        }
        calc.op = k;
        calc.fresh = true;
        calc.expr = num(calc.acc) + L" " + opSymbol(k);
    }
    refreshDisplay();
}

void evenOddTool() {
    double value;
    if (!readField(0, -999999999999.0, 999999999999.0, true, value)) return;
    long long n = (long long)value;
    long long remainder = n % 2 < 0 ? -(n % 2) : n % 2;
    long long rest = n < 0 ? -n : n;
    wstring bits;
    do {
        bits.insert(bits.begin(), wchar_t(L'0' + rest % 2));
        rest /= 2;
    } while (rest > 0);

    wostringstream o;
    o << n << (remainder == 0 ? L" is EVEN" : L" is ODD") << L"\r\n\r\n"
      << L"Reason:  " << n << L" \u00F7 2 leaves a remainder of " << remainder << L"\r\n"
      << L"Binary:  " << (n < 0 ? L"-" : L"") << bits << L"  (last digit " << remainder << L")";
    setOutput(o.str());
}

wstring feelsLike(double celsius) {
    if (celsius <= 0) return L"Freezing - water turns to ice";
    if (celsius < 15) return L"Cold - grab a jacket";
    if (celsius < 25) return L"Pleasant";
    if (celsius < 35) return L"Hot";
    if (celsius < 100) return L"Very hot!";
    return L"Water boils!";
}

void temperatureTool(int action) {
    double c, f;
    if (action == 0) {
        if (!readField(0, -273.15, 1000000, false, c)) return;
        f = c * 9 / 5 + 32;
    } else {
        if (!readField(0, -459.67, 1000000, false, f)) return;
        c = (f - 32) * 5 / 9;
    }
    wstring line = action == 0 ? fixed2(c) + L" \u00B0C = " + fixed2(f) + L" \u00B0F"
                               : fixed2(f) + L" \u00B0F = " + fixed2(c) + L" \u00B0C";
    setOutput(line + L"\r\n\r\nKelvin:  " + fixed2(c + 273.15) + L" K\r\nFeels:   " + feelsLike(c));
}

void electricityTool() {
    double kwh, price, days;
    if (!readField(0, 0, 10000000, false, kwh) || !readField(1, 0, 10000, false, price) ||
        !readField(2, 1, 366, true, days))
        return;
    double total = kwh * price;
    wostringstream o;
    o << L"Consumption:  " << num(kwh) << L" kWh\r\n"
      << L"Price/unit:   " << num(price) << L"\r\n"
      << L"Daily usage:  " << fixed2(kwh / days) << L" kWh per day\r\n"
      << L"Daily cost:   " << fixed2(total / days) << L"\r\n\r\n"
      << L"TOTAL COST:   " << fixed2(total) << L"\r\n"
      << L"Yearly cost at this rate: " << fixed2(total / days * 365);
    setOutput(o.str());
}

void tableTool() {
    double n, upTo;
    if (!readField(0, -1000000, 1000000, true, n) || !readField(1, 1, 50, true, upTo)) return;
    wostringstream o;
    for (int i = 1; i <= (int)upTo; i++) {
        if (i > 1) o << L"\r\n";
        o << setw(8) << (long long)n << L" x " << setw(2) << i << L" = " << setw(12) << (long long)n * i;
    }
    setOutput(o.str());
}

struct Game {
    int secret = 0, maxNumber = 100, maxTries = 10, tries = 0;
    bool active = false;
    wstring log;
} game;

mt19937 rng(random_device{}() ^ (unsigned)GetTickCount());

void newGame(int level) {
    const int maxNumbers[3] = {50, 100, 500};
    const int triesAllowed[3] = {0, 10, 9};
    const wchar_t* names[3] = {L"Easy", L"Normal", L"Hard"};
    game = Game();
    game.maxNumber = maxNumbers[level];
    game.maxTries = triesAllowed[level];
    game.secret = uniform_int_distribution<int>(1, game.maxNumber)(rng);
    game.active = true;
    game.log = wstring(names[level]) + L" game: I'm thinking of a number from 1 to " + to_wstring(game.maxNumber) + L".\r\n" +
               (game.maxTries ? L"You have " + to_wstring(game.maxTries) + L" tries." : wstring(L"Unlimited tries.")) +
               L"\r\n\r\n";
    setOutput(game.log);
    SetWindowTextW(hField[0], L"");
    SetFocus(hField[0]);
}

void guessTool() {
    if (!game.active) {
        setOutput(game.log + L"Pick Easy, Normal or Hard to play again.");
        return;
    }
    double value;
    if (!readField(0, 1, game.maxNumber, true, value)) return;
    int guess = (int)value;
    game.tries++;
    wstring line = L"Guess " + to_wstring(game.tries) + L": " + to_wstring(guess) + L"  ->  ";
    if (guess == game.secret) {
        line += L"CORRECT! You got it in " + to_wstring(game.tries) + (game.tries == 1 ? L" try." : L" tries.");
        line += L"\r\n\r\nPick Easy, Normal or Hard to play again.";
        game.active = false;
    } else {
        int distance = abs(guess - game.secret);
        line += guess < game.secret ? L"too LOW" : L"too HIGH";
        line += distance <= 3 ? L", burning hot!" : distance <= 10 ? L", warm." : L", cold.";
        if (game.maxTries && game.tries >= game.maxTries) {
            line += L"\r\n\r\nOut of tries! The number was " + to_wstring(game.secret) +
                    L".\r\nPick Easy, Normal or Hard to play again.";
            game.active = false;
        }
    }
    game.log += line + L"\r\n";
    setOutput(game.log);
    scrollOutputToEnd();
    SetWindowTextW(hField[0], L"");
    SetFocus(hField[0]);
}

void attendanceTool() {
    double totalValue, absenceValue;
    if (!readField(0, 1, 100000, true, totalValue) || !readField(1, 0, totalValue, true, absenceValue)) return;
    long long total = (long long)totalValue, absences = (long long)absenceValue, attended = total - absences;
    double percent = attended * 100.0 / total;
    int filled = (int)(percent / 5);

    wostringstream o;
    o << L"Attended:    " << attended << L" of " << total << L" classes\r\n"
      << L"Attendance:  " << fixed2(percent) << L"%\r\n"
      << L"[" << wstring(filled, L'#') << wstring(20 - filled, L'.') << L"]\r\n\r\n";
    if (attended * 4 >= total * 3)
        o << L"OK - you meet the 75% requirement.\r\nYou can still miss " << (4 * attended - 3 * total) / 3
          << L" more class(es) and stay at 75%.";
    else
        o << L"WARNING - below the 75% requirement.\r\nAttend the next " << 3 * total - 4 * attended
          << L" class(es) in a row to reach 75%.";
    setOutput(o.str());
}

void runAction(int a) {
    switch (currentMode) {
    case GRADES: gradeTool(); break;
    case EVEN_ODD: evenOddTool(); break;
    case TEMPERATURE: temperatureTool(a); break;
    case ELECTRICITY: electricityTool(); break;
    case TABLE: tableTool(); break;
    case GUESS:
        if (a == 0) guessTool();
        else newGame(a - 1);
        break;
    case ATTENDANCE: attendanceTool(); break;
    }
}

void showMode(int m) {
    currentMode = m;
    const Mode& mode = MODES[m];
    bool isCalc = m == CALCULATOR;
    int fields = fieldCount(m), actions = actionCount(m), ay = actionsY(m), oy = outputY(m);

    SetWindowTextW(hTitle, mode.name);
    SetWindowTextW(hHint, mode.hint);
    for (int i = 0; i < 4; i++) {
        bool hasField = i < fields;
        if (hasField) {
            int y = TOP + i * ROW_PITCH;
            SetWindowTextW(hLabel[i], mode.fields[i]);
            SetWindowTextW(hField[i], mode.defaults[i] ? mode.defaults[i] : L"");
            place(hLabel[i], MAIN_X, y, LABEL_W, ROW_H);
            place(hField[i], MAIN_X + LABEL_W + 22, y + 9, MAIN_W - LABEL_W - 34, ROW_H - 18);
        }
        ShowWindow(hLabel[i], hasField ? SW_SHOW : SW_HIDE);
        ShowWindow(hField[i], hasField ? SW_SHOW : SW_HIDE);

        bool hasAction = i < actions;
        if (hasAction) {
            SetWindowTextW(hAction[i], mode.actions[i]);
            place(hAction[i], MAIN_X + i * 142, ay, 132, 42);
            InvalidateRect(hAction[i], nullptr, TRUE);
        }
        ShowWindow(hAction[i], hasAction ? SW_SHOW : SW_HIDE);
    }
    place(hOutput, MAIN_X + 14, oy + 12, MAIN_W - 28, BOTTOM - oy - 24);
    ShowWindow(hOutput, isCalc ? SW_HIDE : SW_SHOW);
    ShowWindow(hExpr, isCalc ? SW_SHOW : SW_HIDE);
    ShowWindow(hDisplay, isCalc ? SW_SHOW : SW_HIDE);
    for (HWND key : hKey) ShowWindow(key, isCalc ? SW_SHOW : SW_HIDE);
    for (HWND button : hModeBtn) InvalidateRect(button, nullptr, TRUE);
    InvalidateRect(hMain, nullptr, TRUE);

    setOutput(L"");
    if (fields) SetFocus(hField[0]);
    else SetFocus(hMain);

    if (m == GUESS) {
        if (game.log.empty()) newGame(1);
        else {
            setOutput(game.log);
            scrollOutputToEnd();
        }
    }
}

bool handleKey(const MSG& m) {
    if (m.message != WM_KEYDOWN && m.message != WM_CHAR) return false;
    if (currentMode == CALCULATOR) {
        if (m.message == WM_KEYDOWN) {
            if (m.wParam == VK_RETURN) calcPress(L'=');
            else if (m.wParam == VK_BACK) calcPress(L'B');
            else if (m.wParam == VK_ESCAPE || m.wParam == VK_DELETE) calcPress(L'C');
            else return false;
            return true;
        }
        wchar_t c = (wchar_t)m.wParam;
        if (c == L',') c = L'.';
        if (c == L'x' || c == L'X') c = L'*';
        if (c && wcschr(L"0123456789.+-*/^=", c)) {
            calcPress(c);
            return true;
        }
        return false;
    }
    if (m.message == WM_KEYDOWN && m.wParam == VK_RETURN) {
        HWND focus = GetFocus();
        for (int i = 0; i < 4; i++)
            if (focus == hAction[i]) {
                runAction(i);
                return true;
            }
        int fields = fieldCount(currentMode);
        for (int i = 0; i + 1 < fields; i++)
            if (focus == hField[i]) {
                SetFocus(hField[i + 1]);
                SendMessageW(hField[i + 1], EM_SETSEL, 0, -1);
                return true;
            }
        if (actionCount(currentMode) > 0) runAction(0);
        return true;
    }
    return false;
}

void drawButton(const DRAWITEMSTRUCT* d) {
    int id = (int)d->CtlID;
    HDC dc = d->hDC;
    RECT rc = d->rcItem;
    bool isMode = id >= ID_MODE && id < ID_MODE + MODE_COUNT;
    COLORREF fill = C_FN;
    HFONT font = fontUI;

    if (isMode) fill = id - ID_MODE == currentMode ? C_ACCENT : C_SIDE;
    else if (id >= ID_ACTION && id < ID_ACTION + 4) fill = id == ID_ACTION ? C_ACCENT : C_FN;
    else if (id >= ID_KEY && id < ID_KEY + 20) {
        wchar_t k = KEYS[id - ID_KEY].code;
        bool digit = (k >= L'0' && k <= L'9') || k == L'.';
        fill = digit ? C_KEY : (k == L'C' || k == L'B' || k == L'N') ? C_FN : C_ACCENT;
        font = fontKey;
    }
    if (d->itemState & ODS_SELECTED) fill = lighten(fill, 30);

    FillRect(dc, &rc, brBG);
    roundRect(dc, rc, fill, S(12));
    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ oldFont = SelectObject(dc, font);
    wstring text = textOf(d->hwndItem);

    if (isMode) {
        int index = id - ID_MODE;
        wstring tag = to_wstring(index + 1);
        RECT t = rc;
        t.left += S(16);
        SetTextColor(dc, index == currentMode ? C_TEXT : C_MUTED);
        DrawTextW(dc, tag.c_str(), -1, &t, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        t.left += S(28);
        SetTextColor(dc, C_TEXT);
        DrawTextW(dc, text.c_str(), -1, &t, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SetTextColor(dc, C_TEXT);
        DrawTextW(dc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(dc, oldFont);
}

void paintWindow(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client;
    GetClientRect(hwnd, &client);
    FillRect(dc, &client, brBG);
    SetBkMode(dc, TRANSPARENT);

    HGDIOBJ oldFont = SelectObject(dc, fontTitle);
    SetTextColor(dc, C_TEXT);
    RECT r = rectS(SIDE_X + 4, 14, SIDE_W, 36);
    DrawTextW(dc, L"OmniCalc", -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, fontSmall);
    SetTextColor(dc, C_ACCENT);
    r = rectS(SIDE_X + 4, 50, SIDE_W, 24);
    DrawTextW(dc, L"8-in-1 calculator", -1, &r, DT_LEFT | DT_SINGLELINE);
    SelectObject(dc, oldFont);

    if (currentMode == CALCULATOR) {
        roundRect(dc, rectS(MAIN_X, TOP, MAIN_W, 120), C_PANEL, S(16));
    } else {
        for (int i = 0; i < fieldCount(currentMode); i++)
            roundRect(dc, rectS(MAIN_X + LABEL_W + 10, TOP + i * ROW_PITCH, MAIN_W - LABEL_W - 10, ROW_H), C_FIELD, S(10));
        int oy = outputY(currentMode);
        roundRect(dc, rectS(MAIN_X, oy, MAIN_W, BOTTOM - oy), C_PANEL, S(16));
    }
    EndPaint(hwnd, &ps);
}

HWND make(const wchar_t* type, const wchar_t* text, DWORD style, int id, HFONT font) {
    HWND h = CreateWindowExW(0, type, text, WS_CHILD | style, 0, 0, 0, 0, hMain, (HMENU)(INT_PTR)id,
                             GetModuleHandleW(nullptr), nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)font, TRUE);
    return h;
}

void useDarkParts() {
    typedef HRESULT(WINAPI * DwmSetAttribute)(HWND, DWORD, LPCVOID, DWORD);
    typedef HRESULT(WINAPI * SetTheme)(HWND, LPCWSTR, LPCWSTR);
    BOOL on = TRUE;
    if (HMODULE dwm = LoadLibraryW(L"dwmapi.dll")) {
        DwmSetAttribute setAttribute = (DwmSetAttribute)(void*)GetProcAddress(dwm, "DwmSetWindowAttribute");
        if (setAttribute) setAttribute(hMain, 20, &on, sizeof(on));
    }
    if (HMODULE theme = LoadLibraryW(L"uxtheme.dll")) {
        SetTheme setTheme = (SetTheme)(void*)GetProcAddress(theme, "SetWindowTheme");
        if (setTheme) setTheme(hOutput, L"DarkMode_Explorer", nullptr);
    }
}

void createControls() {
    hTitle = make(L"STATIC", L"", WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, ID_TITLE, fontTitle);
    place(hTitle, MAIN_X, 14, MAIN_W, 36);
    hHint = make(L"STATIC", L"", WS_VISIBLE | SS_LEFT | SS_NOPREFIX, ID_HINT, fontSmall);
    place(hHint, MAIN_X, 52, MAIN_W, 24);

    for (int i = 0; i < 4; i++) {
        hLabel[i] = make(L"STATIC", L"", SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, ID_LABEL + i, fontUI);
        hField[i] = make(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, ID_FIELD + i, fontUI);
        SendMessageW(hField[i], EM_LIMITTEXT, 40, 0);
    }
    for (int i = 0; i < 4; i++) hAction[i] = make(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, ID_ACTION + i, fontUI);

    hOutput = make(L"EDIT", L"", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, ID_OUTPUT, fontMono);

    hExpr = make(L"STATIC", L"", SS_RIGHT | SS_NOPREFIX, ID_EXPR, fontUI);
    place(hExpr, MAIN_X + 18, TOP + 12, MAIN_W - 36, 26);
    hDisplay = make(L"STATIC", L"0", SS_RIGHT | SS_NOPREFIX | SS_CENTERIMAGE, ID_DISPLAY, fontDisplay);
    place(hDisplay, MAIN_X + 18, TOP + 40, MAIN_W - 36, 70);
    for (int i = 0; i < 20; i++) {
        hKey[i] = make(L"BUTTON", KEYS[i].label, BS_OWNERDRAW, ID_KEY + i, fontKey);
        place(hKey[i], MAIN_X + (i % 4) * 142, TOP + 136 + (i / 4) * 76, 132, 66);
    }

    for (int i = 0; i < MODE_COUNT; i++) {
        hModeBtn[i] = make(L"BUTTON", MODES[i].name, WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP, ID_MODE + i, fontUI);
        place(hModeBtn[i], SIDE_X, 76 + i * SIDE_PITCH, SIDE_W, SIDE_H);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        hMain = hwnd;
        createControls();
        useDarkParts();
        showMode(CALCULATOR);
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (code != BN_CLICKED && code != BN_DOUBLECLICKED) break;
        if (id >= ID_MODE && id < ID_MODE + MODE_COUNT) showMode(id - ID_MODE);
        else if (id >= ID_ACTION && id < ID_ACTION + 4) runAction(id - ID_ACTION);
        else if (id >= ID_KEY && id < ID_KEY + 20) {
            calcPress(KEYS[id - ID_KEY].code);
            SetFocus(hwnd);
        }
        return 0;
    }
    case WM_DRAWITEM:
        drawButton((const DRAWITEMSTRUCT*)lp);
        return TRUE;
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp;
        int id = GetDlgCtrlID((HWND)lp);
        if (id == ID_OUTPUT || id == ID_EXPR || id == ID_DISPLAY) {
            SetTextColor(dc, id == ID_EXPR ? C_MUTED : (id == ID_OUTPUT && outputError) ? C_ERR : C_TEXT);
            SetBkColor(dc, C_PANEL);
            return (LRESULT)brPanel;
        }
        SetTextColor(dc, id == ID_HINT ? C_MUTED : C_TEXT);
        SetBkColor(dc, C_BG);
        return (LRESULT)brBG;
    }
    case WM_CTLCOLOREDIT:
        SetTextColor((HDC)wp, C_TEXT);
        SetBkColor((HDC)wp, C_FIELD);
        return (LRESULT)brField;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        paintWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    SetProcessDPIAware();
    HDC screen = GetDC(nullptr);
    dpi = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(nullptr, screen);

    brBG = CreateSolidBrush(C_BG);
    brPanel = CreateSolidBrush(C_PANEL);
    brField = CreateSolidBrush(C_FIELD);
    fontUI = makeFont(15, FW_NORMAL, L"Segoe UI");
    fontSmall = makeFont(13, FW_NORMAL, L"Segoe UI");
    fontTitle = makeFont(24, FW_SEMIBOLD, L"Segoe UI");
    fontDisplay = makeFont(46, FW_LIGHT, L"Segoe UI");
    fontKey = makeFont(22, FW_NORMAL, L"Segoe UI");
    fontMono = makeFont(14, FW_NORMAL, L"Consolas");

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = brBG;
    wc.lpszClassName = L"OmniCalcWindow";
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT size = {0, 0, S(CLIENT_W), S(CLIENT_H)};
    AdjustWindowRectEx(&size, style, FALSE, WS_EX_CONTROLPARENT);
    CreateWindowExW(WS_EX_CONTROLPARENT, L"OmniCalcWindow", L"OmniCalc", style, CW_USEDEFAULT, CW_USEDEFAULT,
                    size.right - size.left, size.bottom - size.top, nullptr, nullptr, instance, nullptr);
    ShowWindow(hMain, show);
    UpdateWindow(hMain);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (handleKey(msg) || IsDialogMessageW(hMain, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
