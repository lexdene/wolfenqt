#include "mazescene.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGLWidget>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QPushButton>
#include <QKeyEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebView>

#include <qmath.h>
#include <qdebug.h>

#include "matrix4x4.h"

#ifdef USE_PHONON
#include "mediaplayer/mediaplayer.h"
#endif

View::View()
{
    resize(1024, 768);
    setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

void View::resizeEvent(QResizeEvent *)
{
    resetMatrix();

    qreal factor = width() / 4.0;
    scale(factor, factor);
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

MazeScene::MazeScene(const QVector<Light> &lights, const char *map, int width, int height)
    : m_lights(lights)
    , m_walkingVelocity(0)
    , m_strafingVelocity(0)
    , m_turningSpeed(0)
    , m_pitchSpeed(0)
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

    foreach (WallItem *item, m_walls) {
        if (item->type() == 2)
            item->setConstantLight(0.4);
        else
            item->updateLighting(m_lights);
    }

    QTimer *timer = new QTimer(this);
    timer->setInterval(20);
    timer->start();
    connect(timer, SIGNAL(timeout()), this, SLOT(move()));

    m_time.start();
    updateTransforms();
}

void MazeScene::addWall(const QPointF &a, const QPointF &b, int type)
{
    WallItem *item = new WallItem(this, a, b, type);
#ifdef USE_PHONON
    if (item->type() == 7) {
        m_playerPos = (a + b ) / 2;
        m_player = static_cast<MediaPlayer *>(item->childItem()->widget());
    }
#endif
    item->setVisible(false);
    addItem(item);
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

static inline QTransform rotatingTransform(qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return transform;
}

void MazeScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QLinearGradient g(QPointF(0, rect.top()), QPointF(0, rect.bottom()));

    g.setColorAt(0, QColor(0, 0, 0, 50));
    g.setColorAt(0.4, QColor(0, 0, 0, 100));
    g.setColorAt(0.6, QColor(0, 0, 0, 100));
    g.setColorAt(1, QColor(0, 0, 0, 50));
    painter->fillRect(QRectF(rect.topLeft(), QPointF(rect.right(), rect.center().y())), QColor(100, 120, 200));
    painter->fillRect(QRectF(QPointF(rect.left(), rect.center().y()), rect.bottomRight()), QColor(127, 190, 100));

    static QImage floor = QImage("floor.png").convertToFormat(QImage::Format_RGB32);
    QBrush floorBrush(floor);

    static QImage ceiling = QImage("ceiling.png").convertToFormat(QImage::Format_RGB32);
    QBrush ceilingBrush(ceiling);

    QTransform brushScale;
    brushScale.scale(0.5 / floor.width(), 0.5 / floor.height());
    floorBrush.setTransform(brushScale);
    ceilingBrush.setTransform(brushScale);

    const QRectF r(1, 1, m_width-2, m_height-2);

    Matrix4x4 m;
    m *= Matrix4x4::fromRotation(m_camera.yaw(), Qt::YAxis);
    m *= Matrix4x4::fromRotation(m_camera.pitch(), Qt::XAxis);
    m *= Matrix4x4::fromProjection(0.5);

    qreal heightOffset = 0.04 * qSin(0.01 * m_walkTime) + 0.1;

    Matrix4x4 floorMatrix = Matrix4x4::fromRotation(90, Qt::XAxis);
    floorMatrix *= Matrix4x4::fromTranslation(-m_camera.pos().x(), heightOffset + 0.5, -m_camera.pos().y());
    floorMatrix *= m;

    painter->save();
    painter->setTransform(floorMatrix.toQTransform(), true);
    painter->fillRect(r, floorBrush);
    painter->restore();

    Matrix4x4 ceilingMatrix = Matrix4x4::fromRotation(90, Qt::XAxis);
    ceilingMatrix *= Matrix4x4::fromTranslation(-m_camera.pos().x(), heightOffset - 0.5, -m_camera.pos().y());
    ceilingMatrix *= m;

    painter->save();
    painter->setTransform(ceilingMatrix.toQTransform(), true);
    painter->fillRect(r, ceilingBrush);
    painter->restore();

    painter->fillRect(rect, g);
}

ProjectedItem::ProjectedItem(const QRectF &bounds, bool shadow)
    : m_bounds(bounds)
    , m_shadowItem(0)
{
    if (shadow) {
        m_shadowItem = new QGraphicsRectItem(bounds, this);
        m_shadowItem->setPen(Qt::NoPen);
        m_shadowItem->setZValue(10);
    }

    m_targetRect = m_bounds;
}

void ProjectedItem::setPosition(const QPointF &a, const QPointF &b)
{
    m_a = a;
    m_b = b;
}

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
        break;
    default:
        setImage(brown);
        break;
    }

    static const char *urls[] =
    {
        "http://www.google.com",
        "http://programming.reddit.com",
        "http://www.trolltech.com",
        "http://www.planetkde.org",
        "http://labs.trolltech.com/blogs/"
    };

    m_scale = 0.8;

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(Qt::transparent));

    m_childItem = 0;
    QWidget *childWidget = 0;
    if (type == 3 && a.y() == b.y()) {
        QWidget *widget = new QWidget;
        QPushButton *button = new QPushButton("Open Sesame", widget);
        QObject::connect(button, SIGNAL(clicked()), scene, SLOT(toggleDoors()));
        widget->setLayout(new QVBoxLayout);
        widget->layout()->addWidget(button);
        widget->setPalette(palette);
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
    } else if (type == 0 || type == 2) {
        static int index;
        if (index == 0) {
            QWidget *widget = new QWidget;
            QCheckBox *checkBox = new QCheckBox("Use OpenGL", widget);
            checkBox->setChecked(true);
            QObject::connect(checkBox, SIGNAL(toggled(bool)), scene, SLOT(toggleRenderer()), Qt::QueuedConnection);
            widget->setLayout(new QVBoxLayout);
            widget->layout()->addWidget(checkBox);
            widget->setPalette(palette);
            childWidget = widget;
            m_scale = 0.2;
        } else if (!(index % 7)) {
            static int webIndex = 0;
            const char *url = urls[webIndex++ % (sizeof(urls)/sizeof(char*))];

            QWebView *view = new QWebView;
            view->setUrl(QUrl(url));

            childWidget = view;
        }

        ++index;
    }

    if (!childWidget)
        return;

    childWidget->installEventFilter(scene);

    m_childItem = new QGraphicsProxyWidget(this);
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
        if (!proxy)
            continue;
        if (proxy->widget() == widget) {
            QRectF rect = proxy->boundingRect();

            QPointF center = rect.center();

            qreal scale = item->childScale();
            scale = qMin(scale / rect.width(), scale / rect.height());
            proxy->resetMatrix();
            proxy->translate(0, -0.05);
            proxy->scale(scale, scale);
            proxy->translate(-center.x(), -center.y());

            // refresh cache size
            proxy->setCacheMode(QGraphicsItem::NoCache);
            proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);
            break;
        }
    }

    return false;
}

void ProjectedItem::setConstantLight(qreal light)
{
    if (m_shadowItem)
        m_shadowItem->setBrush(QColor(0, 0, 0, int(light * 255)));
}

void ProjectedItem::updateLighting(const QVector<Light> &lights)
{
    if (!m_shadowItem)
        return;

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

void ProjectedItem::updateTransform(const Camera &camera, qreal time)
{
    QTransform rotation;
    rotation *= QTransform().translate(-camera.pos().x(), -camera.pos().y());
    rotation *= rotatingTransform(camera.yaw());
    QPointF center = (m_a + m_b) / 2;

    QPointF ca = rotation.map(m_a);
    QPointF cb = rotation.map(m_b);

    if (ca.y() <= 0 && cb.y() <= 0) {
        setVisible(false);
        return;
    }

    const qreal fov = 0.5;

    Matrix4x4 m;
    m *= Matrix4x4::fromRotation(-QLineF(m_b, m_a).angle(), Qt::YAxis);
    m *= Matrix4x4::fromTranslation(center.x() - camera.pos().x(), 0.04 * qSin(10 * time) + 0.1, center.y() - camera.pos().y());
    m *= Matrix4x4::fromRotation(camera.yaw(), Qt::YAxis);
    m *= Matrix4x4::fromRotation(camera.pitch(), Qt::XAxis);
    m *= Matrix4x4::fromProjection(fov);

    qreal zm = QLineF(camera.pos(), center).length();

    setVisible(true);
    setZValue(-zm);
    setTransform(m.toQTransform());
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

void MazeScene::updateTransforms()
{
    foreach (WallItem *item, m_walls) {
        item->updateTransform(m_camera, m_walkTime * 0.001);
        if (item->isVisible()) {
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
                }
            }
        }
    }
    foreach (Entity *entity, m_entities)
        entity->updateTransform(m_camera, m_walkTime * 0.001);

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
    while (m_simulationTime <= elapsed) {
        m_camera.setYaw(m_camera.yaw() + m_turningSpeed);
        m_camera.setPitch(qBound(qreal(-30), m_camera.pitch() + m_pitchSpeed, qreal(30)));

        bool walking = false;
        if (m_walkingVelocity != 0) {
            QPointF walkingDelta = QLineF::fromPolar(m_walkingVelocity, m_camera.yaw() - 90).p2();
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
            m_walkTime += 5;
        m_simulationTime += 5;

        foreach (Entity *entity, m_entities) {
            if (entity->move(this))
                movedEntities.insert(entity);
        }
    }

    if (walked || m_turningSpeed != 0 || m_pitchSpeed != 0) {
        updateTransforms();
    } else {
        foreach (Entity *entity, movedEntities)
            entity->updateTransform(m_camera, m_walkTime * 0.001);
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
    foreach (WallItem *item, m_doors)
        item->setAnimationTime(1 - value);
}

void MazeScene::toggleRenderer()
{
    if (views().size() == 0)
        return;
    QGraphicsView *view = views().at(0);
    if (view->viewport()->inherits("QGLWidget"))
        view->setViewport(new QWidget);
    else
        view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
}

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

void Entity::updateTransform(const Camera &camera, qreal time)
{
    qreal angleToCamera = QLineF(m_pos, camera.pos()).angle();
    int cameraAngleIndex = mod(qRound(angleToCamera + 22.5), 360) / 45;

    m_angleIndex = mod(qRound(cameraAngleIndex * 45 - m_angle + 22.5), 360) / 45;

    QPointF delta = QLineF::fromPolar(1, 270.1 + 45 * cameraAngleIndex).p2();
    setPosition(m_pos - delta, m_pos + delta);

    updateImage();
    ProjectedItem::updateTransform(camera, time);
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
                m_angle -= qMin(-angleToTarget, 0.5);
            else
                m_angle += qMin(angleToTarget, 0.5);
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

void MazeScene::addEntity(Entity *entity)
{
    addItem(entity);
    m_entities << entity;
}

static QScriptValue qsRand(QScriptContext *, QScriptEngine *engine)
{
    QScriptValue value(engine, qrand() / (RAND_MAX + 1.0));
    return value;
}

void ScriptWidget::setPreset(int preset)
{
    const char *presets[] =
    {
        "// available functions:\n"
        "// entity.turnLeft()\n"
        "// entity.turnRight()\n"
        "// entity.turnTowards(x, y)\n"
        "// entity.walk()\n"
        "// entity.stop()\n"
        "// rand()\n"
        "// script.display()\n"
        "\n"
        "// available variables:\n"
        "// my_x\n"
        "// my_y\n"
        "// player_x\n"
        "// player_y\n"
        "// time\n"
        "\n"
        "entity.stop();\n",
        "entity.walk();\n"
        "if ((time % 20000) < 10000) {\n"
        "  entity.turnTowards(10, 2.5);\n"
        "  if (my_x >= 5.5)\n"
        "    entity.stop();\n"
        "} else {\n"
        "  entity.turnTowards(-10, 2.5);\n"
        "  if (my_x <= 2.5)\n"
        "    entity.stop();\n"
        "}\n",
        "dx = player_x - my_x;\n"
        "dy = player_y - my_y;\n"
        "if (dx * dx + dy * dy < 5) {\n"
        "  entity.stop();\n"
        "} else {\n"
        "  entity.walk();\n"
        "  entity.turnTowards(player_x, player_y);\n"
        "}\n"
    };

    m_sourceEdit->setPlainText(QLatin1String(presets[preset]));
}

ScriptWidget::ScriptWidget(MazeScene *scene, Entity *entity)
    : m_scene(scene)
    , m_entity(entity)
{
    new QVBoxLayout(this);

    m_statusView = new QLineEdit;
    m_statusView->setReadOnly(true);
    layout()->addWidget(m_statusView);

    m_sourceEdit = new QPlainTextEdit;
    layout()->addWidget(m_sourceEdit);

    QPushButton *compileButton = new QPushButton(QLatin1String("Compile"));
    layout()->addWidget(compileButton);

    QComboBox *combo = new QComboBox;
    layout()->addWidget(combo);

    combo->addItem(QLatin1String("Default"));
    combo->addItem(QLatin1String("Patrol"));
    combo->addItem(QLatin1String("Follow"));

    setPreset(0);
    connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(setPreset(int)));
    connect(compileButton, SIGNAL(clicked()), this, SLOT(updateSource()));

    m_engine = new QScriptEngine(this);
    QScriptValue entityObject = m_engine->newQObject(m_entity);
    m_engine->globalObject().setProperty("entity", entityObject);
    QScriptValue widgetObject = m_engine->newQObject(this);
    m_engine->globalObject().setProperty("script", widgetObject);
    m_engine->globalObject().setProperty("rand", m_engine->newFunction(qsRand));

    m_engine->setProcessEventsInterval(5);

    resize(300, 400);
    updateSource();

    startTimer(50);
    m_time.start();
}

void ScriptWidget::timerEvent(QTimerEvent *)
{
    QPointF player = m_scene->camera().pos();
    QPointF entity = m_entity->pos();

    QScriptValue px(m_engine, player.x());
    QScriptValue py(m_engine, player.y());
    QScriptValue ex(m_engine, entity.x());
    QScriptValue ey(m_engine, entity.y());
    QScriptValue time(m_engine, m_time.elapsed());

    m_engine->globalObject().setProperty("player_x", px);
    m_engine->globalObject().setProperty("player_y", py);
    m_engine->globalObject().setProperty("my_x", ex);
    m_engine->globalObject().setProperty("my_y", ey);
    m_engine->globalObject().setProperty("time", time);

    m_engine->evaluate(m_source);
    if (m_engine->hasUncaughtException()) {
        QString text = m_engine->uncaughtException().toString();
        m_statusView->setText(text);
    }
}

void ScriptWidget::display(QScriptValue value)
{
    m_statusView->setText(value.toString());
}

void ScriptWidget::updateSource()
{
    bool wasEvaluating = m_engine->isEvaluating();
    if (wasEvaluating)
        m_engine->abortEvaluation();

    m_time.restart();
    m_source = m_sourceEdit->toPlainText();
    if (wasEvaluating)
        m_statusView->setText(QLatin1String("Aborted long running evaluation!"));
    else if (m_engine->canEvaluate(m_source))
        m_statusView->setText(QLatin1String("Evaluation succeeded"));
    else
        m_statusView->setText(QLatin1String("Evaluation failed"));
}
