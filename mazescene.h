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

#ifndef MAZESCENE_H
#define MAZESCENE_H

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointF>
#include <QPushButton>
#include <QTime>
#include <QTimeLine>

#include "matrix4x4.h"

class MazeScene;
class MediaPlayer;
class Entity;

class View : public QGraphicsView
{
    Q_OBJECT
public:
    void resizeEvent(QResizeEvent *event);
};

class Camera
{
public:
    Camera()
        : m_yaw(0)
        , m_pitch(0)
        , m_fov(70)
        , m_time(0)
        , m_matrixDirty(true)
    {
    }

    qreal yaw() const { return m_yaw; }
    qreal pitch() const { return m_pitch; }
    qreal fov() const { return m_fov; }
    QPointF pos() const { return m_pos; }

    void setYaw(qreal yaw);
    void setPitch(qreal pitch);
    void setPos(const QPointF &pos);
    void setFov(qreal fov);
    void setTime(qreal time);

    const Matrix4x4 &viewProjectionMatrix() const;
    const Matrix4x4 &viewMatrix() const;

private:
    void updateMatrix() const;

    qreal m_yaw;
    qreal m_pitch;
    qreal m_fov;
    qreal m_time;

    QPointF m_pos;

    mutable bool m_matrixDirty;
    mutable Matrix4x4 m_viewMatrix;
    mutable Matrix4x4 m_viewProjectionMatrix;
};

class Light
{
public:
    Light() {}
    Light(const QPointF &pos, qreal intensity);

    qreal intensityAt(const QPointF &pos) const;

    QPointF pos() const { return m_pos; }
    qreal intensity() const { return m_intensity; }

private:
    QPointF m_pos;
    qreal m_intensity;
};

class ProjectedItem : public QGraphicsItem
{
public:
    ProjectedItem(const QRectF &bounds, bool shadow = true);

    QPointF a() const { return m_a; }
    QPointF b() const { return m_b; }

    virtual void updateTransform(const Camera &camera);

    QRectF boundingRect() const;

    void setPosition(const QPointF &a, const QPointF &b);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void setAnimationTime(qreal time);
    void setImage(const QImage &image);
    void updateLighting(const QVector<Light> &lights, bool useConstantLight);

    void setLightingEnabled(bool enabled);

private:
    QPointF m_a;
    QPointF m_b;
    QRectF m_bounds;
    QRectF m_targetRect;
    QImage m_image;
    QGraphicsRectItem *m_shadowItem;
};

class WallItem : public ProjectedItem
{
public:
    WallItem(MazeScene *scene, const QPointF &a, const QPointF &b, int type);

    QGraphicsProxyWidget *childItem() const
    {
        return m_childItem;
    }

    int type() const { return m_type; }

    void childResized();

private:
    QGraphicsProxyWidget *m_childItem;
    int m_type;
    qreal m_scale;
};

class MazeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    MazeScene(const QVector<Light> &lights, const char *map, int width, int height);

    void addProjectedItem(ProjectedItem *item);
    void addEntity(Entity *entity);
    void addWall(const QPointF &a, const QPointF &b, int type);
    void drawBackground(QPainter *painter, const QRectF &rect);

    bool tryMove(QPointF &pos, const QPointF &delta, Entity *entity = 0) const;

    Camera camera() const { return m_camera; }

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    bool eventFilter(QObject *target, QEvent *event);

    bool handleKey(int key, bool pressed);

public slots:
    void move();
    void toggleRenderer();
    void toggleDoors();
    void loadFinished();

private slots:
    void moveDoors(qreal value);

private:
    bool blocked(const QPointF &pos, Entity *entity) const;
    void updateTransforms();
    void updateRenderer();

    QVector<WallItem *> m_walls;
    QVector<WallItem *> m_doors;
    QVector<QGraphicsItem *> m_floorTiles;
    QVector<QPushButton *> m_buttons;
    QVector<Entity *> m_entities;
    QVector<Light> m_lights;
    QVector<ProjectedItem *> m_projectedItems;

    Camera m_camera;

    qreal m_walkingVelocity;
    qreal m_strafingVelocity;
    qreal m_turningSpeed;
    qreal m_pitchSpeed;

    qreal m_deltaYaw;
    qreal m_deltaPitch;

    QTime m_time;
    QTimeLine *m_doorAnimation;
    long m_simulationTime;
    long m_walkTime;
    int m_width;
    int m_height;
    MediaPlayer *m_player;
    QPointF m_playerPos;
};

#endif
