/****************************************************************************

This file is part of the wolfenqt project on http://qt.gitorious.org.

Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).*
All rights reserved.

Contact:  Nokia Corporation (qt-info@nokia.com)**

You may use this file under the terms of the BSD license as follows:

"Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* Neither the name of Nokia Corporation and its Subsidiary(-ies) nor the
* names of its contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE."

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

#include <QMatrix4x4>

class MazeScene;
class MediaPlayer;
class Entity;
class WalkingItem;

class View : public QGraphicsView
{
    Q_OBJECT
public:
    View();
    void resizeEvent(QResizeEvent *event);
    void setScene(MazeScene *scene);
    void paintEvent(QPaintEvent *event);

private:
    MazeScene *m_scene;
    bool m_firstPaint;
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

    const QMatrix4x4 &viewProjectionMatrix() const;
    const QMatrix4x4 &viewMatrix() const;

private:
    void updateMatrix() const;

    qreal m_yaw;
    qreal m_pitch;
    qreal m_fov;
    qreal m_time;

    QPointF m_pos;

    mutable bool m_matrixDirty;
    mutable QMatrix4x4 m_viewMatrix;
    mutable QMatrix4x4 m_viewProjectionMatrix;
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
    ProjectedItem(const QRectF &bounds, bool shadow = true, bool opaque = true);

    QPointF a() const { return m_a; }
    QPointF b() const { return m_b; }

    virtual void updateTransform(const Camera &camera);

    void setOpaque(bool opaque);
    bool isOpaque() const;

    QRectF boundingRect() const;

    void setPosition(const QPointF &a, const QPointF &b);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void setAnimationTime(qreal time);
    void setImage(const QImage &image);
    void updateLighting(const QVector<Light> &lights, bool useConstantLight);

    void setLightingEnabled(bool enabled);

    void setObscured(bool obscured);
    bool isObscured() const;

private:
    QPointF m_a;
    QPointF m_b;
    QRectF m_bounds;
    QRectF m_targetRect;
    QImage m_image;
    QGraphicsRectItem *m_shadowItem;

    bool m_opaque;
    bool m_obscured;
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

    void viewResized(QGraphicsView *view);
    void setAcceleratedViewport(bool accelerated);

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

    bool m_accelerated;

    WalkingItem *m_walkingItem;
};

#endif
