#include <QSettings>
#include <QFile>

#include "motogamepage.h"
#include "gamehelpdialog.h"
#include "ui_motogamepage.h"
#include "main.h"

extern void startMining(bool LowQ, bool OGL3, bool Fullscreen, CWallet* pWallet);
extern void watchReplay(bool LowQ, bool OGL3, bool Fullscreen, int nHeight);

MotogamePage::MotogamePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MotogamePage)
{
    ui->setupUi(this);
    ui->BlockIndex->setMinimum(1);

    connect(ui->labelHelp, SIGNAL(linkActivated(const QString &)), this, SLOT(onLinkClicked(const QString &)));

    QSettings settings;
    ui->checkHighQ->setChecked(settings.value("GameHighQ", true).toBool());
    ui->checkFullscreen->setChecked(settings.value("GameFullsceen", false).toBool());

#ifdef _WIN32
    if(settings.value("GameOGL3", true).toBool())
        ui->radioOGL3->setChecked(true);
    else
        ui->radioSoft->setChecked(true);
#else
    ui->radioOGL3->setChecked(true);
    ui->radioSoft->setDisabled(true);
#endif

    startTimer(1000);
}

MotogamePage::~MotogamePage()
{
    QSettings settings;
    settings.setValue("GameHighQ", ui->checkHighQ->isChecked());
    settings.setValue("GameFullsceen", ui->checkFullscreen->isChecked());
    settings.setValue("GameOGL3", ui->radioOGL3->isChecked());

    delete ui;
}

void MotogamePage::setModel(WalletModel *pModel)
{
    m_pWalletModel = pModel;
}

void MotogamePage::onLinkClicked(const QString & link)
{
    if (link == "mine-help")
        GameHelpDialog(this).exec();
}

void MotogamePage::on_playButton_clicked()
{
    startMining(!ui->checkHighQ->isChecked(), ui->radioOGL3->isChecked(), ui->checkFullscreen->isChecked(), m_pWalletModel->wallet);
}

void MotogamePage::timerEvent(QTimerEvent *event)
{
    if (nBestHeight == 0)
    {
        ui->BlockIndex->setMaximum(1);
        ui->labelBlockCount->setText(tr("No blocks available"));
    }
    else
    {
        ui->BlockIndex->setMaximum(nBestHeight);
        ui->labelBlockCount->setText(tr("Enter any number from 1 to %1").arg(nBestHeight));
    }
}
/*
void dumpNonce()
{
    QFile File("nonces.txt");
    File.open(QIODevice::WriteOnly);

    for (CBlockIndex* pIndex = pindexGenesisBlock; pIndex; pIndex = pIndex->pnext)
    {
        QString N = QString("%1,").arg(pIndex->Nonce.Nonce);
        File.write(N.toUtf8());
    }
}*/

void MotogamePage::on_watchButton_clicked()
{
   /* dumpNonce();
    return;*/
    int Value = ui->BlockIndex->value();
    if (0 < Value && Value <= nBestHeight)
        watchReplay(!ui->checkHighQ->isChecked(), ui->radioOGL3->isChecked(), ui->checkFullscreen->isChecked(), Value);
}
