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
        // TODO QIODevice
        void *pHandle=nullptr;
        XBinary *pBinary=nullptr;

        if(g_nProcessId)
        {
            pHandle=XProcess::openProcess(g_nProcessId);
        }
        else if(g_pDevice)
        {
            pBinary=new XBinary(g_pDevice);
        }

        DYNSTRUCT dynStruct=getDynStructByName(sName);

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

                infoRecord.sValue=getValue(pHandle,pBinary,nAddress,position);

                result.listRecords.append(infoRecord);
            }
        }

        if(pHandle)
        {
            XProcess::closeProcess(pHandle);
        }
        else if(g_pDevice)
        {
            delete pBinary;
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
            QFileInfo fileInfo(sFileName);

            QJsonObject jsonObject=jsonDocument.object();

            QString sGlobalName=jsonObject.value("name").toString();
            QString sFilePrefix=fileInfo.absolutePath()+QDir::separator()+sGlobalName+QDir::separator();

            QJsonArray jsonStructsArray=jsonObject.value("structs").toArray();

            int nNumberOfStructs=jsonStructsArray.count();

            for(int i=0;i<nNumberOfStructs;i++)
            {
                DYNSTRUCT record={};

                QJsonObject jsonStruct=jsonStructsArray.at(i).toObject();

                record.sIUID=XBinary::generateUUID(); // TODO Check
                record.sName=jsonStruct.value("name").toString();
                record.nSize=jsonStruct.value("size").toInt();

                QString sInfoFile=jsonStruct.value("infofile").toString();

                if(sInfoFile!="")
                {
                    record.sInfoFile=sFilePrefix+sInfoFile;
                }

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
                    position.nBitOffset=jsonPosition.value("bitoffset").toInt();
                    position.nBitSize=jsonPosition.value("bitsize").toInt();

                    if(position.sName.contains("["))
                    {
                        position.posType=POSTYPE_ARRAY;
                    }
                    else if(position.sType.contains("*"))
                    {
                        position.posType=POSTYPE_POINTER;
                    }
                    else if((position.sType=="unsigned char")||
                            (position.sType=="unsigned short")||
                            (position.sType=="unsigned long")||
                            (position.sType=="unsigned long long"))
                    {
                        position.posType=POSTYPE_VARIABLE;
                    }

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
        emit errorMessage(QString("%1: %2").arg(tr("Cannot load file"),sFileName));
    }

    return listResult;
}

QList<XDynStructsEngine::DYNSTRUCT> *XDynStructsEngine::getStructs()
{
    return &g_listDynStructs;
}

QString XDynStructsEngine::getValue(void *pProcess, XBinary *pBinary, qint64 nAddress, DSPOSITION position)
{
    // TODO Endian
    QString sResult;

    if((position.posType==POSTYPE_VARIABLE)||(position.posType==POSTYPE_POINTER))
    {
        if(position.nSize==1)
        {
            quint8 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint8(pProcess,nAddress+position.nOffset);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint8(nAddress+position.nOffset);
            }

            sResult="0x"+XBinary::valueToHex(nValue);
        }
        else if(position.nSize==2)
        {
            quint16 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint16(pProcess,nAddress+position.nOffset);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint16(nAddress+position.nOffset);
            }

            sResult="0x"+XBinary::valueToHex(nValue);
        }
        else if(position.nSize==4)
        {
            quint32 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint32(pProcess,nAddress+position.nOffset);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint32(nAddress+position.nOffset);
            }

            sResult="0x"+XBinary::valueToHex(nValue);
        }
        else if(position.nSize==8)
        {
            quint64 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint64(pProcess,nAddress+position.nOffset);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint64(nAddress+position.nOffset);
            }

            sResult="0x"+XBinary::valueToHex(nValue);
        }
    }
    else
    {
        sResult="...";
    }

    return sResult;
}

XDynStructsEngine::DYNSTRUCT XDynStructsEngine::getDynStructByName(QString sName)
{
    DYNSTRUCT result={};

    if(sName!="")
    {
        int nNumberOfStructs=g_listDynStructs.count();

        for(int i=0;i<nNumberOfStructs;i++)
        {
            if(g_listDynStructs.at(i).sName==sName)
            {
                result=g_listDynStructs.at(i);

                break;
            }
        }
    }

    return result;
}
#ifdef Q_OS_WIN
XDynStructsEngine::INFORECORD XDynStructsEngine::getPEB(qint64 nProcessId)
{
    INFORECORD result={};

    QString sValue="0x"+XBinary::valueToHexOS(XProcess::getPEBAddress(nProcessId));

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

        QString sValue="0x"+XBinary::valueToHexOS(listTEBAddresses.at(i));

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
