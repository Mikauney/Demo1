#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <compression.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();
    void clear();
    void zip(QString fn, QString fnz);
    void unzip(QString fnz, QString fn);
    void pack(QString path, QString fnz);
    void unpack(QString fnz, QString path);

private slots:
    void on_btnFile_clicked();

    void on_btnBackup_clicked();

    void myslot(double per);

    void on_btnRestore_clicked();

    void on_edPathName_textChanged(const QString &arg1);

    void on_btnPath_clicked();

private:
    Ui::Dialog *ui;

    Compression* com;
};
#endif // DIALOG_H
