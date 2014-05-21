#include "motogamepage.h"
#include "gamehelpdialog.h"
#include "ui_motogamepage.h"
#include "main.h"

extern void startMining(CWallet* pWallet);
extern void watchReplay(int nHeight);

MotogamePage::MotogamePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MotogamePage)
{
    ui->setupUi(this);
    ui->BlockIndex->setMinimum(1);

    connect(ui->labelHelp, SIGNAL(linkActivated(const QString &)), this, SLOT(onLinkClicked(const QString &)));

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
    startMining(m_pWalletModel->wallet);
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
        watchReplay(Value);
}
