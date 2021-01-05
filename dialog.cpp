#include "dialog.h"
#include "ui_dialog.h"
#include <QFileDialog>
#include <QProcess>
#include <QtXml>

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    com=new Compression();
    connect(com,&Compression::error,this,&Dialog::clear);
    connect(com,SIGNAL(mysignal(double)),this,SLOT(myslot(double)));
    ui->progressBar->setMaximum(100);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setValue(0);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::clear()
{
    ui->txtLogs->append(tr("压缩/解压缩过程发生错误"));
}

void Dialog::myslot(double per)
{
    if(per>ui->progressBar->value())
    {
        ui->progressBar->setValue(per);
    }
}

void Dialog::zip(QString fn,QString fnz)
{
    ui->txtLogs->append(tr("开始压缩文件：") + fn);
    com->zip(fn, fnz);
    ui->progressBar->setValue(0);
    ui->txtLogs->append(tr("压缩文件完成：") + fnz);
}

void Dialog::unzip(QString fnz, QString fn)
{
    ui->txtLogs->append(tr("开始解压缩文件：") + fnz);
    com->unzip(fnz, fn);
    ui->progressBar->setValue(0);
    ui->txtLogs->append(tr("解压缩文件完成：") + fn);
}

void AddFile(QString fn, QDomDocument doc, QDomElement parent)
{
    QFileInfo fi(fn);
    QFile file(fn);
    file.open(QIODevice::ReadOnly);
    QByteArray bs = file.readAll();
    file.close();
    QString content = bs.toBase64();
    QDomElement ele = doc.createElement("file");
    ele.setAttribute("src", fi.fileName());
    QDomText txt = doc.createTextNode(content);
    ele.appendChild(txt);
    parent.appendChild(ele);
}

void AddPath(QString path, QDomDocument doc, QDomElement parent)
{
    QDir dir(path);
    QDomElement ele = doc.createElement("path");
    ele.setAttribute("src", dir.dirName());
    parent.appendChild(ele);
    QFileInfoList list = dir.entryInfoList();
    for(int i=0; i<list.count(); i++)
    {
        QFileInfo fi = list.at(i);
        if(fi.fileName() == "." | fi.fileName() == "..")
        {
            continue;
        }
        if(fi.isFile())
        {
            QString fn = fi.filePath();
            AddFile(fn, doc, ele);
        }
        else
        {
            QString fn = fi.path();
            AddPath(fn, doc, ele);
        }
    }
}

void Dialog::pack(QString path, QString fnz)
{
    QString fn="./zip.xml";
    qDebug() << "PACK001-" << fn;
    if(QFile::exists(fn)) //如果文件已存在，进行删除
    {
        QFile::remove(fn);
    }

    QFile file(fn);
    if(!file.open(QIODevice::ReadWrite))
    {
        return; //新建文件打开失败
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8"); //使用utf-8格式

    QDomDocument doc;
    QDomProcessingInstruction xmlHeader = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
    doc.appendChild(xmlHeader);  // 开始文档（XML 声明）

    //添加根节点
    QDomElement root = doc.createElement("huffzip");
    doc.appendChild(root);

    QFileInfo fi(path);
    qDebug() << "PACK002-fi.isFile=" << fi.isFile();
    if(fi.isFile())
    {
        AddFile(fi.fileName(), doc, root);
    }
    else
    {
        AddPath(path, doc, root);
    }
    doc.save(stream, 4, QDomNode::EncodingFromTextStream);
    file.close();
    zip(fn, fnz);
}

void ExtFile(QString fn, QDomElement node)
{
    QString content = node.text();
    QByteArray bs = QByteArray::fromBase64(content.toLatin1());
    QFile file(fn);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(bs);
    file.close();
}

void ExtPath(QString path, QDomElement node)
{
    QDir dir(path);
    if(!dir.exists())
    {
        dir.mkdir(path);
    }

    QDomNodeList list = node.childNodes();
    for(int i=0; i<list.count(); i++)
    {
        QDomElement nd = list.at(i).toElement();
        QString fn = path;
        fn.append("/").append(nd.attribute("src"));
        if(nd.nodeName().startsWith("file"))
        {
            ExtFile(fn, nd);
        }
        else
        {
            ExtPath(fn, nd);
        }
    }
}

void Dialog::unpack(QString fnz, QString path)
{
    QString fn="./zip.xml";
    if(QFile::exists(fn)) //如果文件已存在，进行删除
    {
        QFile::remove(fn);
    }

    unzip(fnz, fn);

    QFile file(fn);
    if(!file.open(QIODevice::ReadOnly))
    {
        return; //新建文件打开失败
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8"); //使用utf-8格式

    QDomDocument doc;
    doc.setContent(&file);
    file.close();

    QDomElement root = doc.documentElement();
    QDomNodeList list = root.childNodes();
    for(int i=0; i<list.count(); i++)
    {
        QDomElement node = list.at(i).toElement();
        QString fn = path;
        fn.append("/").append(node.attribute("src"));
        qDebug()<< "UnPack001=" << fn;
        qDebug()<< "UnPack002=" << node.nodeName();
        if(node.nodeName().startsWith("file"))
        {
            ExtFile(fn, node);
        }
        else
        {
            ExtPath(fn, node);
        }
    }
}

void Dialog::on_btnFile_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("打开文件"),".", tr("所有文件(*.*)"));
    ui->edPathName->setText(file);
    ui->txtLogs->append(tr("选择文件：") + file);
}

void Dialog::on_btnBackup_clicked()
{
    QString path = ui->edPathName->text();
    QString out = ui->edOutName->text();
    pack(path, out);
}

void Dialog::on_btnRestore_clicked()
{
    QString path = ui->edPathName->text();
    QString out = ui->edOutName->text();
    unpack(path, out);
}

void Dialog::on_edPathName_textChanged(const QString &arg1)
{
    QString path = ui->edPathName->text();
    QFileInfo fi(path);
    QString out = fi.path();
    if(path.endsWith(".HuffZip", Qt::CaseSensitivity::CaseSensitive))
    {
        //out = path.replace(QRegExp(".HuffZip$"), "");
        ui->btnBackup->setEnabled(false);
        ui->btnRestore->setEnabled(true);
    }
    else
    {
        out = path.append(".HuffZip");
        ui->btnBackup->setEnabled(true);
        ui->btnRestore->setEnabled(false);
    }
    ui->edOutName->setText(out);
}

void Dialog::on_btnPath_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择目录", "./", QFileDialog::ShowDirsOnly);
    ui->edPathName->setText(path);
    ui->txtLogs->append(tr("选择目录：") + path);
}
