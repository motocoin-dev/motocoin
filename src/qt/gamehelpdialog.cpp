#include "gamehelpdialog.h"
#include "ui_gamehelpdialog.h"

GameHelpDialog::GameHelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GameHelpDialog)
{
    ui->setupUi(this);
}

GameHelpDialog::~GameHelpDialog()
{
    delete ui;
}
