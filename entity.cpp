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

#include "entity.h"

const QImage toAlpha(const QImage &image)
{
    if (image.isNull())
        return image;
    QRgb alpha = image.pixel(0, 0);
    QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QRgb *data = reinterpret_cast<QRgb *>(result.bits());
    int size = image.width() * image.height();
    for (int i = 0; i < size; ++i)
        if (data[i] == alpha)
            data[i] = 0;
    return result;
}

Entity::Entity(const QPointF &pos)
    : ProjectedItem(QRectF(-0.3, -0.4, 0.6, 0.9), false)
    , m_pos(pos)
    , m_angle(180)
    , m_walking(false)
    , m_walked(false)
    , m_turnVelocity(0)
    , m_useTurnTarget(false)
    , m_animationIndex(0)
    , m_angleIndex(0)
{
    startTimer(300);
}

void Entity::walk()
{
    m_walking = true;
}

void Entity::stop()
{
    m_walking = false;
    m_useTurnTarget = false;
    m_turnVelocity = 0;
}

void Entity::turnTowards(qreal x, qreal y)
{
    m_turnTarget = QPointF(x, y);
    m_useTurnTarget = true;
}

void Entity::turnLeft()
{
    m_useTurnTarget = false;
    m_turnVelocity = -0.5;
}

void Entity::turnRight()
{
    m_useTurnTarget = false;
    m_turnVelocity = 0.5;
}

static QVector<QImage> loadSoldierImages()
{
    QVector<QImage> images;
    for (int i = 1; i <= 40; ++i) {
        QImage image(QString("soldier/O%0.png").arg(i, 2, 10, QLatin1Char('0')));
        images << toAlpha(image.convertToFormat(QImage::Format_RGB32));
    }
    return images;
}

static inline int mod(int x, int y)
{
    return ((x % y) + y) % y;
}

void Entity::updateTransform(const Camera &camera)
{
    qreal angleToCamera = QLineF(m_pos, camera.pos()).angle();
    int cameraAngleIndex = mod(qRound(angleToCamera + 22.5), 360) / 45;

    m_angleIndex = mod(qRound(cameraAngleIndex * 45 - m_angle + 22.5), 360) / 45;

    QPointF delta = QLineF::fromPolar(1, 270.1 + 45 * cameraAngleIndex).p2();
    setPosition(m_pos - delta, m_pos + delta);

    updateImage();
    ProjectedItem::updateTransform(camera);
}

bool Entity::move(MazeScene *scene)
{
    bool moved = false;
    if (m_useTurnTarget) {
        qreal angleToTarget = QLineF::fromPolar(1, m_angle)
            .angleTo(QLineF(m_pos, m_turnTarget));

        if (angleToTarget != 0) {
            if (angleToTarget >= 180)
                angleToTarget -= 360;

            if (angleToTarget < 0)
                m_angle -= qMin(-angleToTarget, qreal(0.5));
            else
                m_angle += qMin(angleToTarget, qreal(0.5));
            moved = true;
        }
    } else if (m_turnVelocity != 0) {
        m_angle += m_turnVelocity;
        moved = true;
    }

    m_walked = false;
    if (m_walking) {
        QPointF walkingDelta = QLineF::fromPolar(0.006, m_angle).p2();
        if (scene->tryMove(m_pos, walkingDelta, this)) {
            moved = true;
            m_walked = true;
        }
    }

    return moved;
}

void Entity::timerEvent(QTimerEvent *)
{
    ++m_animationIndex;
    updateImage();
}

void Entity::updateImage()
{
    static QVector<QImage> images = loadSoldierImages();
    if (m_walked)
        setImage(images.at(8 + 8 * (m_animationIndex % 4) + m_angleIndex));
    else
        setImage(images.at(m_angleIndex));
}
