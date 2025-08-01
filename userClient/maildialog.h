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

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void okButtonClicked();

private:
    Ui::mailDialog *ui;
    QString getCredential() const;
    void chkNowUserMail();
};

#endif // MAILDIALOG_H
