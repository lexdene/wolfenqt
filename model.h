/****************************************************************************

This file is part of the wolfenqt project on http://qt.gitorious.org.

Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).*
All rights reserved.

Contact:  Nokia Corporation (qt-info@nokia.com)**

You may use this file under the terms of the BSD license as follows:

"Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* Neither the name of Nokia Corporation and its Subsidiary(-ies) nor the
* names of its contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE."

****************************************************************************/
#ifndef MODEL_H
#define MODEL_H

#include <QPainter>
#include <QString>
#include <QVector>

#include <math.h>

#include <QMatrix4x4>
#include <QVector3D>

class Model
{
public:
    Model() {}
    Model(const QString &filePath);

    void render(bool wireframe = false, bool normals = false) const;
    void render(QPainter *painter, const QMatrix4x4 &matrix, bool normals = false) const;

    QString fileName() const { return m_fileName; }
    int faces() const { return m_pointIndices.size() / 3; }
    int edges() const { return m_edgeIndices.size() / 2; }
    int points() const { return m_points.size(); }

    QVector3D size() const;

private:
    QString m_fileName;

    QVector<QVector3D> m_points;
    QVector<QVector3D> m_normals;

#ifdef QT_OPENGL_ES_2
    QVector<ushort> m_edgeIndices;
    QVector<ushort> m_pointIndices;
#else
    QVector<uint> m_edgeIndices;
    QVector<uint> m_pointIndices;
#endif

    QVector3D m_size;

    mutable QVector<QLineF> m_lines;
    mutable QVector<QVector3D> m_mapped;
};

#endif
