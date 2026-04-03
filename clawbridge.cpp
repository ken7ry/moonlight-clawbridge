#include "clawbridge.h"
#include "streaming/session.h"

#include <Limelight.h>
#include <QStringList>
#include <cstring>

// ================================================================
// Event payload structs (one per event type)
// ================================================================
struct CBEvt_Gp  { CB_Uint32 flags; CB_Uint8 action; };
struct CBEvt_Kb  { CB_short vk;   CB_Uint8 action; char mods; };
struct CBEvt_Mb  { CB_Uint8 btn;   CB_Uint8 action; };
struct CBEvt_Wh  { CB_short dx;    CB_short dy; };
struct CBEvt_Mv  { CB_short dx;    CB_short dy; };
struct CBEvt_Txt { char* str; };

// ================================================================
// Safe push: checks SDL_PushEvent return, frees on failure
// Special handling for CB_EV_TEXT (heap->str needs delete[])
// ================================================================
template<typename PAYLOAD>
static void push(CB_Uint32 evType, const PAYLOAD& p)
{
    PAYLOAD* heap = new PAYLOAD(p);
    SDL_Event ev = {};
    ev.type         = static_cast<Uint32>(evType);
    ev.user.code    = 0;
    ev.user.data1   = static_cast<void*>(const_cast<PAYLOAD*>(heap));
    ev.user.data2   = nullptr;
    if (SDL_PushEvent(&ev) < 0) {
        if (evType == CB_EV_TEXT) {
            CBEvt_Txt* txtPayload = reinterpret_cast<CBEvt_Txt*>(heap);
            delete[] txtPayload->str;
        }
        delete heap;
    }
}

// ================================================================
// Constructor — start TCP listener on :9999
// ================================================================
ClawBridgeServer::ClawBridgeServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    if (!m_server->listen(QHostAddress::Any, 9999)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "[ClawBridge] Listen on :9999 failed: %s",
                    qUtf8Printable(m_server->errorString()));
        return;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "[ClawBridge] TCP listening on 0.0.0.0:9999");

    connect(m_server, &QTcpServer::newConnection,
            this, &ClawBridgeServer::onNewConnection);
}

ClawBridgeServer::~ClawBridgeServer()
{
    if (m_server && m_server->isListening())
        m_server->close();
}

// ================================================================
// Client connection / read / disconnect
// ================================================================
void ClawBridgeServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* sock = m_server->nextPendingConnection();
        if (!sock) continue;

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "[ClawBridge] Connected: %s",
                    qUtf8Printable(sock->peerAddress().toString()));
        m_clients.append(sock);

        connect(sock, &QTcpSocket::readyRead,
                this, &ClawBridgeServer::onReadyRead);
        connect(sock, &QTcpSocket::disconnected,
                this, &ClawBridgeServer::onDisconnect);

        // Auto-kick after 5 min idle
        QTimer* idle = new QTimer(sock);
        idle->setSingleShot(true);
        idle->setInterval(300000);
        connect(idle, &QTimer::timeout, sock, &QTcpSocket::disconnectFromHost);
        connect(sock, &QTcpSocket::readyRead, idle,
                QOverload<>::of(&QTimer::start));
    }
}

void ClawBridgeServer::onReadyRead()
{
    QTcpSocket* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    // OOM defense: cap line length to 1024 bytes
    while (sock->canReadLine()) {
        QString line = QString::fromUtf8(sock->readLine(1024)).trimmed();
        if (line.isEmpty()) continue;
        parseLine(line);
    }
}

void ClawBridgeServer::onDisconnect()
{
    QTcpSocket* sock = qobject_cast<QTcpSocket*>(sender());
    if (sock) {
        // Fully disconnect all signal/slot connections to prevent phantom calls
        // from dangling timers during high-freq connect/disconnect cycles
        sock->disconnect();
        m_clients.removeAll(sock);
        sock->deleteLater();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] Disconnected");
    }
}

// ================================================================
// Command parser — ALL injection goes through SDL event push
// (No direct LiSend* calls — must be on the main thread)
// ================================================================
void ClawBridgeServer::parseLine(const QString& raw)
{
    QString cmd = raw;

    // ----- Gamepad -----
    if (cmd.startsWith("GP:")) {
        QString part = cmd.mid(3).toUpper();
        bool pressed = true;
        if (part.endsWith("_DOWN"))      { pressed = true;  part.chop(5); }
        else if (part.endsWith("_UP"))   { pressed = false; part.chop(3); }
        else { SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] GP: needs _DOWN/_UP"); return; }

        CB_Uint32 flag = nameToButton(part);
        if (flag == 0) return;

        push<CBEvt_Gp>(CB_EV_GAMEPAD, { flag, pressed ? SDL_PRESSED : SDL_RELEASED });
        return;
    }

    // ----- Keyboard -----
    if (cmd.startsWith("KB:")) {
        QString part = cmd.mid(3).toUpper();
        bool pressed = true;
        if (part.endsWith("_DOWN"))      { pressed = true;  part.chop(5); }
        else if (part.endsWith("_UP"))   { pressed = false; part.chop(3); }
        else { SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] KB: needs _DOWN/_UP"); return; }

        CB_short vk = nameToKeyCode(part);
        if (vk == 0) return;

        push<CBEvt_Kb>(CB_EV_KEYBOARD, { vk, pressed ? SDL_PRESSED : SDL_RELEASED, 0 });
        return;
    }

    // ----- Mouse Button -----
    if (cmd.startsWith("MB:")) {
        QString part = cmd.mid(3).toUpper();
        bool pressed = true;
        if (part.endsWith("_DOWN"))      { pressed = true;  part.chop(5); }
        else if (part.endsWith("_UP"))   { pressed = false; part.chop(3); }
        else { SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] MB: needs _DOWN/_UP"); return; }

        CB_Uint8 btn = nameToMouseBtn(part);
        if (btn == 0) return;

        push<CBEvt_Mb>(CB_EV_MOUSEBTN, { btn, pressed ? SDL_PRESSED : SDL_RELEASED });
        return;
    }

    // ----- Mouse Move -----
    if (cmd.startsWith("MV:")) {
        QStringList nums = cmd.mid(3).split(",");
        if (nums.size() == 2) {
            CB_short dx = nums[0].toShort();
            CB_short dy = nums[1].toShort();
            if (dx != 0 || dy != 0) {
                push<CBEvt_Mv>(CB_EV_MOUSEMOVE, { dx, dy });
            }
        }
        return;
    }

    // ----- Vertical Scroll -----
    if (cmd.startsWith("WH:")) {
        short val = cmd.mid(3).toShort();
        push<CBEvt_Wh>(CB_EV_SCROLL, { 0, val });
        return;
    }

    // ----- Horizontal Scroll -----
    if (cmd.startsWith("HS:")) {
        short val = cmd.mid(3).toShort();
        push<CBEvt_Wh>(CB_EV_SCROLL, { val, 0 });
        return;
    }

    // ----- Text (UTF-8) -----
    if (cmd.startsWith("TXT:")) {
        QString txt = cmd.mid(4);
        if (!txt.isEmpty()) {
            QByteArray utf8 = txt.toUtf8();
            CBEvt_Txt payload;
            payload.str = new char[utf8.length() + 1];
            std::memcpy(payload.str, utf8.constData(), utf8.length() + 1);
            push<CBEvt_Txt>(CB_EV_TEXT, payload);
        }
        return;
    }

    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] Unknown: %s", qUtf8Printable(raw));
}

// ================================================================
// Gamepad button name -> Limelight button flag
// ================================================================
CB_Uint32 ClawBridgeServer::nameToButton(const QString& name)
{
    // Hex passthrough
    if (name.length() >= 3 && name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) {
        bool ok;
        uint val = name.mid(2).toUInt(&ok, 16);
        if (ok) return val;
    }

    struct Entry { const char* n; CB_Uint32 f; };
    static const Entry map[] = {
        { "A",           0x1000  },
        { "B",           0x2000  },
        { "X",           0x4000  },
        { "Y",           0x8000  },
        { "START",       0x0010  },
        { "OPTIONS",     0x0010  },
        { "PLAY",        0x0010  },
        { "BACK",        0x0020  },
        { "SELECT",      0x0020  },
        { "LS_CLICK",    0x0040  },
        { "L3",          0x0040  },
        { "RS_CLICK",    0x0080  },
        { "R3",          0x0080  },
        { "LB",          0x0100  },
        { "L1",          0x0100  },
        { "RB",          0x0200  },
        { "R1",          0x0200  },
        { "SPECIAL",     0x0400  },
        { "PS",          0x0400  },
        { "DPAD_UP",     0x0001  },
        { "UP",          0x0001  },
        { "D_UP",        0x0001  },
        { "DPAD_DOWN",   0x0002  },
        { "DOWN",        0x0002  },
        { "D_DOWN",      0x0002  },
        { "DPAD_LEFT",   0x0004  },
        { "LEFT",        0x0004  },
        { "D_LEFT",      0x0004  },
        { "DPAD_RIGHT",  0x0008  },
        { "RIGHT",       0x0008  },
        { "D_RIGHT",     0x0008  },
        { "PADDLE1",     0x010000 },
        { "PADDLE2",     0x020000 },
        { "PADDLE3",     0x040000 },
        { "PADDLE4",     0x080000 },
        { "TOUCHPAD",    0x100000 },
        { "MISC",        0x200000 },
        { "SHARE",       0x200000 },
    };

    QByteArray ba = name.toLatin1();
    for (auto& e : map) {
        if (ba == e.n) return e.f;
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] Unknown button: %s", qUtf8Printable(name));
    return 0;
}

// ================================================================
// Keyboard name -> Windows VK code
// ================================================================
CB_short ClawBridgeServer::nameToKeyCode(const QString& name)
{
    if (name.length() == 1 && name[0].isLetter())
        return 0x41 + (name[0].toUpper().unicode() - 'A');
    if (name.length() == 1 && name[0].isDigit())
        return 0x30 + name[0].unicode() - '0';
    if (name.length() >= 2 && name.startsWith("F")) {
        bool ok;
        int n = name.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 12) return 0x70 + n - 1;
        if (ok && n >= 13 && n <= 24) return 0x7C + n - 13;
    }

    struct Entry { const char* n; CB_short v; };
    static const Entry map[] = {
        { "ENTER",    0x0D }, { "RETURN",   0x0D },
        { "ESC",      0x1B }, { "ESCAPE",   0x1B },
        { "BACKSPACE",0x08 }, { "BKSP",     0x08 },
        { "TAB",      0x09 },
        { "SPACE",    0x20 },
        { "CAPSLOCK", 0x14 },
        { "PRINTSCREEN",0x2C},{ "PRTSCR",   0x2C },
        { "SCROLLLOCK",0x91 },
        { "PAUSE",    0x13 },
        { "INSERT",   0x2D }, { "INS",      0x2D },
        { "DELETE",   0x2E }, { "DEL",      0x2E },
        { "HOME",     0x24 },
        { "END",      0x23 },
        { "PAGEUP",   0x21 }, { "PGUP",     0x21 },
        { "PAGEDOWN", 0x22 }, { "PGDN",     0x22 },
        { "KB_UP",    0x26 },
        { "KB_DOWN",  0x28 },
        { "KB_LEFT",  0x25 },
        { "KB_RIGHT", 0x27 },
        { "LSHIFT",   0xA0 }, { "RSHIFT",   0xA1 },
        { "LCTRL",    0xA2 }, { "RCTRL",    0xA3 },
        { "LALT",     0xA4 }, { "RALT",     0xA5 },
        { "LWIN",     0x5B }, { "RWIN",     0x5C },
        { "GUI",      0x5B },
        { "MINUS",    0xBD },
        { "EQUALS",   0xBB },
        { "LBRACKET", 0xDB }, { "LBRA",     0xDB },
        { "RBRACKET", 0xDD }, { "RBRA",     0xDD },
        { "BACKSLASH",0xDC },
        { "SEMICOLON",0xBA },
        { "APOSTROPHE",0xDE },
        { "COMMA",    0xBC },
        { "PERIOD",   0xBE },
        { "SLASH",    0xBF },
        { "GRAVE",    0xC0 },
        { "NUMPAD0",  0x60 }, { "NP0",   0x60 },
        { "NUMPAD1",  0x61 }, { "NP1",   0x61 },
        { "NUMPAD2",  0x62 }, { "NP2",   0x62 },
        { "NUMPAD3",  0x63 }, { "NP3",   0x63 },
        { "NUMPAD4",  0x64 }, { "NP4",   0x64 },
        { "NUMPAD5",  0x65 }, { "NP5",   0x65 },
        { "NUMPAD6",  0x66 }, { "NP6",   0x66 },
        { "NUMPAD7",  0x67 }, { "NP7",   0x67 },
        { "NUMPAD8",  0x68 }, { "NP8",   0x68 },
        { "NUMPAD9",  0x69 }, { "NP9",   0x69 },
        { "NP_ENTER",    0x0D },
        { "NP_MULTIPLY", 0x6A },
        { "NP_PLUS",     0x6B },
        { "NP_MINUS",    0x6D },
        { "NP_PERIOD",   0x6E },
        { "NP_DIVIDE",   0x6F },
        { "NUMLOCK",  0x90 },
    };

    QByteArray ba = name.toLatin1();
    for (auto& e : map) {
        if (ba == e.n) return e.v;
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] Unknown key: %s", qUtf8Printable(name));
    return 0;
}

// ================================================================
// Mouse button name -> Limelight button number
// ================================================================
CB_Uint8 ClawBridgeServer::nameToMouseBtn(const QString& name)
{
    if (name == "LEFT"  || name == "BTN_LEFT")  return 1;
    if (name == "RIGHT" || name == "BTN_RIGHT") return 3;
    if (name == "MIDDLE" || name == "BTN_MIDDLE" || name == "WHEEL") return 2;
    if (name == "X1" || name == "BTN_X1" || name == "BACK")   return 4;
    if (name == "X2" || name == "BTN_X2" || name == "FORWARD")return 5;
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[ClawBridge] Unknown mouse: %s", qUtf8Printable(name));
    return 0;
}
