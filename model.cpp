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

#include "model.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QVarLengthArray>

#ifndef QT_NO_OPENGL
#ifndef QT_OPENGL_ES_2
#include <GL/glew.h>
#endif
#include <QtOpenGL>
#endif

Model::Model(const QString &filePath)
    : m_fileName(QFileInfo(filePath).fileName())
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QVector3D boundsMin( 1e9, 1e9, 1e9);
    QVector3D boundsMax(-1e9,-1e9,-1e9);

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString input = in.readLine();
        if (input.isEmpty() || input[0] == '#')
            continue;

        QTextStream ts(&input);
        QString id;
        ts >> id;
        if (id == "v") {
            QVector3D p;
            for (int i = 0; i < 3; ++i) {
                ts >> ((float *)&p)[i];
                ((float *)&boundsMin)[i] = qMin(((float *)&boundsMin)[i], ((float *)&p)[i]);
                ((float *)&boundsMax)[i] = qMax(((float *)&boundsMax)[i], ((float *)&p)[i]);
            }
            m_points << p;
        } else if (id == "f" || id == "fo") {
            QVarLengthArray<int, 4> p;

            while (!ts.atEnd()) {
                QString vertex;
                ts >> vertex;
                const int vertexIndex = vertex.split('/').value(0).toInt();
                if (vertexIndex)
                    p.append(vertexIndex > 0 ? vertexIndex - 1 : m_points.size() + vertexIndex);
            }

            for (int i = 0; i < p.size(); ++i) {
                const int edgeA = p[i];
                const int edgeB = p[(i + 1) % p.size()];

                if (edgeA < edgeB)
                    m_edgeIndices << edgeA << edgeB;
            }

            for (int i = 0; i < 3; ++i)
                m_pointIndices << p[i];

            if (p.size() == 4)
                for (int i = 0; i < 3; ++i)
                    m_pointIndices << p[(i + 2) % 4];
        }
    }

    const QVector3D bounds = boundsMax - boundsMin;
    const qreal scale = 1 / qMax(bounds.x() / 1, qMax(bounds.y(), bounds.z() / 1));
    for (int i = 0; i < m_points.size(); ++i)
        m_points[i] = (m_points[i] - (boundsMin + bounds * 0.5)) * scale;

    m_size = bounds * scale;

    m_normals.resize(m_points.size());
    for (int i = 0; i < m_pointIndices.size(); i += 3) {
        const QVector3D a = m_points.at(m_pointIndices.at(i));
        const QVector3D b = m_points.at(m_pointIndices.at(i+1));
        const QVector3D c = m_points.at(m_pointIndices.at(i+2));

        const QVector3D normal = QVector3D::crossProduct(b - a, c - a).normalized();

        for (int j = 0; j < 3; ++j)
            m_normals[m_pointIndices.at(i + j)] += normal;
    }

    for (int i = 0; i < m_normals.size(); ++i)
        m_normals[i] = m_normals[i].normalized();
}

QVector3D Model::size() const
{
    return m_size;
}

void Model::render(bool wireframe, bool normals) const
{
#ifdef QT_NO_OPENGL
    Q_UNUSED(wireframe);
    Q_UNUSED(normals);
#else
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(true);

#ifdef QT_OPENGL_ES_2
    GLenum elementType = GL_UNSIGNED_SHORT;
#else
    GLenum elementType = GL_UNSIGNED_INT;
#endif

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (float *)m_points.data());
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (float *)m_normals.data());

    if (wireframe)
        glDrawElements(GL_LINES, m_edgeIndices.size(), elementType, m_edgeIndices.data());
    else
        glDrawElements(GL_TRIANGLES, m_pointIndices.size(), elementType, m_pointIndices.data());

    if (normals) {
        QVector<QVector3D> points;
        QVector<QVector3D> normals;
        for (int i = 0; i < m_normals.size(); ++i) {
            points << m_points.at(i) << (m_points.at(i) + m_normals.at(i) * 0.02f);
            normals << m_normals.at(i) << m_normals.at(i);
        }

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (float *)points.data());
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (float *)normals.data());
        glDrawArrays(GL_LINES, 0, points.size());
    }

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glDisable(GL_DEPTH_TEST);
#endif
}

void Model::render(QPainter *painter, const QMatrix4x4 &matrix, bool normals) const
{
    m_mapped.resize(m_points.size());
    for (int i = 0; i < m_points.size(); ++i)
        m_mapped[i] = matrix.map(m_points.at(i));

    m_lines.clear();
    for (int i = 0; i < m_edgeIndices.size(); i += 2) {
        const QVector3D a = m_mapped.at(m_edgeIndices.at(i));
        const QVector3D b = m_mapped.at(m_edgeIndices.at(i+1));

        if (a.z() > 0 && b.z() > 0)
            m_lines << QLineF(a.toPointF(), b.toPointF());
    }

    if (normals) {
        for (int i = 0; i < m_normals.size(); ++i) {
            const QVector3D a = m_mapped.at(i);
            const QVector3D b = matrix.map(m_points.at(i) + m_normals.at(i) * 0.02f);

            if (a.z() > 0 && b.z() > 0)
                m_lines << QLineF(a.toPointF(), b.toPointF());
        }
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->drawLines(m_lines);
}
