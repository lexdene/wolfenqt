/****************************************************************************
**
** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Graphics Dojo project on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
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
