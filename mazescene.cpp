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

#include "mazescene.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QPushButton>
#include <QKeyEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebView>

#include <qmath.h>
#include <qdebug.h>

#include <limits>

#include "scriptwidget.h"
#include "entity.h"
#include "modelitem.h"

#include <QVector3D>

#ifdef USE_PHONON
#include "mediaplayer/mediaplayer.h"
#endif

#ifndef QT_NO_OPENGL
#include <QGLWidget>
#endif

View::View()
    : m_scene(0)
{
}

void View::setScene(MazeScene *scene)
{
    QGraphicsView::setScene(scene);

    m_scene = scene;
    m_scene->viewResized(this);
}

void View::resizeEvent(QResizeEvent *)
{
    resetMatrix();
    qreal factor = width() / 4.0;
    scale(factor, factor);

    if (m_scene)
        m_scene->viewResized(this);
}

Light::Light(const QPointF &pos, qreal intensity)
    : m_pos(pos)
    , m_intensity(intensity)
{
}

qreal Light::intensityAt(const QPointF &pos) const
{
    const qreal quadraticIntensity = 150 * m_intensity;
    const qreal linearIntensity = 30 * m_intensity;

    const qreal d = QLineF(m_pos, pos).length();
    return quadraticIntensity / (d * d)
        + linearIntensity / d;
}

QMatrix4x4 fromRotation(float angle, Qt::Axis axis)
{
    QMatrix4x4 m;
    if (axis == Qt::XAxis)
        m.rotate(angle, QVector3D(1, 0, 0));
    else if (axis == Qt::YAxis)
        m.rotate(-angle, QVector3D(0, 1, 0));
    else if (axis == Qt::ZAxis)
        m.rotate(angle, QVector3D(0, 0, 1));
    return m;
}

QMatrix4x4 fromProjection(float fovAngle)
{
    float fov = qCos(fovAngle / 2) / qSin(fovAngle / 2);

    float zNear = 0.01;
    float zFar = 1000;

    float m33 = (zNear + zFar) / (zNear - zFar);
    float m34 = (2 * zNear * zFar) / (zNear - zFar);

    qreal data[] =
    {
        fov, 0, 0, 0,
        0, fov, 0, 0,
        0, 0, m33, m34,
        0, 0, -1, 0
    };
    return QMatrix4x4(data);
}

class WalkingItem : public QGraphicsPixmapItem
{
public:
    WalkingItem(MazeScene *scene);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    bool walking() const { return m_walking; }

private:
    void updatePixmap();

    MazeScene *m_scene;
    bool m_walking;

    QPixmap m_walkingPixmap;
    QPixmap m_standingPixmap;
};

QPixmap colorize(const QPixmap &source, const QColor &color)
{
    QImage temp(source.size(), QImage::Format_ARGB32_Premultiplied);

    temp.fill(0x0);
    QPainter p(&temp);
    p.drawPixmap(0, 0, source);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(temp.rect(), color);
    p.end();

    return QPixmap::fromImage(temp);
}

WalkingItem::WalkingItem(MazeScene *scene)
    : m_scene(scene)
    , m_walking(false)
    , m_walkingPixmap(colorize(QPixmap("walking.png"), QColor(Qt::green).darker()))
    , m_standingPixmap(colorize(QPixmap("standing.png"), QColor(Qt::green).darker()))
{
    setShapeMode(BoundingRectShape);
    updatePixmap();
}

void WalkingItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
    m_walking = !m_walking;
    updatePixmap();
}

void WalkingItem::updatePixmap()
{
    if (m_walking)
        setPixmap(m_walkingPixmap);
    else
        setPixmap(m_standingPixmap);
}

MazeScene::MazeScene(const QVector<Light> &lights, const char *map, int width, int height)
    : m_lights(lights)
    , m_walkingVelocity(0)
    , m_strafingVelocity(0)
    , m_turningSpeed(0)
    , m_pitchSpeed(0)
    , m_deltaYaw(0)
    , m_deltaPitch(0)
    , m_simulationTime(0)
    , m_walkTime(0)
    , m_width(width)
    , m_height(height)
    , m_player(0)
{
    m_camera.setPos(QPointF(1.5, 1.5));
    m_camera.setYaw(0.1);

    m_doorAnimation = new QTimeLine(1000, this);
    m_doorAnimation->setUpdateInterval(20);
    connect(m_doorAnimation, SIGNAL(valueChanged(qreal)), this, SLOT(moveDoors(qreal)));

    QMap<char, int> types;
    types[' '] = -2;
    types['-'] = -1;
    types['#'] = 0;
    types['&'] = 1;
    types['@'] = 2;
    types['%'] = 3;
    types['$'] = 4;
    types['?'] = 5;
    types['!'] = 6;
    types['='] = 7;
    types['*'] = 8;
    types['/'] = 9;
    types['.'] = 10;

    int type;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            type = types[map[y*width+x]];
            if (type >= 0)
                continue;

            type = types[map[(y-1)*width+x]];
            if (type >= -1)
                addWall(QPointF(x, y), QPointF(x+1, y), type);

            type = types[map[(y+1)*width+x]];
            if (type >= -1)
                addWall(QPointF(x+1, y+1), QPointF(x, y+1), type);

            type = types[map[y*width+x-1]];
            if (type >= -1)
                addWall(QPointF(x, y+1), QPointF(x, y), type);

            type = types[map[y*width+x+1]];
            if (type >= -1)
                addWall(QPointF(x+1, y), QPointF(x+1, y+1), type);
        }
    }

    QTimer *timer = new QTimer(this);
    timer->setInterval(20);
    timer->start();
    connect(timer, SIGNAL(timeout()), this, SLOT(move()));

    m_time.start();
    updateTransforms();
    updateRenderer();

    m_walkingItem = new WalkingItem(this);
    m_walkingItem->scale(0.008, 0.008);
    m_walkingItem->setZValue(100000);

    addItem(m_walkingItem);
}

void MazeScene::viewResized(QGraphicsView *view)
{
    QRectF bounds = m_walkingItem->sceneBoundingRect();
    QPointF bottomLeft = view->mapToScene(QPoint(5, view->height() - 5));

    m_walkingItem->setPos(bottomLeft.x(), bottomLeft.y() - bounds.height());
}

void MazeScene::addProjectedItem(ProjectedItem *item)
{
    addItem(item);
    m_projectedItems << item;
}

void MazeScene::addWall(const QPointF &a, const QPointF &b, int type)
{
    WallItem *item = new WallItem(this, a, b, type);
#ifdef USE_PHONON
    if (item->childItem() && item->type() == 7) {
        m_playerPos = (a + b ) / 2;
        m_player = static_cast<MediaPlayer *>(item->childItem()->widget());
    }
#endif

    QGraphicsProxyWidget *proxy = item->childItem();
    QWebView *view = proxy ? qobject_cast<QWebView *>(proxy->widget()) : 0;
    if (view) {
        connect(view, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished()));
        proxy->setVisible(false);
    }

    item->setVisible(false);
    addProjectedItem(item);
    m_walls << item;

    if (type == -1)
        m_doors << item;

    setSceneRect(-1, -1, 2, 2);
    if (item->childItem()) {
        QObject *widget = item->childItem()->widget()->children().value(0);
        QPushButton *button = qobject_cast<QPushButton *>(widget);
        if (button)
            m_buttons << button;
    }
}

void MazeScene::loadFinished()
{
    QWidget *widget = qobject_cast<QWidget *>(sender());

    if (widget) {
        foreach (WallItem *item, m_walls) {
            QGraphicsProxyWidget *proxy = item->childItem();
            if (proxy && proxy->widget() == widget)
                proxy->setVisible(true);
        }
    }
}

static inline QTransform rotatingTransform(qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return transform;
}

void Camera::setPitch(qreal pitch)
{
    m_pitch = qBound(qreal(-30), pitch, qreal(30));
    m_matrixDirty = true;
}

void Camera::setYaw(qreal yaw)
{
    m_yaw = yaw;
    m_matrixDirty = true;
}

void Camera::setPos(const QPointF &pos)
{
    m_pos = pos;
    m_matrixDirty = true;
}

void Camera::setFov(qreal fov)
{
    m_fov = fov;
    m_matrixDirty = true;
}

void Camera::setTime(qreal time)
{
    m_time = time;
    m_matrixDirty = true;
}


const QMatrix4x4 &Camera::viewMatrix() const
{
    updateMatrix();
    return m_viewMatrix;
}

const QMatrix4x4 &Camera::viewProjectionMatrix() const
{
    updateMatrix();
    return m_viewProjectionMatrix;
}

void Camera::updateMatrix() const
{
    if (!m_matrixDirty)
        return;

    m_matrixDirty = false;

    QMatrix4x4 m;
    m.scale(-1, 1, 1);
    m *= fromRotation(m_yaw + 180, Qt::YAxis);
    m.translate(-m_pos.x(), 0.04 * qSin(10 * m_time) + 0.1, -m_pos.y());
    m = fromRotation(m_pitch, Qt::XAxis) * m;
    m_viewMatrix = m;
    m_viewProjectionMatrix = fromProjection(m_fov) * m_viewMatrix;
}

void MazeScene::drawBackground(QPainter *painter, const QRectF &)
{
    static QImage floor = QImage("floor.png").convertToFormat(QImage::Format_RGB32);
    QBrush floorBrush(floor);

    static QImage ceiling = QImage("ceiling.png").convertToFormat(QImage::Format_RGB32);
    QBrush ceilingBrush(ceiling);

    QTransform brushScale;
    brushScale.scale(0.5 / floor.width(), 0.5 / floor.height());
    floorBrush.setTransform(brushScale);
    ceilingBrush.setTransform(brushScale);

    const QRectF r(1, 1, m_width-2, m_height-2);

    QMatrix4x4 m = m_camera.viewProjectionMatrix();

    QMatrix4x4 floorMatrix = m;
    floorMatrix.translate(0, 0.5, 0);
    floorMatrix *= fromRotation(90, Qt::XAxis);

    painter->save();
    painter->setTransform(floorMatrix.toTransform(0), true);
    painter->fillRect(r, floorBrush);
    painter->restore();

    QMatrix4x4 ceilingMatrix = m;
    ceilingMatrix.translate(0, -0.5, 0);
    ceilingMatrix *= fromRotation(90, Qt::XAxis);

    painter->save();
    painter->setTransform(ceilingMatrix.toTransform(0), true);
    painter->fillRect(r, ceilingBrush);
    painter->restore();
}

void MazeScene::addEntity(Entity *entity)
{
    addProjectedItem(entity);
    m_entities << entity;
}

ProjectedItem::ProjectedItem(const QRectF &bounds, bool shadow, bool opaque)
    : m_bounds(bounds)
    , m_shadowItem(0)
    , m_opaque(opaque)
    , m_obscured(false)
{
    if (shadow) {
        m_shadowItem = new QGraphicsRectItem(bounds, this);
        m_shadowItem->setPen(Qt::NoPen);
        m_shadowItem->setZValue(10);
    }

    m_targetRect = m_bounds;
}

void ProjectedItem::setOpaque(bool opaque)
{
    m_opaque = opaque;
}

bool ProjectedItem::isOpaque() const
{
    return m_opaque;
}

void ProjectedItem::setPosition(const QPointF &a, const QPointF &b)
{
    m_a = a;
    m_b = b;
}

void ProjectedItem::setLightingEnabled(bool enable)
{
    if (m_shadowItem)
        m_shadowItem->setVisible(enable);
}

class ProxyWidget : public QGraphicsProxyWidget
{
public:
    ProxyWidget(QGraphicsItem *parent = 0)
        : QGraphicsProxyWidget(parent)
    {
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant & value)
    {
        // we want the position of proxy widgets to stay at (0, 0)
        // so ignore the position changes from the native QWidget
        if (change == ItemPositionChange)
            return QPointF();
        else
            return QGraphicsProxyWidget::itemChange(change, value);
    }
};


WallItem::WallItem(MazeScene *scene, const QPointF &a, const QPointF &b, int type)
    : ProjectedItem(QRectF(-0.5, -0.5, 1.0, 1.0))
    , m_type(type)
{
    setPosition(a, b);

    static QImage brown = QImage("brown.png").convertToFormat(QImage::Format_RGB32);
    static QImage book = QImage("book.png").convertToFormat(QImage::Format_RGB32);
    static QImage door = QImage("door.png").convertToFormat(QImage::Format_RGB32);

    switch (type) {
    case -1:
        setImage(door);
        break;
    case 1:
        setImage(book);
        break;
    case 2:
        setOpaque(false);
        break;
    default:
        setImage(brown);
        break;
    }

    m_scale = 0.8;

    m_childItem = 0;
    QWidget *childWidget = 0;
    if (type == 3 && a.y() == b.y()) {
        QWidget *widget = new QWidget;
        QPushButton *button = new QPushButton("Open Sesame", widget);
        QObject::connect(button, SIGNAL(clicked()), scene, SLOT(toggleDoors()));
        widget->setLayout(new QVBoxLayout);
        widget->layout()->addWidget(button);
        childWidget = widget;
        m_scale = 0.3;
    } else if (type == 4) {
        View *view = new View;
        view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        view->resize(480, 320); // not soo big
        view->setViewport(new QWidget); // no OpenGL here
        childWidget = view;
    } else if (type == 5) {
        Entity *entity = new Entity(QPointF(6.5, 2.5));
        scene->addEntity(entity);
        childWidget = new ScriptWidget(scene, entity);
    } else if (type == 7) {
#ifdef USE_PHONON
        Q_INIT_RESOURCE(mediaplayer);
        childWidget = new MediaPlayer(QString());
        m_scale = 0.6;
#endif
    } else if (type == 8) {
        ModelItem *dialog = new ModelItem;
        childWidget = dialog;
        scene->addProjectedItem(dialog);
        m_scale = 0.5;
    } else if (type == 9) {
        if (a.x() > b.x()) {
            QWebView *view = new QWebView;
            view->setUrl(QUrl(QLatin1String("http://www.google.com")));
            childWidget = view;
        }
    } else if (type == 10) {
#ifndef QT_NO_OPENGL
        QWidget *widget = new QWidget;
        QCheckBox *checkBox = new QCheckBox("Use OpenGL", widget);
        checkBox->setChecked(true);
        QObject::connect(checkBox, SIGNAL(toggled(bool)), scene, SLOT(toggleRenderer()), Qt::QueuedConnection);
        widget->setLayout(new QVBoxLayout);
        widget->layout()->addWidget(checkBox);
        childWidget = widget;
        m_scale = 0.2;
#endif
    }

    if (!childWidget)
        return;

    childWidget->installEventFilter(scene);

    m_childItem = new ProxyWidget(this);
    m_childItem->setWidget(childWidget);
    m_childItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    QRectF rect = m_childItem->boundingRect();
    QPointF center = rect.center();

    qreal scale = qMin(m_scale / rect.width(), m_scale / rect.height());
    m_childItem->translate(0, -0.05);
    m_childItem->scale(scale, scale);
    m_childItem->translate(-center.x(), -center.y());
}

bool MazeScene::eventFilter(QObject *target, QEvent *event)
{
    QWidget *widget = qobject_cast<QWidget *>(target);
    if (!widget || event->type() != QEvent::Resize)
        return false;

    foreach (WallItem *item, m_walls) {
        QGraphicsProxyWidget *proxy = item->childItem();
        if (proxy && proxy->widget() == widget)
            item->childResized();
    }

    return false;
}

void WallItem::childResized()
{
    QRectF rect = m_childItem->boundingRect();

    QPointF center = rect.center();

    qreal scale = qMin(m_scale / rect.width(), m_scale / rect.height());
    m_childItem->resetMatrix();
    m_childItem->translate(0, -0.05);
    m_childItem->scale(scale, scale);
    m_childItem->translate(-center.x(), -center.y());

    // refresh cache size
    m_childItem->setCacheMode(QGraphicsItem::NoCache);
    m_childItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
}

void ProjectedItem::updateLighting(const QVector<Light> &lights, bool useConstantLight)
{
    if (!m_shadowItem)
        return;

    if (useConstantLight) {
        m_shadowItem->setBrush(QColor(0, 0, 0, 100));
        return;
    }

    const qreal constantIntensity = 80;
    qreal la = constantIntensity;
    qreal lb = constantIntensity;
    foreach (const Light &light, lights) {
        la += light.intensityAt(m_b);
        lb += light.intensityAt(m_a);
    }

    int va = qMax(0, 255 - int(la));
    int vb = qMax(0, 255 - int(lb));

    if (va == vb) {
        m_shadowItem->setBrush(QColor(0, 0, 0, va));
    } else {
        const QRectF rect = boundingRect();
        QLinearGradient g(rect.topLeft(), rect.topRight());

        g.setColorAt(0, QColor(0, 0, 0, va));
        g.setColorAt(1, QColor(0, 0, 0, vb));

        m_shadowItem->setBrush(g);
    }
}

QRectF ProjectedItem::boundingRect() const
{
    return m_bounds;
}

void ProjectedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!m_image.isNull()) {
        QRectF target = m_targetRect.translated(0.5, 0.5);
        QRectF source = QRectF(0, 0, m_image.width() * (1 - target.x()), m_image.height());
        painter->drawImage(m_targetRect, m_image, source);
    }
}

void ProjectedItem::setAnimationTime(qreal time)
{
    // hacky way of handling door animation
    QRectF rect = boundingRect();
    m_targetRect = QRectF(QPointF(rect.left() + rect.width() * time, rect.top()),
                          rect.bottomRight());
    if (m_shadowItem)
        m_shadowItem->setRect(m_targetRect);
    update();
}

void ProjectedItem::setImage(const QImage &image)
{
    m_image = image;
    update();
}

void ProjectedItem::setObscured(bool obscured)
{
    m_obscured = obscured;
}

bool ProjectedItem::isObscured() const
{
    return m_obscured;
}

void ProjectedItem::updateTransform(const Camera &camera)
{
    if (!m_obscured) {
        QTransform rotation;
        rotation *= QTransform().translate(-camera.pos().x(), -camera.pos().y());
        rotation *= rotatingTransform(camera.yaw());
        QPointF center = (m_a + m_b) / 2;

        QPointF ca = rotation.map(m_a);
        QPointF cb = rotation.map(m_b);

        if (ca.y() > 0 || cb.y() > 0) {
            QMatrix4x4 m = camera.viewProjectionMatrix();
            m.translate(center.x(), 0, center.y());
            m *= fromRotation(-QLineF(m_b, m_a).angle(), Qt::YAxis);

            qreal zm = QLineF(camera.pos(), center).length();

            setVisible(true);
            setZValue(-zm);
            setTransform(m.toTransform(0));
            return;
        }
    }

    // hide the item by placing it far outside the scene
    // we could use setVisible() but that causes unnecessary
    // update to cahced items
    QTransform transform;
    transform.translate(-1000, -1000);
    setTransform(transform);
}


void MazeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (focusItem()) {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        QPointF delta(event->scenePos() - event->lastScenePos());
        m_deltaYaw += delta.x() * 80;
        m_deltaPitch -= delta.y() * 80;
    }
}

void MazeScene::keyPressEvent(QKeyEvent *event)
{
    if (handleKey(event->key(), true)) {
        event->accept();
        return;
    }

    QGraphicsScene::keyPressEvent(event);
}

void MazeScene::keyReleaseEvent(QKeyEvent *event)
{
    if (handleKey(event->key(), false)) {
        event->accept();
        return;
    }

    QGraphicsScene::keyReleaseEvent(event);
}

bool MazeScene::handleKey(int key, bool pressed)
{
    if (focusItem())
        return false;

    switch (key) {
    case Qt::Key_Left:
    case Qt::Key_Q:
        m_turningSpeed = (pressed ? -0.5 : 0.0);
        return true;
    case Qt::Key_Right:
    case Qt::Key_E:
        m_turningSpeed = (pressed ? 0.5 : 0.0);
        return true;
    case Qt::Key_Down:
        m_pitchSpeed = (pressed ? 0.5 : 0.0);
        return true;
    case Qt::Key_Up:
        m_pitchSpeed = (pressed ? -0.5 : 0.0);
        return true;
    case Qt::Key_S:
        m_walkingVelocity = (pressed ? -0.01 : 0.0);
        return true;
    case Qt::Key_W:
        m_walkingVelocity = (pressed ? 0.01 : 0.0);
        return true;
    case Qt::Key_A:
        m_strafingVelocity = (pressed ? -0.01 : 0.0);
        return true;
    case Qt::Key_D:
        m_strafingVelocity = (pressed ? 0.01 : 0.0);
        return true;
    }

    return false;
}

static inline QRectF rectFromPoint(const QPointF &point, qreal size)
{
    return QRectF(point, point).adjusted(-size/2, -size/2, size/2, size/2);
}

bool MazeScene::blocked(const QPointF &pos, Entity *me) const
{
    const QRectF rect = rectFromPoint(pos, me ? 0.7 : 0.25);

    foreach (WallItem *item, m_walls) {
        if (item->type() == 6
            || item->type() == -1 && m_doorAnimation->state() != QTimeLine::Running
               && m_doorAnimation->direction() == QTimeLine::Backward)
            continue;

        const QPointF a = item->a();
        const QPointF b = item->b();

        QRectF wallRect = QRectF(a, b).adjusted(-0.01, -0.01, 0.01, 0.01);

        if (wallRect.intersects(rect))
            return true;
    }

    foreach (Entity *entity, m_entities) {
        if (entity == me)
            continue;
        QRectF entityRect = rectFromPoint(entity->pos(), 0.8);

        if (entityRect.intersects(rect))
            return true;
    }

    if (me) {
        QRectF cameraRect = rectFromPoint(m_camera.pos(), 0.4);

        if (cameraRect.intersects(rect))
            return true;
    }

    return false;
}

bool MazeScene::tryMove(QPointF &pos, const QPointF &delta, Entity *entity) const
{
    const QPointF old = pos;

    if (delta.x() != 0 && !blocked(pos + QPointF(delta.x(), 0), entity))
        pos.setX(pos.x() + delta.x());

    if (delta.y() != 0 && !blocked(pos + QPointF(0, delta.y()), entity))
        pos.setY(pos.y() + delta.y());

    return pos != old;
}

struct Span
{
    ProjectedItem *item;

    // screen coordinates
    float sx1;
    float sx2;

    float cy;
};

int split(QList<Span> &list, float x)
{
    for (int i = 0; i < list.size(); ++i) {
        Span &span = list[i];
        if (span.sx2 == x)
            return i+1;

        if (span.sx1 <= x && span.sx2 > x) {
            Span split = span;
            span.sx2 = x;
            split.sx1 = x;
            list.insert(i+1, split);
            return i+1;
        }
    }
    return -1;
}

bool insertSpan(QList<Span> &list, const Span &span, bool checkOnly)
{
    int left = split(list, span.sx1);
    int right = split(list, span.sx2);

    bool visible = false;
    for (int i = left; i < right; ++i) {
        Span &s = list[i];
        if (s.cy > span.cy) {
            visible = true;
            if (!checkOnly) {
                s.item = span.item;
                s.cy = span.cy;
            }
        }
    }
    return visible;
}

bool insertProjectedItem(QList<Span> &list, ProjectedItem *item, const QTransform &cameraTransform, bool checkOnly)
{
    QPointF ca = cameraTransform.map(item->a());
    QPointF cb = cameraTransform.map(item->b());

    if (ca.y() <= 0 && cb.y() <= 0)
        return false;

    const float clip = 0.0001;
    if (ca.y() <= 0) {
        float t = (clip - ca.y()) / (cb.y() - ca.y());
        ca.setX(ca.x() + t * (cb.x() - ca.x()));
        ca.setY(clip);
    } else if(cb.y() <= 0) {
        float t = (clip - ca.y()) / (cb.y() - ca.y());
        cb.setX(ca.x() + t * (cb.x() - ca.x()));
        cb.setY(clip);
    }

    Span span;
    span.item = item;
    span.sx1 = ca.x() / ca.y();
    span.sx2 = cb.x() / cb.y();
    span.cy = (ca.y() + cb.y()) * 0.5f;

    if (span.sx1 >= span.sx2)
        qSwap(span.sx1, span.sx2);

    return insertSpan(list, span, checkOnly);
}

void MazeScene::updateTransforms()
{
    Span span;
    span.item = 0;
    span.sx1 = -std::numeric_limits<float>::infinity();
    span.sx2 =  std::numeric_limits<float>::infinity();
    span.cy = std::numeric_limits<float>::infinity();

    QList<Span> visibleList;
    visibleList << span;

    QTransform rotation;
    rotation *= QTransform().translate(-m_camera.pos().x(), -m_camera.pos().y());
    rotation *= rotatingTransform(m_camera.yaw());

    // first add all opaque items
    foreach (ProjectedItem *item, m_projectedItems) {
        if (item->isOpaque()) {
            item->setObscured(true);
            insertProjectedItem(visibleList, item, rotation, false);
        }
    }

    // mark visible opaque items
    for (int i = 0; i < visibleList.size(); ++i)
        if (visibleList.at(i).item)
            visibleList.at(i).item->setObscured(false);

    // now add all non-opaque items
    foreach (ProjectedItem *item, m_projectedItems) {
        if (!item->isOpaque())
            item->setObscured(!insertProjectedItem(visibleList, item, rotation, true));
    }

    foreach (ProjectedItem *item, m_projectedItems)
        item->updateTransform(m_camera);

    foreach (WallItem *item, m_walls) {
        if (item->isVisible() && !item->isObscured()) {
            // embed recursive scene
            if (QGraphicsProxyWidget *child = item->childItem()) {
                View *view = qobject_cast<View *>(child->widget());
                if (view && !view->scene()) {
                    const char *map =
                        "#$###"
                        "#   #"
                        "# @ #"
                        "#   #"
                        "#####";
                    QVector<Light> lights;
                    lights << Light(QPointF(2.5, 2.5), 1)
                           << Light(QPointF(1.5, 1.5), 0.4);
                    MazeScene *embeddedScene = new MazeScene(lights, map, 5, 5);
                    view->setScene(embeddedScene);
                    view->setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
                }
            }
        }
    }

#ifdef USE_PHONON
    if (m_player) {
        qreal distance = QLineF(m_camera.pos(), m_playerPos).length();
        m_player->setVolume(qPow(2, -0.3 * distance));
    }
#endif
    setFocusItem(0); // setVisible(true) might give focus to one of the items
    update();
}

void MazeScene::move()
{
    QSet<Entity *> movedEntities;
    long elapsed = m_time.elapsed();
    bool walked = false;

    const int stepSize = 5;
    int steps = (elapsed - m_simulationTime) / stepSize;

    if (steps) {
        m_deltaYaw /= steps;
        m_deltaPitch /= steps;

        m_deltaYaw += m_turningSpeed;
        m_deltaPitch += m_pitchSpeed;
    }

    qreal walkingVelocity = m_walkingVelocity;
    if (m_walkingItem->walking())
        walkingVelocity = 0.005;

    for (int i = 0; i < steps; ++i) {
        m_camera.setYaw(m_camera.yaw() + m_deltaYaw);
        m_camera.setPitch(m_camera.pitch() + m_deltaPitch);

        bool walking = false;
        if (walkingVelocity != 0) {
            QPointF walkingDelta = QLineF::fromPolar(walkingVelocity, m_camera.yaw() - 90).p2();
            QPointF pos = m_camera.pos();
            if (tryMove(pos, walkingDelta)) {
                walking = true;
                m_camera.setPos(pos);
            }
        }

        if (m_strafingVelocity != 0) {
            QPointF walkingDelta = QLineF::fromPolar(m_strafingVelocity, m_camera.yaw()).p2();
            QPointF pos = m_camera.pos();
            if (tryMove(pos, walkingDelta)) {
                walking = true;
                m_camera.setPos(pos);
            }
        }

        walked = walked || walking;

        if (walking)
            m_walkTime += stepSize;
        m_simulationTime += stepSize;

        foreach (Entity *entity, m_entities) {
            if (entity->move(this))
                movedEntities.insert(entity);
        }
    }

    m_camera.setTime(m_walkTime * 0.001);

    if (walked || m_deltaYaw != 0 || m_deltaPitch != 0) {
        updateTransforms();
    } else {
        foreach (Entity *entity, movedEntities)
            entity->updateTransform(m_camera);
    }

    if (steps) {
        m_deltaYaw = 0;
        m_deltaPitch = 0;
    }
}

void MazeScene::toggleDoors()
{
    setFocusItem(0);

    if (m_doorAnimation->state() == QTimeLine::Running)
        return;

    foreach (QPushButton *button, m_buttons) {
        if (m_doorAnimation->direction() == QTimeLine::Forward)
            button->setText("Close Sesame!");
        else
            button->setText("Open Sesame!");
    }

    m_doorAnimation->toggleDirection();
    m_doorAnimation->start();
}

void MazeScene::moveDoors(qreal value)
{
    bool opaqueStatusChanged = false;
    foreach (WallItem *item, m_doors) {
        item->setAnimationTime(1 - value);

        bool shouldBeOpaque = qFuzzyCompare(value, 1);
        if (item->isOpaque() != shouldBeOpaque) {
            opaqueStatusChanged = true;
            item->setOpaque(shouldBeOpaque);
        }
    }
    if (opaqueStatusChanged)
        updateTransforms();
}

void MazeScene::toggleRenderer()
{
#ifndef QT_NO_OPENGL
    if (views().size() == 0)
        return;
    QGraphicsView *view = views().at(0);
    if (view) {
        view->setViewport(
                view->viewport()->inherits("QGLWidget")
                ? new QWidget
                : new QGLWidget(QGLFormat(QGL::SampleBuffers)));

        updateRenderer();
    }
#endif
}

void MazeScene::updateRenderer()
{
    QGraphicsView *view = views().isEmpty() ? 0 : views().at(0);
    bool accelerated = view && view->viewport()->inherits("QGLWidget");
    if (view) {
        if (accelerated)
            view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        else
            view->setRenderHints(QPainter::Antialiasing);
    }

    foreach (WallItem *item, m_walls)
        item->updateLighting(m_lights, item->type() == 2);
}
