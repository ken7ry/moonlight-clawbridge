#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QList>
#include <QString>
#include <QObject>
#include "SDL_compat.h"

// ================================================================
// ClawBridge — TCP Remote Controller for Moonlight-Qt
//
// Listens on 0.0.0.0:9999, receives text commands,
// injects them as SDL user-events into the main event loop.
// ALL LiSend* calls happen on the main thread (thread-safe).
// ================================================================

// Custom SDL event types (hardcoded, shared with session.cpp)
#define CB_EV_GAMEPAD    (SDL_USEREVENT + 101)
#define CB_EV_KEYBOARD   (SDL_USEREVENT + 102)
#define CB_EV_MOUSEBTN   (SDL_USEREVENT + 103)
#define CB_EV_SCROLL     (SDL_USEREVENT + 104)
#define CB_EV_MOUSEMOVE  (SDL_USEREVENT + 105)
#define CB_EV_TEXT       (SDL_USEREVENT + 106)

typedef unsigned int  CB_Uint32;
typedef unsigned char CB_Uint8;
typedef short         CB_short;

class ClawBridgeServer : public QObject
{
    Q_OBJECT

public:
    explicit ClawBridgeServer(QObject* parent = nullptr);
    ~ClawBridgeServer();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnect();

private:
    void     parseLine(const QString& line);
    CB_Uint32 nameToButton(const QString& name);
    CB_short  nameToKeyCode(const QString& name);
    CB_Uint8  nameToMouseBtn(const QString& name);

    QTcpServer*          m_server  = nullptr;
    QList<QTcpSocket*>   m_clients;
};
