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
#include "xdynstructsengine.h"

XDynStructsEngine::XDynStructsEngine(QObject *pParent) : QObject(pParent)
{
    g_nProcessId=0;
    g_pDevice=nullptr;
}

void XDynStructsEngine::adjust()
{
    QString sStructsPath=XBinary::convertPathName(g_pXOptions->getStructsPath());

    if(sStructsPath!=g_sStructsPath)
    {
        XBinary::OSINFO osInfo=XProcess::getOsInfo();

    #ifdef QT_DEBUG
        qDebug("Build: %s",osInfo.sBuild.toLatin1().data());
    #endif

        // Load structs
        g_listDynStructs.clear();

//        if(options.bSystem)
        {
            QString sFileName=sStructsPath+QDir::separator()+osInfo.sArch+QDir::separator()+QString("%1.json").arg(osInfo.sBuild);

            if(!XBinary::isFileExists(sFileName))
            {
                if(osInfo.sBuild.contains("10.0."))
                {
                    // TODO 10.0.19041
                    sFileName=sStructsPath+QDir::separator()+osInfo.sArch+QDir::separator()+QString("%1.json").arg("10.0.17134");
                }
            }

            g_listDynStructs.append(loadFile(sFileName));
        }

//        if(options.bGeneral)
        {
            g_listDynStructs.append(loadFile(sStructsPath+QDir::separator()+osInfo.sArch+QDir::separator()+QString("general.json")));
        }

//        if(options.bCustom)
        {
            g_listDynStructs.append(loadFile(sStructsPath+QDir::separator()+osInfo.sArch+QDir::separator()+QString("custom.json")));
        }
    }

    g_sStructsPath=sStructsPath;
}

void XDynStructsEngine::setProcessId(qint64 nProcessId)
{
    g_nProcessId=nProcessId;
    adjust();
}

void XDynStructsEngine::setDevice(QIODevice *pDevice)
{
    g_pDevice=pDevice;
    adjust();
}

void XDynStructsEngine::setOptions(XOptions *pXOptions)
{
    g_pXOptions=pXOptions;
}

qint64 XDynStructsEngine::getProcessId()
{
    return g_nProcessId;
}

QIODevice *XDynStructsEngine::getDevice()
{
    return g_pDevice;
}

XDynStructsEngine::INFO XDynStructsEngine::getInfo(qint64 nAddress,QString sStructName,STRUCTTYPE structType,qint32 nCount)
{
    INFO result={};

    if(sStructName!="")
    {
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

        if(structType==STRUCTTYPE_VARIABLE)
        {
            DYNSTRUCT dynStruct=getDynStructByName(sStructName);

            if(dynStruct.sIUID!="")
            {
                result.bIsValid=true;

                if(nCount<=1)
                {
                    qint32 nNumberOfPositions=dynStruct.listPositions.count();

                    for(qint32 i=0;i<nNumberOfPositions;i++)
                    {
                        DSPOSITION position=dynStruct.listPositions.at(i);

                        INFORECORD infoRecord={};

                        infoRecord.nOffset=position.nOffset;
                        infoRecord.nAddress=nAddress+infoRecord.nOffset;
                        infoRecord.sType=position.sType;
                        infoRecord.sName=position.sName;

                        infoRecord.sValue=getValue(pHandle,pBinary,infoRecord.nAddress,position.nSize,position.recordType,position.nBitOffset,position.nBitSize);
                        infoRecord.sValueData=getValueData(infoRecord.nAddress,position.recordType,position.sType,infoRecord.sValue,position.nArrayCount);
                        infoRecord.sComment=getComment(pHandle,pBinary,infoRecord.nAddress,sStructName,infoRecord.sType,infoRecord.sName);

                        result.listRecords.append(infoRecord);
                    }
                }
                else
                {
                    for(qint32 i=0;i<nCount;i++)
                    {
                        INFORECORD infoRecord={};

                        infoRecord.nOffset=i*dynStruct.nSize;
                        infoRecord.nAddress=nAddress+infoRecord.nOffset;
                        infoRecord.sType=dynStruct.sName;
                        infoRecord.sName=QString("%1[%2]").arg(tr("Value"),QString::number(i));

                        infoRecord.sValue=getValue(pHandle,pBinary,infoRecord.nAddress,dynStruct.nSize,dynStruct.recordType,0,0);
                        infoRecord.sValueData=getValueData(infoRecord.nAddress,dynStruct.recordType,infoRecord.sType,infoRecord.sValue,1);
                        infoRecord.sComment=getComment(pHandle,pBinary,infoRecord.nAddress,sStructName,infoRecord.sType,infoRecord.sName);

                        result.listRecords.append(infoRecord);
                    }
                }
            }
        }
        else if(structType==STRUCTTYPE_POINTER)
        {
            result.bIsValid=true;

            qint32 nPointerSize=sizeof(void *);

            for(qint32 i=0;i<nCount;i++)
            {
                INFORECORD infoRecord={};

                infoRecord.nOffset=i*nPointerSize;
                infoRecord.nAddress=nAddress+infoRecord.nOffset;
                infoRecord.sType=sStructName+" *";
                infoRecord.sName=QString("Value[%1]").arg(i); // mb TODO translate

                infoRecord.sValue=getValue(pHandle,pBinary,infoRecord.nAddress,nPointerSize,RECORDTYPE_POINTER,0,0);
                infoRecord.sValueData=getValueData(infoRecord.nAddress,RECORDTYPE_POINTER,infoRecord.sType,infoRecord.sValue,1);

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
            QString sFilePrefix=fileInfo.absolutePath()+QDir::separator()+sGlobalName;

            QJsonArray jsonStructsArray=jsonObject.value("structs").toArray();

            qint32 nNumberOfStructs=jsonStructsArray.count();

            for(qint32 i=0;i<nNumberOfStructs;i++)
            {
                DYNSTRUCT record={};

                QJsonObject jsonStruct=jsonStructsArray.at(i).toObject();

                record.sIUID=XBinary::generateUUID(); // TODO Check
                record.sName=jsonStruct.value("name").toString();
                record.nSize=jsonStruct.value("size").toInt();
                record.recordType=getRecordType(record.sName);

                QString sInfoFile=jsonStruct.value("infofile").toString();

                if(sInfoFile!="")
                {
                    record.sInfoFilePrefix=sFilePrefix;
                    record.sInfoFile=sInfoFile;
                }

                QJsonArray jsonPositionsArray=jsonStruct.value("positions").toArray();

                qint32 nNumberOfPositions=jsonPositionsArray.count();

                for(qint32 j=0;j<nNumberOfPositions;j++)
                {
                    QJsonObject jsonPosition=jsonPositionsArray.at(j).toObject();

                    QString sName=jsonPosition.value("name").toString();
                    QString sType=jsonPosition.value("type").toString();
                    qint64 nOffset=jsonPosition.value("offset").toInt();
                    qint64 nSize=jsonPosition.value("size").toInt();
                    qint32 nBitOffset=jsonPosition.value("bitoffset").toInt();
                    qint32 nBitSize=jsonPosition.value("bitsize").toInt();
                    RECORDTYPE recordType=RECORDTYPE_AUTO;
                    qint32 nArrayCount=0;

                    if(sName.contains("["))
                    {
                        nArrayCount=sName.section("[",-1,-1).section("]",0,0).toInt();

                        nArrayCount=qMax(nArrayCount,1);
                        nArrayCount=qMin(nArrayCount,512); // TODO const

                        recordType=RECORDTYPE_ARRAY;
                    }
                    else if(sType.contains("*"))
                    {
                        recordType=RECORDTYPE_POINTER;
                    }
                    else
                    {
                        recordType=getRecordType(sType);
                    }

                    if(nArrayCount==0)
                    {
                        nArrayCount=1;
                    }

                    DSPOSITION position={};

                    position.sName=sName;
                    position.sType=sType;
                    position.nOffset=nOffset;
                    position.nSize=nSize;
                    position.nBitOffset=nBitOffset;
                    position.nBitSize=nBitSize;
                    position.recordType=recordType;
                    position.nArrayCount=nArrayCount;

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

QString XDynStructsEngine::getValue(void *pProcess,XBinary *pBinary,qint64 nAddress,qint64 nSize,RECORDTYPE recordType,qint32 nBitOffset,qint32 nBitSize)
{
    // TODO Endian
    QString sResult;

    if((recordType==RECORDTYPE_VARIABLE)||(recordType==RECORDTYPE_POINTER))
    {
        if(nSize==1)
        {
            quint8 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint8(pProcess,nAddress);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint8(nAddress);
            }

            nValue=XBinary::getBits_uint8(nValue,nBitOffset,nBitSize);

            sResult="0x"+XBinary::valueToHex(nValue);
        }
        else if(nSize==2)
        {
            quint16 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint16(pProcess,nAddress);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint16(nAddress);
            }

            nValue=XBinary::getBits_uint16(nValue,nBitOffset,nBitSize);

            sResult="0x"+XBinary::valueToHex(nValue);
        }
        else if(nSize==4)
        {
            quint32 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint32(pProcess,nAddress);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint32(nAddress);
            }

            nValue=XBinary::getBits_uint32(nValue,nBitOffset,nBitSize);

            sResult="0x"+XBinary::valueToHex(nValue);
        }
        else if(nSize==8)
        {
            quint64 nValue=0;

            if(pProcess)
            {
                nValue=XProcess::read_uint64(pProcess,nAddress);
            }
            else if(pBinary)
            {
                nValue=pBinary->read_uint64(nAddress);
            }

            nValue=XBinary::getBits_uint64(nValue,nBitOffset,nBitSize);

            sResult="0x"+XBinary::valueToHex(nValue);
        }
    }
    else
    {
        sResult="...";
    }

    return sResult;
}

QString XDynStructsEngine::getValueData(qint64 nAddress,RECORDTYPE recordType,QString sType,QString sValue,qint32 nArrayCount)
{
    QString sResult;

    if(recordType==RECORDTYPE_POINTER)
    {
        sType=sType.section("*",0,-2).trimmed();
        sResult=QString("%1&%2").arg(sValue,sType);
    }
    else if(recordType==RECORDTYPE_AUTO)
    {
        sResult=QString("0x%1&%2").arg(XBinary::valueToHex(nAddress),sType);
    }
    else if(recordType==RECORDTYPE_ARRAY)
    {
        STRUCTTYPE structType=STRUCTTYPE_VARIABLE;

        if(sType.contains("*"))
        {
            structType=STRUCTTYPE_POINTER;
        }

        sResult=QString("0x%1&%2&%3&%4").arg(XBinary::valueToHex(nAddress),sType,QString::number(structType),QString::number(nArrayCount));
    }

    return sResult;
}

QString XDynStructsEngine::getComment(void *pProcess,XBinary *pBinary,qint64 nAddress,QString sStructName,QString sType,QString sName)
{
    QString sResult;

    if(sType=="struct _UNICODE_STRING")
    {
        quint16 nStringSize=0;
        qint64 nStringAddress=0;
        QString sString;

        if(pProcess)
        {
            nStringSize=XProcess::read_uint16(pProcess,nAddress);

            if(sizeof(void *)==8)
            {
                nStringAddress=XProcess::read_uint64(pProcess,nAddress+8);
            }
            else
            {
                nStringAddress=XProcess::read_uint32(pProcess,nAddress+4);
            }
        }
        else if(pBinary)
        {
            nStringSize=pBinary->read_uint16(nAddress);

            if(sizeof(void *)==8)
            {
                nStringAddress=pBinary->read_uint64(nAddress+8);
            }
            else
            {
                nStringAddress=pBinary->read_uint32(nAddress+4);
            }
        }

        if(pProcess)
        {
            sString=XProcess::read_unicodeString(pProcess,nStringAddress,nStringSize);
        }
        else if(pBinary)
        {
            sString=pBinary->read_unicodeString(nStringAddress,nStringSize);
        }

        sResult=QString("\"%1\"").arg(sString);
    }

    if(sStructName=="struct _PEB_LDR_DATA")
    {
        if(sName=="InLoadOrderModuleList")
        {
            sResult=createListEntryLinks(pProcess,pBinary,nAddress,"struct _LDR_DATA_TABLE_ENTRY",0*(2*sizeof(void *)));
        }
        else if(sName=="InMemoryOrderModuleList")
        {
            sResult=createListEntryLinks(pProcess,pBinary,nAddress,"struct _LDR_DATA_TABLE_ENTRY",1*(2*sizeof(void *)));
        }
        else if(sName=="InInitializationOrderModuleList")
        {
            sResult=createListEntryLinks(pProcess,pBinary,nAddress,"struct _LDR_DATA_TABLE_ENTRY",2*(2*sizeof(void *)));
        }
    }
    else if(sStructName=="struct _LDR_DATA_TABLE_ENTRY")
    {
        if(sName=="InLoadOrderLinks")
        {
            sResult=createListEntryLinks(pProcess,pBinary,nAddress,"struct _LDR_DATA_TABLE_ENTRY",0*(2*sizeof(void *)));
        }
        else if(sName=="InMemoryOrderLinks")
        {
            sResult=createListEntryLinks(pProcess,pBinary,nAddress,"struct _LDR_DATA_TABLE_ENTRY",1*(2*sizeof(void *)));
        }
        else if(sName=="InInitializationOrderLinks")
        {
            sResult=createListEntryLinks(pProcess,pBinary,nAddress,"struct _LDR_DATA_TABLE_ENTRY",2*(2*sizeof(void *)));
        }
//        else if(sName=="InProgressLinks")
//        {
//            sResult=createListEntryLinks(pProcess,pBinary,nAddress,"struct _LDR_DATA_TABLE_ENTRY",0);
//        }
    }

    return sResult;
}

XDynStructsEngine::DYNSTRUCT XDynStructsEngine::getDynStructByName(QString sName)
{
    DYNSTRUCT result={};

    if(sName!="")
    {
        qint32 nNumberOfStructs=g_listDynStructs.count();

        for(qint32 i=0;i<nNumberOfStructs;i++)
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

XDynStructsEngine::RECORDTYPE XDynStructsEngine::getRecordType(QString sType)
{
    RECORDTYPE result=RECORDTYPE_AUTO;

    if( (sType=="unsigned char")||
        (sType=="unsigned short")||
        (sType=="unsigned int")||
        (sType=="unsigned long")||
        (sType=="unsigned long long")||
        (sType=="signed char")||
        (sType=="signed short")||
        (sType=="signed int")||
        (sType=="signed long")||
        (sType=="signed long long")||
        (sType=="char")||
        (sType=="short")||
        (sType=="int")||
        (sType=="long")||
        (sType=="long long"))
    {
        result=RECORDTYPE_VARIABLE;
    }

    return result;
}

QString XDynStructsEngine::createListEntryLinks(void *pProcess,XBinary *pBinary,qint64 nAddress,QString sStructName,qint64 nDeltaOffset)
{
    QString sResult;

    qint64 nFlink=0;
    qint64 nBlink=0;

    if(sizeof(void *)==8)
    {
        if(pProcess)
        {
            nFlink=XProcess::read_uint64(pProcess,nAddress);
            nBlink=XProcess::read_uint64(pProcess,nAddress+8);
        }
        else if(pBinary)
        {
            nFlink=pBinary->read_uint64(nAddress);
            nBlink=pBinary->read_uint64(nAddress+8);
        }
    }
    else
    {
        if(pProcess)
        {
            nFlink=XProcess::read_uint32(pProcess,nAddress);
            nBlink=XProcess::read_uint32(pProcess,nAddress+4);
        }
        else if(pBinary)
        {
            nFlink=pBinary->read_uint32(nAddress);
            nBlink=pBinary->read_uint32(nAddress+4);
        }
    }

    QString sFlink=QString("0x%1&%2").arg(XBinary::valueToHex(nFlink-nDeltaOffset),sStructName);
    QString sBlink=QString("0x%1&%2").arg(XBinary::valueToHex(nBlink-nDeltaOffset),sStructName);

    sResult=QString("<a href=\"%1\">Flink</a> <a href=\"%2\">Blink</a>").arg(sFlink,sBlink);

    return sResult;
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

    for(qint32 i=0;i<nNumberOfThreads;i++)
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
