#include "mazescene.h"

#include <QCheckBox>
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

MazeScene::MazeScene(const char *map, int width, int height)
    : m_cameraPos(1.5, 1.5)
    , m_cameraAngle(0.1)
    , m_walkingVelocity(0)
    , m_turningVelocity(0)
    , m_simulationTime(0)
    , m_walkTime(0)
    , m_width(width)
    , m_height(height)
{
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
}

void MazeScene::addWall(const QPointF &a, const QPointF &b, int type)
{
    WallItem *item = new WallItem(this, a, b, type);
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

    QTransform rotation = rotatingTransform(m_cameraAngle);
    rotation.translate(-m_cameraPos.x(), -m_cameraPos.y());

    static QImage floor = QImage("floor.png").convertToFormat(QImage::Format_RGB32);
    QBrush floorBrush(floor);

    static QImage ceiling = QImage("ceiling.png").convertToFormat(QImage::Format_RGB32);
    QBrush ceilingBrush(ceiling);

    QTransform brushScale;
    brushScale.scale(0.5 / floor.width(), 0.5 / floor.height());
    floorBrush.setTransform(brushScale);
    ceilingBrush.setTransform(brushScale);

    QTransform project;
    const qreal fov = 0.5;
    const qreal wallHeight = 0.5 + 0.04 * qSin(0.01 * m_walkTime);
    const qreal ceilingHeight = -0.5 + 0.04 * qSin(0.01 * m_walkTime);
    const QRectF r(1, 1, m_width-2, m_height-2);

    painter->save();
    project = QTransform(rotation.m11(), 0, fov * rotation.m12(), rotation.m21(), 0, fov * rotation.m22(), rotation.m31(), wallHeight, fov * rotation.m32());
    painter->setTransform(project, true);
    painter->fillRect(r, floorBrush);
    painter->restore();

    painter->save();
    project = QTransform(rotation.m11(), 0, fov * rotation.m12(), rotation.m21(), 0, fov * rotation.m22(), rotation.m31(), ceilingHeight, fov * rotation.m32());
    painter->setTransform(project, true);
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
        "http://labs.trolltech.com/blogs/",
        "http://chaos.troll.no/~tavestbo/webkit"
    };

    qreal scale = 0.8;

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(Qt::transparent));

    m_childItem = 0;
    QWidget *childWidget = 0;
    if (type == 3 && a.y() == b.y()) {
        QWidget *widget = new QWidget;
        QPushButton *button = new QPushButton("Open Sesame", widget);
        QObject::connect(button, SIGNAL(pressed()), scene, SLOT(toggleDoors()));
        widget->setLayout(new QVBoxLayout);
        widget->layout()->addWidget(button);
        widget->setPalette(palette);
        childWidget = widget;
        scale = 0.3;
    } else if (type == 4) {
        View *view = new View;
        view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        view->resize(480, 320); // not soo big
        view->setViewport(new QWidget); // no OpenGL here
        childWidget = view;
    } else if (type == 5) {
        scene->addEntity(new Entity(QPointF(3.5, 3.5)));
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
            scale = 0.2;
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

    m_childItem = new QGraphicsProxyWidget(this);
    m_childItem->setWidget(childWidget);
    m_childItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    QRectF rect = m_childItem->boundingRect();
    QPointF center = rect.center();

    scale = qMin(scale / rect.width(), scale / rect.height());
    m_childItem->scale(scale, scale);
    m_childItem->translate(-center.x(), -center.y());
}

void ProjectedItem::setDepths(qreal za, qreal zb)
{
    if (!m_shadowItem)
        return;

    const qreal falloff = 40;
    const int maxAlpha = 180;
    int va = int(falloff * zb);
    int vb = int(falloff * za);

    if (va == vb || va >= maxAlpha && vb >= maxAlpha) {
        m_shadowItem->setBrush(QColor(0, 0, 0, qMin(maxAlpha, va)));
    } else {
        qreal xa = 0;
        qreal xb = 1;

        if (va >= maxAlpha) {
            xa = 1 - (maxAlpha - vb) / (va - vb);
            va = maxAlpha;
        }

        if (vb >= maxAlpha) {
            xb = (maxAlpha - va) / (vb - va);
            vb = maxAlpha;
        }

        const QRectF rect = boundingRect();
        QLinearGradient g(rect.topLeft(), rect.topRight());

        g.setColorAt(xa, QColor(0, 0, 0, va));
        g.setColorAt(xb, QColor(0, 0, 0, vb));

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
}

void ProjectedItem::updateTransform(const QPointF &cameraPos, qreal cameraAngle, qreal time)
{
    QTransform rotation = rotatingTransform(cameraAngle);
    rotation.translate(-cameraPos.x(), -cameraPos.y());

    QPointF ca = rotation.map(m_a);
    QPointF cb = rotation.map(m_b);

    if (ca.y() <= 0 && cb.y() <= 0) {
        setVisible(false);
        return;
    }

    const qreal mx = ca.x() - cb.x();
    const qreal tx = 0.5 * (ca.x() + cb.x());
    const qreal mz = ca.y() - cb.y();
    const qreal tz = 0.5 * (ca.y() + cb.y());
    const qreal fov = 0.5;

    const QTransform project(mx, 0, mz * fov, 0, 1, 0, tx, 0.04 * qSin(10 * time), tz * fov);

    const qreal za = QLineF(QPointF(), ca).length();
    const qreal zb = QLineF(QPointF(), cb).length();
    const qreal zm = QLineF(QPointF(), (ca + cb) / 2).length();

    setVisible(true);
    setZValue(-zm);
    setTransform(project);

    setDepths(za, zb);
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
    case Qt::Key_Right:
        m_turningVelocity = (pressed ? (key == Qt::Key_Left ? -1 : 1) * 0.5 : 0.0);
        return true;
    case Qt::Key_Up:
    case Qt::Key_Down:
        m_walkingVelocity = (pressed ? (key == Qt::Key_Down ? -1 : 1) * 0.01 : 0.0);
        return true;
    }

    return false;
}

static inline QRectF rectFromPoint(const QPointF &point, qreal size)
{
    return QRectF(point, point).adjusted(-size/2, -size/2, size/2, size/2);
}

bool MazeScene::blocked(const QPointF &pos) const
{
    const QRectF rect = rectFromPoint(pos, 0.4);

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
        QRectF entityRect = rectFromPoint(entity->pos(), 0.4);

        if (entityRect.intersects(rect))
            return true;
    }

    return false;
}

bool MazeScene::tryMove(QPointF &pos, const QPointF &delta) const
{
    bool moved = false;
    if (!blocked(pos + QPointF(delta.x(), 0))) {
        pos.setX(pos.x() + delta.x());
        moved = true;
    }

    if (!blocked(pos + QPointF(0, delta.y()))) {
        pos.setY(pos.y() + delta.y());
        moved = true;
    }

    return moved;
}

void MazeScene::updateTransforms()
{
    foreach (WallItem *item, m_walls) {
        item->updateTransform(m_cameraPos, m_cameraAngle, m_walkTime * 0.001);
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
                    MazeScene *embeddedScene = new MazeScene(map, 5, 5);
                    view->setScene(embeddedScene);
                }
            }
        }
    }
    foreach (Entity *entity, m_entities)
        entity->updateTransform(m_cameraPos, m_cameraAngle, m_walkTime * 0.001);
    setFocusItem(0); // setVisible(true) might give focus to one of the items
    update();
}

void MazeScene::move()
{
    qreal oldCameraAngle = m_cameraAngle;
    QPointF oldCameraPos = m_cameraPos;

    long elapsed = m_time.elapsed();
    while (m_simulationTime <= elapsed) {
        m_cameraAngle += m_turningVelocity;

        if (m_walkingVelocity != 0) {
            QPointF walkingDir = rotatingTransform(-m_cameraAngle).map(QPointF(0, 1));
            QPointF walkingDelta = m_walkingVelocity * walkingDir;
            if (tryMove(m_cameraPos, walkingDelta))
                m_walkTime += 5;
        }

        m_simulationTime += 5;
    }

    if (oldCameraAngle != m_cameraAngle || oldCameraPos != m_cameraPos)
        updateTransforms();
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
    : ProjectedItem(QRectF(-0.3, -0.3, 0.6, 0.8), false)
    , m_pos(pos)
{
    static QImage img = toAlpha(QImage("soldier/O01.png").convertToFormat(QImage::Format_RGB32));
    setImage(img);
}

void Entity::updateTransform(const QPointF &cameraPos, qreal cameraRotation, qreal time)
{
    QPointF delta = cameraPos - m_pos;
    delta /= QLineF(QPointF(), delta).length();
    delta = rotatingTransform(90.1).map(delta);
    setPosition(m_pos - delta, m_pos + delta);
    ProjectedItem::updateTransform(cameraPos, cameraRotation, time);
}

void MazeScene::addEntity(Entity *entity)
{
    addItem(entity);
    m_entities << entity;
}
