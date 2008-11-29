#ifndef MATRIX4X4_H
#define MATRIX4X4_H

#include <qnamespace.h>
#include <QTransform>

class Matrix4x4
{
public:
    Matrix4x4();
    Matrix4x4(qreal *data);

    Matrix4x4 &operator*=(const Matrix4x4 &other);
    Matrix4x4 operator*(const Matrix4x4 &other) const;

    static Matrix4x4 fromRotation(qreal angle, Qt::Axis axis);
    static Matrix4x4 fromTranslation(qreal dx, qreal dy, qreal dz);
    static Matrix4x4 fromScale(qreal sx, qreal sy, qreal sz);
    static Matrix4x4 fromProjection(qreal fov);
    static Matrix4x4 convert2dTo3d();

    QTransform toQTransform() const;

    void setData(qreal *data);

private:
    qreal m[16];
};

#endif
