#ifndef MAILDIALOG_H
#define MAILDIALOG_H

#include <QDialog>

namespace Ui {
class mailDialog;
}

class MailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MailDialog(QWidget *parent = nullptr);
    ~MailDialog();

    QString getEmail() const;
    QString getPassword() const;

private:
    Ui::mailDialog *ui;
};

#endif // MAILDIALOG_H
