#include "matrix4x4.h"

Matrix4x4::Matrix4x4()
{
    float identity[] =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    setData(identity);
}

Matrix4x4::Matrix4x4(float *data)
{
    setData(data);
}

Matrix4x4 Matrix4x4::orderSwapped() const
{
    float data[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            data[4*i + j] = m[4*j + i];
    return Matrix4x4(data);
}

void Matrix4x4::setData(float *data)
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

Point3d Matrix4x4::operator*(const Point3d &p) const
{
    qreal x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
    qreal y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
    qreal z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    qreal w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];

    return Point3d(x / w, y / w, z / w);
}

Matrix4x4 Matrix4x4::fromRotation(float angle, Qt::Axis axis)
{
    QTransform rot;
    rot.rotate(angle);
    switch (axis) {
    case Qt::XAxis:
        {
        float data[] =
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
        float data[] =
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
        float data[] =
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

Matrix4x4 Matrix4x4::fromTranslation(float dx, float dy, float dz)
{
    float data[] =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        dx, dy, dz, 1
    };
    return Matrix4x4(data);
}

Matrix4x4 Matrix4x4::fromScale(float sx, float sy, float sz)
{
    float data[] =
    {
        sx, 0, 0, 0,
        0, sy, 0, 0,
        0, 0, sz, 0,
        0, 0, 0, 1
    };
    return Matrix4x4(data);
}

Matrix4x4 Matrix4x4::fromProjection(float fov)
{
    float zNear = 0.001;
    float zFar = 1000;

    float m33 = (zNear + zFar) / (zNear - zFar);
    float m34 = (2 * zNear * zFar) / (zNear - zFar);

    float data[] =
    {
        fov, 0, 0, 0,
        0, fov, 0, 0,
        0, 0, m33, -1,
        0, 0, m34, 0
    };
    return Matrix4x4(data);
}

QTransform Matrix4x4::toQTransform() const
{
    return QTransform(m[0], m[1], m[3],
                      m[4], m[5], m[7],
                      m[12], m[13], m[15]);
}

Matrix4x4 Matrix4x4::fromQTransform(const QTransform &transform)
{
    float data[] =
    {
        transform.m11(), transform.m12(), 0, transform.m13(),
        transform.m21(), transform.m22(), 0, transform.m23(),
        0, 0, 1, 0,
        transform.m31(), transform.m32(), 0, transform.m33()
    };

    return Matrix4x4(data);
}
