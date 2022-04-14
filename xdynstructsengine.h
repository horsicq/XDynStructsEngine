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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "xbinary.h"
#include "xhtml.h"
#include "xoptions.h"
#include "xprocess.h"
#ifdef USE_XWINIODRIVER
#include "xwiniodriver.h"
#endif

class XDynStructsEngine : public QObject
{
    Q_OBJECT

    // TODO user/Kernel
    // TODO def XPROCESS !!!
    // TODO create for HEX
public:

    enum IOMODE
    {
        IOMODE_UNKNOWN=0,
        IOMODE_DEVICE,
        IOMODE_PROCESS_USER,
    #ifdef USE_XWINIODRIVER
        IOMODE_PROCESS_KERNEL
    #endif
    };

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
        quint64 nAddress;    // Do not show if -1
        quint64 nOffset;     // Do not show if -1; Offset from begin
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
    ~XDynStructsEngine();

    void adjust();
    void setProcessId(qint64 nProcessId,IOMODE ioMode);
    void setDevice(QIODevice *pDevice);
    void setOptions(XOptions *pXOptions);
    IOMODE getIOMode();
    qint64 getProcessId();
    QIODevice *getDevice();
    INFO getInfo(quint64 nAddress,QString sStructName,STRUCTTYPE structType,qint32 nCount);
    QList<DYNSTRUCT> loadFile(QString sFileName);
    QList<DYNSTRUCT> *getStructs();
    QString getValue(quint64 nAddress,quint64 nSize,RECORDTYPE recordType,qint32 nBitOffset,qint32 nBitSize);
    QString getValueData(quint64 nAddress,RECORDTYPE recordType,QString sType,QString sValue,qint32 nArrayCount);
    QString getComment(quint64 nAddress,QString sStructName,QString sType,QString sName);
    DYNSTRUCT getDynStructByName(QString sName);
    static RECORDTYPE getRecordType(QString sType);
    QString createListEntryLinks(quint64 nAddress,QString sStructName,qint64 nDeltaOffset);

    XIODevice *createIODevice(quint64 nAddress,quint64 nSize);

private:
#ifdef Q_OS_WIN
    INFORECORD getPEB(qint64 nProcessId);
    QList<INFORECORD> getTEBs(qint64 nProcessId);
    INFORECORD getEPROCESS(qint64 nProcessId);
    QList<INFORECORD> getKPCRs(qint64 nProcessId);
#endif

signals:
    void errorMessage(QString sErrorMessage);

private:
    QList<DYNSTRUCT> g_listDynStructs;
//    OPTIONS g_options;
    XOptions *g_pXOptions;
    QIODevice *g_pDevice;
    qint64 g_nProcessId;
    void *g_hProcess;
    void *g_hDriver;
    QString g_sStructsPath;
    XBinary *g_pBinary;
    IOMODE g_ioMode;
};

#endif // XDYNSTRUCTSENGINE_H
