#ifndef MOTOGAMEPAGE_H
#define MOTOGAMEPAGE_H

#include <QWidget>
#include "walletmodel.h"

namespace Ui {
class MotogamePage;
}

class MotogamePage : public QWidget
{
    Q_OBJECT

    void timerEvent(QTimerEvent *event);

public:
    explicit MotogamePage(QWidget *parent = 0);
    ~MotogamePage();

    void setModel(WalletModel *pModel);

private slots:
    void on_playButton_clicked();
    void on_watchButton_clicked();
    void onLinkClicked(const QString & link);

private:
    Ui::MotogamePage *ui;
    WalletModel *m_pWalletModel;
};

#endif // MOTOGAMEPAGE_H
