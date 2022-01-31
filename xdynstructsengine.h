/* Copyright (c) 2021-2022 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XDYNSTRUCTSENGINE_H
#define XDYNSTRUCTSENGINE_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "xbinary.h"
#include "xhtml.h"
#include "xprocess.h"
#include "xprocessdevice.h"
#include "xoptions.h"

class XDynStructsEngine : public QObject
{
    Q_OBJECT

public:
//    struct OPTIONS
//    {
//        bool bSystem;
//        bool bGeneral;
//        bool bCustom;
//    };

    enum RECORDTYPE
    {
        RECORDTYPE_AUTO=0,
        RECORDTYPE_NONE,
        RECORDTYPE_VARIABLE,
        RECORDTYPE_POINTER,
        RECORDTYPE_ARRAY
    };

    struct DSPOSITION
    {
        qint64 nOffset;
        qint64 nSize;
        qint32 nBitOffset;
        qint32 nBitSize;
        QString sName;
        QString sType;
        RECORDTYPE recordType;
        qint32 nArrayCount;
    };

    struct DYNSTRUCT
    {
        QString sIUID;
        QString sName;
        QString sInfoFilePrefix;
        QString sInfoFile;
        qint64 nSize;
        RECORDTYPE recordType;
        QList<DSPOSITION> listPositions;
    };

    struct INFORECORD
    {
        qint64 nAddress;    // Do not show if -1
        qint64 nOffset;     // Do not show if -1; Offset from begin
        QString sType;
        QString sName;
        QString sValue;
        QString sValueData; // If not empty  -> active link
        QString sComment;
    };

    struct INFO
    {
        bool bIsValid;
        QList<INFORECORD> listRecords;
    };

    enum STRUCTTYPE
    {
        STRUCTTYPE_VARIABLE=0,
        STRUCTTYPE_POINTER
    };

    explicit XDynStructsEngine(QObject *pParent=nullptr);

    void adjust();
    void setProcessId(qint64 nProcessId);
    void setDevice(QIODevice *pDevice);
    void setOptions(XOptions *pXOptions);
    qint64 getProcessId();
    QIODevice *getDevice();
    INFO getInfo(qint64 nAddress,QString sStructName,STRUCTTYPE structType,qint32 nCount);
    QList<DYNSTRUCT> loadFile(QString sFileName);
    QList<DYNSTRUCT> *getStructs();
    QString getValue(void *pProcess,XBinary *pBinary,qint64 nAddress,qint64 nSize,RECORDTYPE recordType,qint32 nBitOffset,qint32 nBitSize);
    QString getValueData(qint64 nAddress,RECORDTYPE recordType,QString sType,QString sValue,qint32 nArrayCount);
    QString getComment(void *pProcess,XBinary *pBinary,qint64 nAddress,QString sStructName,QString sType,QString sName);
    DYNSTRUCT getDynStructByName(QString sName);
    static RECORDTYPE getRecordType(QString sType);
    static QString createListEntryLinks(void *pProcess,XBinary *pBinary,qint64 nAddress,QString sStructName,qint64 nDeltaOffset);

private:
#ifdef Q_OS_WIN
    INFORECORD getPEB(qint64 nProcessId);
    QList<INFORECORD> getTEBs(qint64 nProcessId);
#endif

signals:
    void errorMessage(QString sErrorMessage);

private:
    QList<DYNSTRUCT> g_listDynStructs;
//    OPTIONS g_options;
    XOptions *g_pXOptions;
    QIODevice *g_pDevice;
    qint64 g_nProcessId;
    QString g_sStructsPath;
};

#endif // XDYNSTRUCTSENGINE_H
