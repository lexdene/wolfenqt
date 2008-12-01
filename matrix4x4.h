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

#ifndef MATRIX4X4_H
#define MATRIX4X4_H

#include <qnamespace.h>
#include <QTransform>

#include "point3d.h"

class Matrix4x4
{
public:
    Matrix4x4();
    Matrix4x4(float *data);

    Matrix4x4 &operator*=(const Matrix4x4 &other);
    Matrix4x4 operator*(const Matrix4x4 &other) const;

    Point3d operator*(const Point3d &point) const;

    static Matrix4x4 fromRotation(float angle, Qt::Axis axis);
    static Matrix4x4 fromTranslation(float dx, float dy, float dz);
    static Matrix4x4 fromScale(float sx, float sy, float sz);
    static Matrix4x4 fromProjection(float fov);

    QTransform toQTransform() const;
    static Matrix4x4 fromQTransform(const QTransform &transform);

    void setData(float *data);
    const float *data() const { return m; }

    Matrix4x4 orderSwapped() const;

private:
    float m[16];
};

#endif
