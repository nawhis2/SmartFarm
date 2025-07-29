#include "maildialog.h"
#include "ui_maildialog.h"

MailDialog::MailDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::mailDialog)
{
    ui->setupUi(this);
    setWindowTitle("메일 설정");
    setModal(true);
}

MailDialog::~MailDialog()
{
    delete ui;
}

QString MailDialog::getEmail() const
{
    return ui->emailLineEdit->text();
}

QString MailDialog::getPassword() const
{
    return ui->passwordLineEdit->text();
}
