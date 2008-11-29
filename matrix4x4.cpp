#include "matrix4x4.h"

Matrix4x4::Matrix4x4()
{
    qreal identity[] =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    setData(identity);
}

Matrix4x4::Matrix4x4(qreal *data)
{
    setData(data);
}

void Matrix4x4::setData(qreal *data)
{
    for (int i = 0; i < 16; ++i)
        m[i] = data[i];
}

Matrix4x4 &Matrix4x4::operator*=(const Matrix4x4 &rhs)
{
    const Matrix4x4 lhs = *this;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[4*i + j] = 0;
            for (int k = 0; k < 4; ++k)
                m[4*i + j] += lhs.m[4*i + k] * rhs.m[4*k + j];
        }
    }
    return *this;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4 &other) const
{
    return Matrix4x4(*this) *= other;
}

Matrix4x4 Matrix4x4::fromRotation(qreal angle, Qt::Axis axis)
{
    QTransform rot;
    rot.rotate(angle);
    switch (axis) {
    case Qt::XAxis:
        {
        qreal data[] =
        {
            1, 0, 0, 0,
            0, rot.m11(), rot.m12(), 0,
            0, rot.m21(), rot.m22(), 0,
            0, 0, 0, 1
        };
        return Matrix4x4(data);
        }
    case Qt::YAxis:
        {
        qreal data[] =
        {
            rot.m11(), 0, rot.m12(), 0,
            0, 1, 0, 0,
            rot.m21(), 0, rot.m22(), 0,
            0, 0, 0, 1
        };
        return Matrix4x4(data);
        }
    case Qt::ZAxis:
        {
        qreal data[] =
        {
            rot.m11(), rot.m12(), 0, 0,
            rot.m21(), rot.m22(), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        return Matrix4x4(data);
        }
    default:
        return Matrix4x4();
    }
}

Matrix4x4 Matrix4x4::fromTranslation(qreal dx, qreal dy, qreal dz)
{
    qreal data[] =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        dx, dy, dz, 1
    };
    return Matrix4x4(data);
}

Matrix4x4 Matrix4x4::fromScale(qreal sx, qreal sy, qreal sz)
{
    qreal data[] =
    {
        sx, 0, 0, 0,
        0, sy, 0, 0,
        0, 0, sz, 0,
        0, 0, 0, 1
    };
    return Matrix4x4(data);
}

Matrix4x4 Matrix4x4::fromProjection(qreal fov)
{
    qreal data[] =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, fov,
        0, 0, 0, 0
    };
    return Matrix4x4(data);
}

Matrix4x4 convert2dTo3d()
{
    qreal data[] =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 1
    };
    return Matrix4x4(data);
}

QTransform Matrix4x4::toQTransform() const
{
    return QTransform(m[0], m[1], m[3],
                      m[4], m[5], m[7],
                      m[12], m[13], m[15]);
}
