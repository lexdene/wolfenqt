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
