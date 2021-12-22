/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     LiuMingHang <liuminghang@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "renamedialog.h"
#include "unionimage/baseutils.h"
#include "unionimage/imageutils.h"

#include "accessibility/ac-desktop-define.h"

#include <DMessageBox>
#include <DLabel>
#include <DFontSizeManager>

#include <QRegExp>
#include <QDebug>

DWIDGET_USE_NAMESPACE
typedef DLabel QLbtoDLabel;

#define FILENAMEMAXLENGTH 255

RenameDialog::RenameDialog(const QString &filename, QWidget *parent)
    : DDialog(parent)
    , m_filenamepath(filename)
{
    this->setIcon(QIcon::fromTheme("deepin-image-viewer"));
    DWidget *widet = new DWidget(this);
    addContent(widet);
    m_vlayout = new QVBoxLayout(widet);
    m_hlayout = new QHBoxLayout();
    m_edtlayout = new QHBoxLayout();
    m_lineedt = new DLineEdit(widet);
    QFrame *line = new QFrame(widet);
    QLbtoDLabel *labtitle = new QLbtoDLabel();
    okbtn = new DSuggestButton(tr("Confirm"), widet);
    cancelbtn = new DPushButton(tr("Cancel"), widet);
    m_labformat = new DLabel(widet);
    m_vlayout->setContentsMargins(2, 0, 2, 1);
//    okbtn->setText();
//    cancelbtn->setText();
    m_hlayout->addWidget(cancelbtn);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    m_hlayout->addWidget(line);
    m_hlayout->addWidget(okbtn);
    labtitle->setText(tr("Input a new name"));
    labtitle->setAlignment(Qt::AlignCenter);
    m_vlayout->addWidget(labtitle);
    m_vlayout->addStretch();
    m_edtlayout->addWidget(m_lineedt);
    m_labformat->setEnabled(false);
    m_edtlayout->addWidget(m_labformat);
    m_vlayout->addLayout(m_edtlayout);
    m_vlayout->addStretch();

    m_labTips = new DLabel();
    m_labTips->adjustSize();
    m_labTips->setWordWrap(true);
    m_labTips->setIndent(10);

    m_vlayout->addWidget(m_labTips);

    m_vlayout->addLayout(m_hlayout);
    widet->setLayout(m_vlayout);
//    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    InitDlg();
    m_lineedt->lineEdit()->setFocus();
    int Dirlen = /*m_DirPath.size() +*/ 1 + m_labformat->text().size();
    //正则表达式排除文管不支持的字符
    QRegExp rx("[^\\\\//:*?\"<>|]*");
    QRegExpValidator *pReg = new QRegExpValidator(rx, this);
    m_lineedt->lineEdit()->setValidator(pReg);
    connect(okbtn, &DSuggestButton::clicked, this, [ = ] {
        m_filename = m_lineedt->text() + m_labformat->text();
        m_filenamepath = m_DirPath + "/" + m_filename;
        accept();
    });
    connect(cancelbtn, &DPushButton::clicked, this, [ = ] {
        reject();
    });
    connect(m_lineedt, &DLineEdit::textChanged, this, [ = ](const QString & arg) {
        int len = arg.toLocal8Bit().length();
        //修复字符串长度超长会将
        if (len > 255 - Dirlen) return;
        QString fileabname = m_DirPath + "/" + arg + m_labformat->text();
        QFile file(fileabname);
        if (file.exists()) {
            okbtn->setEnabled(false);
            m_labTips->setVisible(true);
            setCurrentTip();
        } else if (arg.isEmpty()) {
            okbtn->setEnabled(false);
            m_labTips->setVisible(false);
            setFixedSize(380, 180);
        } else {
            okbtn->setEnabled(true);
            m_labTips->setVisible(false);
            setFixedSize(380, 180);
        }
    });
    connect(m_lineedt, &DLineEdit::textEdited, this, [ = ](const QString & arg) {
        qDebug() << "textEdited" << arg;
        int len = arg.toLocal8Bit().length();
        QString Interceptstr;
        if (len > 255 - Dirlen) {
            int num = 0;
            int i = 0;
            for (; i < arg.size(); i++) {
                if (arg.at(i) >= 0x4e00 && arg.at(i) <= 0x9fa5) {
                    num += 3;
                    if (num >= 255 - Dirlen - 1) break;
                } else if (num < 255 - Dirlen) {
                    num += 1;
                } else {
                    break;
                }
            }
            Interceptstr = arg.left(i);
            m_lineedt->lineEdit()->setText(Interceptstr);
        };
    });
    okbtn->setEnabled(false);
    setObjectName(RENAME_WIDGET);
    setAccessibleName(RENAME_WIDGET);
    m_lineedt->setObjectName(INPUT_EDIT);
    m_lineedt->setAccessibleName(INPUT_EDIT);
    okbtn->setObjectName(OK_BUTTON);
    okbtn->setAccessibleName(OK_BUTTON);
    cancelbtn->setObjectName(CANCEL_BUTTON);
    cancelbtn->setAccessibleName(CANCEL_BUTTON);
    widet->setObjectName(RENAME_CONTENT);
    widet->setObjectName(RENAME_CONTENT);

    m_tipString = tr("The file \"%1\" already exists, please use another name").arg(m_lineedt->text() + m_labformat->text());
    m_labTips->setText(m_tipString);
    setFixedSize(380, 180 + m_labTips->height() * 2);
}


//void RenameDialog::onThemeChanged(ViewerThemeManager::AppTheme theme)
//{
//    QPalette pe;
//    if (theme == ViewerThemeManager::Dark) {
//        pe.setColor(QPalette::WindowText, Qt::darkGray);
//    } else {
//        pe.setColor(QPalette::WindowText, Qt::lightGray);
//    }
//    m_labformat->setPalette(pe);
//}


QString RenameDialog::GetFilePath()
{
    return m_filenamepath;
}

QString RenameDialog::GetFileName()
{
    return m_filename;
}

void RenameDialog::InitDlg()
{
    QFileInfo fileinfo(m_filenamepath);
    m_DirPath = fileinfo.path();
    m_filename = fileinfo.fileName();
    QString format = fileinfo.suffix();
    QString basename;
    //basename会过滤掉.,那么1.....png就会出现basename为1,completeBaseName不会,修改bug66356
    m_basename = fileinfo.completeBaseName();
    m_lineedt->setText(m_basename);
    m_labformat->setText("." + format);
}

void RenameDialog::setCurrentTip()
{
    m_tipString = tr("The file \"%1\" already exists, please use another name").arg(m_lineedt->text() + m_labformat->text());
    QFont font;
    int currentSize = DFontSizeManager::instance()->fontPixelSize(font);
//    int widthString = Libutils::base::stringWidth(DFontSizeManager::instance()->get(DFontSizeManager::T8), m_tipString);
    int heightString = Libutils::base::stringHeight(DFontSizeManager::instance()->get(DFontSizeManager::T8), m_tipString) + 12;

    double lineCount = double (m_labTips->height()) / double(currentSize);

    if (lineCount >= 1.7) {
        heightString = heightString * 2;

        m_tipString = tr("The file \"%1\" already exists, please use another name").arg("");

        int width = Libutils::base::stringWidth(DFontSizeManager::instance()->get(DFontSizeManager::T8), m_tipString);
        int widthC;
        if (currentSize > 15) {
            widthC = m_labTips->width() * 2 - width  - 200;
        } else if (currentSize > 11) {
            widthC = m_labTips->width() * 2 - width  - 250;
        } else {
            widthC = m_labTips->width() * 2 - width  - 270;
        }

        QString a = geteElidedText(DFontSizeManager::instance()->get(DFontSizeManager::T8),
                                   m_lineedt->text() + m_labformat->text(), widthC);
        m_tipString = tr("The file \"%1\" already exists, please use another name").arg(a);
    }
    m_labTips->setText(m_tipString);
    if (m_labTips->isVisible()) {
        setFixedSize(380, 180 + heightString);
    } else {
        setFixedSize(380, 180);
    }

}

void RenameDialog::paintEvent(QPaintEvent *event)
{
    QFont font;
    int currentSize = DFontSizeManager::instance()->fontPixelSize(font);

    m_currentFontSize = currentSize;
    setCurrentTip();

    QWidget::paintEvent(event);
}


QString RenameDialog::geteElidedText(QFont font, QString str, int MaxWidth)
{
    QFontMetrics fontWidth(font);
    int width = fontWidth.horizontalAdvance(str);
    if (width >= MaxWidth) {
        str = fontWidth.elidedText(str, Qt::ElideMiddle, MaxWidth);
    }
    return str;
}
