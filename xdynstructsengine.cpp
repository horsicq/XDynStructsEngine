// copyright (c) 2021 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "xdynstructsengine.h"

XDynStructsEngine::XDynStructsEngine(QObject *pParent) : QObject(pParent)
{

}

void XDynStructsEngine::setData(OPTIONS options)
{
    if(g_options.sStructsPath!=options.sStructsPath)
    {
        XProcess::SYSTEMINFO systemInfo=XProcess::getSystemInfo();
        // Load structs
        g_listDynStructs.clear();

        if(options.bSystem)
        {
            g_listDynStructs.append(loadFile(options.sStructsPath+QDir::separator()+systemInfo.sArch+QDir::separator()+QString("%1.json").arg(systemInfo.sBuild)));
        }

        if(options.bGeneral)
        {
            g_listDynStructs.append(loadFile(options.sStructsPath+QDir::separator()+systemInfo.sArch+QDir::separator()+QString("general.json")));
        }

        if(options.bCustom)
        {
            g_listDynStructs.append(loadFile(options.sStructsPath+QDir::separator()+systemInfo.sArch+QDir::separator()+QString("custom.json")));
        }
    }

    g_options=options;
}

XDynStructsEngine::OPTIONS XDynStructsEngine::getOptions()
{
    return g_options;
}

XDynStructsEngine::INFO XDynStructsEngine::getInfo(QIODevice *pDevice, qint64 nOffset, QString sStruct)
{
    INFO result={};

    if(sStruct!="")
    {
        // TODO
    }

    return result;
}

XDynStructsEngine::INFO XDynStructsEngine::getInfo(qint64 nProcessId, qint64 nAddress, QString sStruct)
{
    INFO result={};

    if(sStruct!="")
    {
        // TODO
    }
    else
    {
    #ifdef Q_OS_WIN
        result.listRecords.append(getPEB(nProcessId));
        result.listRecords.append(getTEBs(nProcessId));
    #endif
    }

    return result;
}

QList<XDynStructsEngine::DYNSTRUCT> XDynStructsEngine::loadFile(QString sFileName)
{
    QList<DYNSTRUCT> listResult;

    QFile file;
    file.setFileName(sFileName);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QString sJsonData=file.readAll();

        QJsonDocument jsonDocument=QJsonDocument::fromJson(sJsonData.toUtf8());

        if(jsonDocument.isObject())
        {
            QJsonObject jsonObject=jsonDocument.object();
            // TODO get name and info

            QJsonArray jsonArray=jsonObject.value("records").toArray();

            int nNumberOfRecords=jsonArray.count();

            for(int i=0;i<nNumberOfRecords;i++)
            {
                jsonArray.at(i).toObject();
            }
        }

        file.close();
    }
    else
    {
    #ifdef QT_DEBUG
        qDebug("Cannot load file: %s",sFileName.toLatin1().data());
    #endif
    }

    return listResult;
}
#ifdef Q_OS_WIN
XDynStructsEngine::INFORECORD XDynStructsEngine::getPEB(qint64 nProcessId)
{
    INFORECORD result={};

    QString sValue=XBinary::valueToHexOS(XProcess::getPEBAddress(nProcessId));

    result.nAddress=-1;
    result.nOffset=-1;
    result.sType="struct PEB *";
    result.sName="pPeb";
    result.sValue=sValue;
    result.sValueData=QString("%1&%2").arg(sValue,"struct PEB");

    return result;
}
#endif
#ifdef Q_OS_WIN
QList<XDynStructsEngine::INFORECORD> XDynStructsEngine::getTEBs(qint64 nProcessId)
{
    QList<INFORECORD> listResult;

    QList<qint64> listTEBAddresses=XProcess::getTEBAddresses(nProcessId);

    int nNumberOfThreads=listTEBAddresses.count();

    for(int i=0;i<nNumberOfThreads;i++)
    {
        INFORECORD record={};

        QString sValue=XBinary::valueToHexOS(listTEBAddresses.at(i));

        record.nAddress=-1;
        record.nOffset=-1;
        record.sType="struct TEB *";
        record.sName="pTeb";
        record.sValue=sValue;
        record.sValueData=QString("%1&%2").arg(sValue,"struct TEB");

        listResult.append(record);
    }

    return listResult;
}
#endif
