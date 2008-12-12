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

#ifndef ENTITY_H
#define ENTITY_H

#include <QPointF>
#include <QObject>

#include "mazescene.h"

class Entity : public QObject, public ProjectedItem
{
    Q_OBJECT
public:
    Entity(const QPointF &pos);
    void updateTransform(const Camera &camera);

    QPointF pos() const { return m_pos; }

    bool move(MazeScene *scene);

public slots:
    void turnTowards(qreal x, qreal y);
    void turnLeft();
    void turnRight();
    void walk();
    void stop();

protected:
    void timerEvent(QTimerEvent *event);

private:
    void updateImage();

private:
    QPointF m_pos;
    qreal m_angle;
    bool m_walking;
    bool m_walked;

    qreal m_turnVelocity;
    QPointF m_turnTarget;

    bool m_useTurnTarget;

    int m_animationIndex;
    int m_angleIndex;
};

#endif
