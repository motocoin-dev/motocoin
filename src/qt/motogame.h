#ifndef MOTOGAME_H
#define MOTOGAME_H

#include <QObject>
#include <QProcess>
#include <list>
#include <memory>
#include "main.h"
#include "wallet.h"

class Motogame : public QObject
{
    Q_OBJECT

    QProcess m_Motogame;

    CWallet* m_pWallet;
    CReserveKey m_ReserveKey;
    CBlockIndex* m_pPrevBest;

    std::list<std::unique_ptr<CBlockTemplate>> m_Templates;

    void updateBlock();

    std::list<std::unique_ptr<CBlockTemplate>>::iterator findBlock(const MotoWork& Work);

    void timerEvent(QTimerEvent *event);

public:
    explicit Motogame(bool LowQ, bool OGL3, CWallet* pWallet, QObject *parent = 0);
    ~Motogame();

signals:
    
public slots:
    void onReadAvailable();
    void onGameFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onError(QProcess::ProcessError error);
};

class SuicideProcess : public QProcess
{
    Q_OBJECT

public:
    SuicideProcess()
    {
        connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinish(int, QProcess::ExitStatus)));
    }

public slots:
    void onFinish(int exitCode, QProcess::ExitStatus exitStatus) { deleteLater(); }
};

#endif // MOTOGAME_H
