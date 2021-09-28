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
#ifndef XDYNSTRUCTSENGINE_H
#define XDYNSTRUCTSENGINE_H

#include "xbinary.h"
#include "xprocess.h"

class XDynStructsEngine : public QObject
{
    Q_OBJECT

    enum VT
    {
        VT_AUTO=0,
        VT_NONE,
        VT_VARIABLE,
        VT_POINTER,
        VT_ARRAY
    };

    struct DSPOSITION
    {
        qint64 nOffset;
        qint64 nSize;
        QString sName;
        QString sType;
        VT vt;
    };

    struct DYNSTRUCT
    {
        QString sIUID;
        QString sName;
        QString sInfo;
        qint64 nSize;
        QList<DSPOSITION> listPositions;
    };

public:
    struct INFORECORD
    {
        qint64 nAddress;
        qint64 nOffset; // Offset from begin; do not show if -1
        QString sType;
        QString sName;
        QString sValue;
        QString sValueData; // If not empty  -> active link
        QString sComment;
    };

    struct INFO
    {
        QList<INFORECORD> listRecords;
    };

    explicit XDynStructsEngine(QObject *pParent=nullptr);

    INFO getInfo(QIODevice *pDevice,qint64 nOffset,DYNSTRUCT *pDynStruct=0);
    INFO getInfo(qint64 nProcessId,DYNSTRUCT *pDynStruct=0);

private:
    INFORECORD getPEB(qint64 nProcessId);

signals:

};

#endif // XDYNSTRUCTSENGINE_H
