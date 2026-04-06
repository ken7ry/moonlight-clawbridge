#include "clawbridge.h"
#include "streaming/session.h"

#include <Limelight.h>
#include <QStringList>
#include <cstring>
#include <algorithm>

// ================================================================
// Virtual gamepad state (NOT registered as system controller)
// Keyboard and gamepad work independently — no input conflict!
// ================================================================
static struct {
    int     buttonFlags = 0;
    unsigned char lt = 0, rt = 0;
    short   lsX = 0, lsY = 0, rsX = 0, rsY = 0;
} g_cbCtrl;

// ================================================================
// Safe push helpers
// ================================================================
template<typename T>
static void cleanupPayload(T* p) { delete p; }

template<>
inline void cleanupPayload<CBEvt_Txt>(CBEvt_Txt* p) {
    delete[] p->str;
    delete p;
}

template<typename PAYLOAD>
static void push(CB_Uint32 evType, const PAYLOAD& p)
{
    PAYLOAD* heap = new PAYLOAD(p);
    SDL_Event ev = {};
    ev.type         = static_cast<Uint32>(evType);
    ev.user.code    = 0;
    ev.user.data1   = static_cast<void*>(heap);
    ev.user.data2   = nullptr;
    if (SDL_PushEvent(&ev) < 0) {
        cleanupPayload(heap);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[ClawBridge] SDL_PushEvent failed: event=%u", evType);
    }
}

static void pushText(CB_Uint32 evType, char* str)
{
    CBEvt_Txt* heap = new CBEvt_Txt(str);
    SDL_Event ev = {};
    ev.type       = static_cast<Uint32>(evType);
    ev.user.code  = 0;
    ev.user.data1 = heap;
    ev.user.data2 = nullptr;
    if (SDL_PushEvent(&ev) < 0) {
        cleanupPayload(heap);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[ClawBridge] SDL_PushEvent failed: CB_EV_TEXT");
    }
}

// ================================================================
// Server Implementation
// ================================================================

ClawBridgeServer::ClawBridgeServer(QObject* parent)
    : QObject(parent)
{
    m_server = new QTcpServer(this);
    m_server->setMaxPendingConnections(4);
    connect(m_server, &QTcpServer::newConnection, this, &ClawBridgeServer::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, 9999)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[ClawBridge] Failed to listen on port 9999: %s",
                     qUtf8Printable(m_server->errorString()));
    } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "[ClawBridge] TCP server listening on 0.0.0.0:9999");
    }
}

ClawBridgeServer::~ClawBridgeServer()
{
    if (m_server && m_server->isListening()) m_server->close();
    for (auto* sock : m_clients) {
        sock->disconnect();
        sock->deleteLater();
    }
    qDeleteAll(m_clients);
    m_clients.clear();
}

void ClawBridgeServer::onNewConnection()
{
    while (QTcpSocket* sock = m_server->nextPendingConnection()) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "[ClawBridge] Client connected: %s",
                    qUtf8Printable(sock->peerAddress().toString()));
        m_clients.append(sock);
        connect(sock, &QTcpSocket::readyRead, this, &ClawBridgeServer::onReadyRead);
        connect(sock, &QTcpSocket::disconnected, this, &ClawBridgeServer::onDisconnect);
    }
}

void ClawBridgeServer::onReadyRead()
{
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    while (sock->canReadLine()) {
        QByteArray raw = sock->readLine().trimmed();
        if (raw.isEmpty()) continue;
        parseLine(QString::fromUtf8(raw));
    }
}

void ClawBridgeServer::onDisconnect()
{
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (sock) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "[ClawBridge] Client disconnected");
        sock->disconnect();
        sock->deleteLater();
        m_clients.removeAll(sock);
    }
}

// ================================================================
// Command Parser
// ================================================================

void ClawBridgeServer::parseLine(const QString& cmd)
{
    // ----- Gamepad Buttons -----
    if (cmd.startsWith("GP:")) {
        QString part = cmd.mid(3).toUpper();
        bool pressed = true;
        if (part.endsWith("_PRESS"))       { pressed = true;  part.chop(6); }
        else if (part.endsWith("_RELEASE")) { pressed = false; part.chop(8); }
        else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "[ClawBridge] GP: requires _PRESS or _RELEASE");
            return;
        }
        CB_Uint32 flag = nameToButton(part);
        if (flag == 0) return;
        push<CBEvt_Gp>(CB_EV_GAMEPAD, { flag, static_cast<CB_Uint8>(pressed ? SDL_PRESSED : SDL_RELEASED) });
        return;
    }

    // ----- Joystick -----
    // Format: Joy:<lx>,<ly>,<rx>,<ry>
    // Values: -32768 to 32767 (0,0 = centered)
    if (cmd.startsWith("Joy:")) {
        QStringList nums = cmd.mid(4).split(",");
        if (nums.size() == 4) {
            short lx = nums[0].toShort();
            short ly = nums[1].toShort();
            short rx = nums[2].toShort();
            short ry = nums[3].toShort();
            push<CBEvt_Js>(CB_EV_JOYSTICK, { lx, ly, rx, ry, 0, 0 });
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "[ClawBridge] Joy: expects lx,ly,rx,ry");
        }
        return;
    }

    // ----- Triggers -----
    // Format: Trig:<LT>,<RT>   (values 0-255)
    if (cmd.startsWith("Trig:")) {
        QStringList nums = cmd.mid(5).split(",");
        if (nums.size() == 2) {
            unsigned char lt = (unsigned char)nums[0].toShort();
            unsigned char rt = (unsigned char)nums[1].toShort();
            push<CBEvt_Js>(CB_EV_JOYSTICK, { 0, 0, 0, 0, lt, rt });
        }
        return;
    }

    // ----- Keyboard -----
    if (cmd.startsWith("KB:")) {
        QString part = cmd.mid(3).toUpper();
        bool pressed = true;
        if (part.endsWith("_DOWN"))      { pressed = true;  part.chop(5); }
        else if (part.endsWith("_UP"))   { pressed = false; part.chop(3); }
        else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "[ClawBridge] KB: requires _DOWN or _UP");
            return;
        }
        CB_short vk = nameToKeyCode(part);
        if (vk == 0) return;
        push<CBEvt_Kb>(CB_EV_KEYBOARD, { vk, static_cast<CB_Uint8>(pressed ? SDL_PRESSED : SDL_RELEASED), 0 });
        return;
    }

    // ----- Mouse Button -----
    if (cmd.startsWith("MB:")) {
        QString part = cmd.mid(3).toUpper();
        bool pressed = true;
        if (part.endsWith("_PRESS"))       { pressed = true;  part.chop(6); }
        else if (part.endsWith("_RELEASE")) { pressed = false; part.chop(8); }
        else {
            int sep = part.indexOf('_');
            if (sep > 0) {
                QString btnStr = part.left(sep);
                QString actStr = part.mid(sep + 1);
                if (actStr == "PRESS" || actStr == "RELEASE") {
                    bool ok = false;
                    int btn = btnStr.toInt(&ok);
                    if (ok && btn >= 1 && btn <= 9) {
                        push<CBEvt_Mb>(CB_EV_MOUSEBTN,
                            { static_cast<CB_Uint8>(btn),
                              static_cast<CB_Uint8>(actStr == "PRESS" ? SDL_PRESSED : SDL_RELEASED) });
                        return;
                    }
                }
            }
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "[ClawBridge] MB: requires _PRESS or _RELEASE");
            return;
        }
        CB_Uint8 btn = nameToMouseBtn(part);
        if (btn == 0) return;
        push<CBEvt_Mb>(CB_EV_MOUSEBTN,
            { btn, static_cast<CB_Uint8>(pressed ? SDL_PRESSED : SDL_RELEASED) });
        return;
    }

    // ----- Mouse Move (Relative) -----
    if (cmd.startsWith("MV:")) {
        QStringList nums = cmd.mid(3).split(",");
        if (nums.size() == 2) {
            CB_short dx = nums[0].toShort();
            CB_short dy = nums[1].toShort();
            if (dx != 0 || dy != 0)
                push<CBEvt_Mv>(CB_EV_MOUSEMOVE, { dx, dy });
        }
        return;
    }

    // ----- Mouse Position (Absolute) -----
    if (cmd.startsWith("MP:")) {
        QStringList nums = cmd.mid(3).split(",");
        if (nums.size() == 4) {
            CB_short x = nums[0].toShort();
            CB_short y = nums[1].toShort();
            CB_short w = nums[2].toShort();
            CB_short h = nums[3].toShort();
            push<CBEvt_Mp>(CB_EV_MOUSEPOS, { x, y, w, h });
        }
        return;
    }

    // ----- Vertical Scroll -----
    if (cmd.startsWith("SL:")) {
        short val = cmd.mid(3).toShort();
        push<CBEvt_Wh>(CB_EV_SCROLL, { 0, val });
        return;
    }

    // ----- Horizontal Scroll -----
    if (cmd.startsWith("HL:")) {
        short val = cmd.mid(3).toShort();
        push<CBEvt_Wh>(CB_EV_SCROLL, { val, 0 });
        return;
    }

    // ----- Text Input -----
    if (cmd.startsWith("TXT:")) {
        QString text = cmd.mid(4);
        if (text.isEmpty()) return;
        QByteArray utf8 = text.toUtf8();
        char* copy = new char[static_cast<size_t>(utf8.size()) + 1];
        std::memcpy(copy, utf8.constData(), static_cast<size_t>(utf8.size()) + 1);
        pushText(CB_EV_TEXT, copy);
        return;
    }

    // ----- Ping -----
    if (cmd == "PING" || cmd == "p") {
        auto* sock = qobject_cast<QTcpSocket*>(sender());
        if (sock) sock->write("PONG\n");
        return;
    }

    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "[ClawBridge] 无法识别指令: %s", qUtf8Printable(cmd));
}

// ================================================================
// Key Name → Windows VK Code
// ================================================================
CB_short ClawBridgeServer::nameToKeyCode(const QString& name)
{
    if (name.length() == 1 && name[0].isLetter())
        return static_cast<CB_short>(0x41 + (name[0].toUpper().unicode() - 'A'));
    if (name.length() == 1 && name[0].isDigit())
        return static_cast<CB_short>(0x30 + name[0].unicode() - '0');
    if (name.length() >= 2 && name.startsWith("F")) {
        bool ok;
        int n = name.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 12) return static_cast<CB_short>(0x70 + n - 1);
        if (ok && n >= 13 && n <= 24) return static_cast<CB_short>(0x7C + n - 13);
    }

    struct Entry { const char* n; CB_short v; };
    static const Entry map[] = {
        { "ENTER", 0x0D }, { "RETURN", 0x0D }, { "ESC", 0x1B }, { "ESCAPE", 0x1B },
        { "BACKSPACE", 0x08 }, { "BKSP", 0x08 }, { "TAB", 0x09 }, { "SPACE", 0x20 },
        { "CAPSLOCK", 0x14 }, { "PRINTSCREEN", 0x2C }, { "PRTSCR", 0x2C },
        { "SCROLLLOCK", 0x91 }, { "PAUSE", 0x13 }, { "INSERT", 0x2D }, { "INS", 0x2D },
        { "DELETE", 0x2E }, { "DEL", 0x2E }, { "HOME", 0x24 }, { "END", 0x23 },
        { "PAGEUP", 0x21 }, { "PGUP", 0x21 }, { "PAGEDOWN", 0x22 }, { "PGDN", 0x22 },
        { "KB_UP", 0x26 }, { "KB_DOWN", 0x28 }, { "KB_LEFT", 0x25 }, { "KB_RIGHT", 0x27 },
        { "LSHIFT", 0xA0 }, { "RSHIFT", 0xA1 }, { "LCTRL", 0xA2 }, { "RCTRL", 0xA3 },
        { "LALT", 0xA4 }, { "RALT", 0xA5 }, { "LWIN", 0x5B }, { "RWIN", 0x5C }, { "GUI", 0x5B },
        { "MINUS", 0xBD }, { "EQUALS", 0xBB },
        { "LBRACKET", 0xDB }, { "LBRA", 0xDB }, { "RBRACKET", 0xDD }, { "RBRA", 0xDD },
        { "BACKSLASH", 0xDC }, { "SEMICOLON", 0xBA }, { "APOSTROPHE", 0xDE },
        { "COMMA", 0xBC }, { "PERIOD", 0xBE }, { "SLASH", 0xBF },
        { "BACKTICK", 0xC0 }, { "GRAVE", 0xC0 },
        { "NUMPAD0", 0x60 }, { "NP0", 0x60 },
        { "NUMPAD1", 0x61 }, { "NP1", 0x61 },
        { "NUMPAD2", 0x62 }, { "NP2", 0x62 },
        { "NUMPAD3", 0x63 }, { "NP3", 0x63 },
        { "NUMPAD4", 0x64 }, { "NP4", 0x64 },
        { "NUMPAD5", 0x65 }, { "NP5", 0x65 },
        { "NUMPAD6", 0x66 }, { "NP6", 0x66 },
        { "NUMPAD7", 0x67 }, { "NP7", 0x67 },
        { "NUMPAD8", 0x68 }, { "NP8", 0x68 },
        { "NUMPAD9", 0x69 }, { "NP9", 0x69 },
        { "NUMPADMULT", 0x6A }, { "NPMUL", 0x6A },
        { "NUMPADADD", 0x6B }, { "NPADD", 0x6B },
        { "NUMPADSUB", 0x6D }, { "NPSUB", 0x6D },
        { "NUMPADDIV", 0x6F }, { "NPDIV", 0x6F },
        { "NUMPADDEC", 0x6E }, { "NPDEC", 0x6E },
        { "NUMPADENTER", 0x0D }, { "NPENTER", 0x0D },
    };

    QByteArray ba = name.toUtf8();
    for (auto& e : map) {
        if (ba == e.n) return e.v;
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "[ClawBridge] 未知按键: %s", qUtf8Printable(name));
    return 0;
}

// ================================================================
// Mouse button name → Limelight button number
// ================================================================
CB_Uint8 ClawBridgeServer::nameToMouseBtn(const QString& name)
{
    // Numeric buttons: 1=left, 2=middle, 3=right, 4=X1, 5=X2
    bool ok = false;
    int n = name.toInt(&ok);
    if (ok && n >= 1 && n <= 5) return (CB_Uint8)n;

    if (name == "LEFT"   || name == "BTN_LEFT")   return 1;
    if (name == "RIGHT"  || name == "BTN_RIGHT")  return 3;
    if (name == "MIDDLE" || name == "BTN_MIDDLE" || name == "WHEEL") return 2;
    if (name == "X1" || name == "BTN_X1" || name == "BACK")    return 4;
    if (name == "X2" || name == "BTN_X2" || name == "FORWARD") return 5;
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "[ClawBridge] 未知鼠标按键: %s", qUtf8Printable(name));
    return 0;
}

// ================================================================
// Gamepad button name → Limelight button flag
// ================================================================
CB_Uint32 ClawBridgeServer::nameToButton(const QString& name)
{
    if (name.length() >= 3 && name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) {
        bool ok;
        uint val = name.mid(2).toUInt(&ok, 16);
        if (ok) return val;
    }

    struct Entry { const char* n; CB_Uint32 f; };
    static const Entry map[] = {
        { "A", 0x1000 }, { "B", 0x2000 }, { "X", 0x4000 }, { "Y", 0x8000 },
        { "START", 0x0010 }, { "OPTIONS", 0x0010 }, { "PLAY", 0x0010 },
        { "BACK", 0x0020 }, { "SELECT", 0x0020 },
        { "LS_CLICK", 0x0040 }, { "L3", 0x0040 },
        { "RS_CLICK", 0x0080 }, { "R3", 0x0080 },
        { "LB", 0x0100 }, { "L1", 0x0100 },
        { "RB", 0x0200 }, { "R1", 0x0200 },
        { "SPECIAL", 0x0400 }, { "PS", 0x0400 },
        { "DPAD_UP", 0x0001 }, { "UP", 0x0001 }, { "D_UP", 0x0001 },
        { "DPAD_DOWN", 0x0002 }, { "DOWN", 0x0002 }, { "D_DOWN", 0x0002 },
        { "DPAD_LEFT", 0x0004 }, { "LEFT", 0x0004 }, { "D_LEFT", 0x0004 },
        { "DPAD_RIGHT", 0x0008 }, { "RIGHT", 0x0008 }, { "D_RIGHT", 0x0008 },
    };

    QByteArray ba = name.toUtf8();
    for (auto& e : map) {
        if (ba == e.n) return e.f;
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "[ClawBridge] 未知手柄按键: %s", qUtf8Printable(name));
    return 0;
}
