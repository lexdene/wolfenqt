#include "mazescene.h"

#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QKeyEvent>
#include <QTimer>
#include <QWebView>

MazeScene::MazeScene()
    : m_cameraPos(1.5, 1.5)
    , m_cameraAngle(180)
    , m_walkingVelocity(0)
    , m_turningVelocity(0)
    , m_simulationTime(0)
    , m_dirty(true)
{
    m_time.start();

    QTimer *timer = new QTimer(this);
    timer->setInterval(20);
    timer->start();
    connect(timer, SIGNAL(timeout()), this, SLOT(move()));
}

void MazeScene::addWall(const QPointF &a, const QPointF &b)
{
    WallItem *item = new WallItem(a, b);
    addItem(item);
    m_walls << item;

    setSceneRect(-1, -1, 2, 2);
}

WallItem::WallItem(const QPointF &a, const QPointF &b)
    : m_a(a)
    , m_b(b)
{
    static const char *urls[] =
    {
        "http://www.google.com",
        "http://www.youtube.com",
        "http://programming.reddit.com",
        "http://www.trolltech.com",
        "http://www.planetkde.org"
    };

    if ((qrand() % 100) >= 10) {
        m_webItem = 0;
        return;
    }

    static int index = 0;
    const char *url = urls[(index++) % (sizeof(urls)/sizeof(char*))];

    m_webItem = new QGraphicsProxyWidget(this);

    QWebView *view = new QWebView;
    view->setUrl(QUrl(url));
    m_webItem->setWidget(view);
    m_webItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
}

QRectF WallItem::boundingRect() const
{
    return QRectF(-0.5, -0.5, 1, 1);
}

void WallItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    static QPixmap pm("wall_ceilingtile.jpg");
    painter->drawPixmap(boundingRect(), pm, pm.rect());
}

static inline QTransform rotatingTransform(qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return transform;
}

static void updateTransform(WallItem *item, const QPointF &a, const QPointF &b, const QPointF &cameraPos, qreal cameraAngle)
{
    const QTransform rotation = rotatingTransform(cameraAngle);

    QPointF ca = rotation.map(a - cameraPos);
    QPointF cb = rotation.map(b - cameraPos);

    if (ca.y() <= 0 && cb.y() <= 0) {
        item->setVisible(false);
        return;
    }

    const qreal mx = ca.x() - cb.x();
    const qreal tx = 0.5 * (ca.x() + cb.x());
    const qreal mz = ca.y() - cb.y();
    const qreal tz = 0.5 * (ca.y() + cb.y());
    const qreal fov = 0.5;

    QTransform project(mx, 0, mz * fov, 0, 1, 0, tx, 0, tz * fov);

    item->setVisible(true);
    item->setZValue(-tz);
    item->setTransform(project);

    if (item->webItem()) {
        QRectF rect = item->webItem()->boundingRect();
        QPointF center = rect.center();

        item->webItem()->setVisible(true);
        item->webItem()->setZValue(-tz);
        item->webItem()->resetMatrix();
        item->webItem()->scale(0.7 / rect.width(), 0.7 / rect.height());
        item->webItem()->translate(-center.x(), -center.y());
    }
}

void MazeScene::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat() && handleKey(event->key(), true)) {
        event->accept();
        return;
    }

    QGraphicsScene::keyPressEvent(event);
}

void MazeScene::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat() && handleKey(event->key(), false)) {
        event->accept();
        return;
    }

    QGraphicsScene::keyPressEvent(event);
}

bool MazeScene::handleKey(int key, bool pressed)
{
    switch (key) {
    case Qt::Key_Left:
    case Qt::Key_Right:
        m_turningVelocity = (pressed ? (key == Qt::Key_Left ? -1 : 1) : 0) * 0.5;
        return true;
    case Qt::Key_Up:
    case Qt::Key_Down:
        m_walkingVelocity = (pressed ? (key == Qt::Key_Down ? -1 : 1) : 0) * 0.01;
        return true;
    }

    return false;
}

void MazeScene::move()
{
    long elapsed = m_time.elapsed();
    while (m_simulationTime <= elapsed) {
        m_cameraAngle += m_turningVelocity;
        m_cameraPos += m_walkingVelocity * rotatingTransform(-m_cameraAngle).map(QPointF(0, 1));
        m_simulationTime += 5;

        if (!m_dirty)
            m_dirty = (m_turningVelocity != 0 || m_walkingVelocity != 0);
    }

    if (m_dirty) {
        foreach (WallItem *item, m_walls)
            updateTransform(item, item->a(), item->b(), m_cameraPos, m_cameraAngle);
    }
}
