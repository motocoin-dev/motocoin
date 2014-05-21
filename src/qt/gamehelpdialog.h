#ifndef GAMEHELPDIALOG_H
#define GAMEHELPDIALOG_H

#include <QDialog>

namespace Ui {
class GameHelpDialog;
}

class GameHelpDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit GameHelpDialog(QWidget *parent = 0);
    ~GameHelpDialog();
    
private:
    Ui::GameHelpDialog *ui;
};

#endif // GAMEHELPDIALOG_H
