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
    g_nProcessId=0;
    g_pDevice=nullptr;
}

void XDynStructsEngine::setStructsPath(QString sStructsPath, OPTIONS options)
{
    if(sStructsPath!=g_sStructsPath)
    {
        XProcess::SYSTEMINFO systemInfo=XProcess::getSystemInfo();
        // Load structs
        g_listDynStructs.clear();

        if(options.bSystem)
        {
            g_listDynStructs.append(loadFile(sStructsPath+QDir::separator()+systemInfo.sArch+QDir::separator()+QString("%1.json").arg(systemInfo.sBuild)));
        }

        if(options.bGeneral)
        {
            g_listDynStructs.append(loadFile(sStructsPath+QDir::separator()+systemInfo.sArch+QDir::separator()+QString("general.json")));
        }

        if(options.bCustom)
        {
            g_listDynStructs.append(loadFile(sStructsPath+QDir::separator()+systemInfo.sArch+QDir::separator()+QString("custom.json")));
        }
    }

    g_sStructsPath=sStructsPath;
}

void XDynStructsEngine::setProcessId(qint64 nProcessId)
{
    g_nProcessId=nProcessId;
}

void XDynStructsEngine::setDevice(QIODevice *pDevice)
{
    g_pDevice=pDevice;
}

qint64 XDynStructsEngine::getProcessId()
{
    return g_nProcessId;
}

QIODevice *XDynStructsEngine::getDevice()
{
    return g_pDevice;
}

XDynStructsEngine::INFO XDynStructsEngine::getInfo(qint64 nAddress, QString sName)
{
    INFO result={};

    if(sName!="")
    {
        int nNumberOfStructs=g_listDynStructs.count();

        DYNSTRUCT dynStruct={};

        for(int i=0;i<nNumberOfStructs;i++)
        {
            if(g_listDynStructs.at(i).sName==sName)
            {
                dynStruct=g_listDynStructs.at(i);

                break;
            }
        }

        if(dynStruct.sIUID!="")
        {
            result.bIsValid=true;

            int nNumberOfPositions=dynStruct.listPositions.count();

            for(int i=0;i<nNumberOfPositions;i++)
            {
                INFORECORD infoRecord={};

                DSPOSITION position=dynStruct.listPositions.at(i);

                infoRecord.nAddress=nAddress+position.nOffset;
                infoRecord.nOffset=position.nOffset;
                infoRecord.sType=position.sType;
                infoRecord.sName=position.sName;

                if(position.posType==POSTYPE_VARIABLE)
                {

                }

                result.listRecords.append(infoRecord);
            }
        }
    }
    else
    {
        if(g_nProcessId)
        {
        #ifdef Q_OS_WIN
            result.bIsValid=true;
            result.listRecords.append(getPEB(g_nProcessId));
            result.listRecords.append(getTEBs(g_nProcessId));
        #endif
        }
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

            QJsonArray jsonStructsArray=jsonObject.value("structs").toArray();

            int nNumberOfStructs=jsonStructsArray.count();

            for(int i=0;i<nNumberOfStructs;i++)
            {
                DYNSTRUCT record={};

                QJsonObject jsonStruct=jsonStructsArray.at(i).toObject();

                record.sIUID=XBinary::generateUUID(); // TODO Check
                record.sName=jsonStruct.value("name").toString();
                record.nSize=jsonStruct.value("size").toInt();
                record.sInfoFile=jsonStruct.value("infofile").toString();

                QJsonArray jsonPositionsArray=jsonStruct.value("positions").toArray();

                int nNumberOfPositions=jsonPositionsArray.count();

                for(int j=0;j<nNumberOfPositions;j++)
                {
                    DSPOSITION position={};

                    QJsonObject jsonPosition=jsonPositionsArray.at(j).toObject();

                    position.sName=jsonPosition.value("name").toString();
                    position.sType=jsonPosition.value("type").toString();
                    position.nOffset=jsonPosition.value("offset").toInt();
                    position.nSize=jsonPosition.value("size").toInt();

                    // TODO VT

                    record.listPositions.append(position);
                }

                listResult.append(record);
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

QList<XDynStructsEngine::DYNSTRUCT> *XDynStructsEngine::getStructs()
{
    return &g_listDynStructs;
}
#ifdef Q_OS_WIN
XDynStructsEngine::INFORECORD XDynStructsEngine::getPEB(qint64 nProcessId)
{
    INFORECORD result={};

    QString sValue=XBinary::valueToHexOS(XProcess::getPEBAddress(nProcessId));

    result.nAddress=-1;
    result.nOffset=-1;
    result.sType="struct _PEB *";
    result.sName="pPeb";
    result.sValue=sValue;
    result.sValueData=QString("%1&%2").arg(sValue,"struct _PEB");

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
        record.sType="struct _TEB *";
        record.sName="pTeb";
        record.sValue=sValue;
        record.sValueData=QString("%1&%2").arg(sValue,"struct _TEB");

        listResult.append(record);
    }

    return listResult;
}
#endif
