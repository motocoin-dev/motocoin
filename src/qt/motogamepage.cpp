#include "motogamepage.h"
#include "gamehelpdialog.h"
#include "ui_motogamepage.h"
#include "main.h"

extern void startMining(bool LowQ, bool OGL3, CWallet* pWallet);
extern void watchReplay(bool LowQ, bool OGL3, int nHeight);

MotogamePage::MotogamePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MotogamePage),
    m_GroupQ(this),
    m_GroupRender(this)
{
    ui->setupUi(this);
    ui->BlockIndex->setMinimum(1);

    connect(ui->labelHelp, SIGNAL(linkActivated(const QString &)), this, SLOT(onLinkClicked(const QString &)));

    m_GroupQ.addButton(ui->radioHighQ);
    m_GroupQ.addButton(ui->radioLowQ);
    m_GroupRender.addButton(ui->radioOGL3);
    m_GroupRender.addButton(ui->radioSoft);
    ui->radioHighQ->setChecked(true);
    ui->radioOGL3->setChecked(true);
#ifndef _WIN32
    ui->radioSoft->setDisabled(true);
#endif
    startTimer(1000);
}

MotogamePage::~MotogamePage()
{
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
    startMining(ui->radioLowQ->isChecked(), ui->radioOGL3->isChecked(), m_pWalletModel->wallet);
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

void MotogamePage::on_watchButton_clicked()
{
    int Value = ui->BlockIndex->value();
    if (0 < Value && Value <= nBestHeight)
        watchReplay(ui->radioLowQ->isChecked(), ui->radioOGL3->isChecked(), Value);
}
