#include "mazescene.h"

#include <QCheckBox>
#include <QGLWidget>
#include <QGraphicsView>
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
    WallItem *item = new WallItem(this, a, b);
    addItem(item);
    m_walls << item;

    setSceneRect(-1, -1, 2, 2);
}

WallItem::WallItem(MazeScene *scene, const QPointF &a, const QPointF &b)
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
        m_childItem = 0;
        return;
    }

    m_childItem = new QGraphicsProxyWidget(this);

    static int index = 0;
    if (index == 0) {
        QWidget *widget = new QWidget;

        QCheckBox *checkBox = new QCheckBox("Use OpenGL", widget);
        checkBox->setChecked(true);
        QObject::connect(checkBox, SIGNAL(toggled(bool)), scene, SLOT(toggleRenderer()), Qt::QueuedConnection);

        QPalette palette;
        palette.setColor(QPalette::Window, QColor(Qt::transparent));
        widget->setPalette(palette);

        m_childItem->setWidget(widget);
    } else {
        const char *url = urls[index % (sizeof(urls)/sizeof(char*))];

        QWebView *view = new QWebView;
        view->setUrl(QUrl(url));

        m_childItem->setWidget(view);
    }

    m_childItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    QRectF rect = m_childItem->boundingRect();
    QPointF center = rect.center();

    const qreal scale = index ? 0.7 : 0.1;

    m_childItem->scale(scale / rect.width(), scale / rect.height());
    m_childItem->translate(-center.x(), -center.y());

    ++index;
}

QRectF WallItem::boundingRect() const
{
    return QRectF(-0.5, -0.5, 1, 1);
}

void WallItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    static QImage img = QImage("wall_ceilingtile.jpg").convertToFormat(QImage::Format_RGB32);
    painter->drawImage(boundingRect(), img, img.rect());
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

#if 0
    QGraphicsProxyWidget *child = item->childItem();
    if (child) {
        //QRectF rect = child->boundingRect();
        //QPointF center = rect.center();

        //child->setZValue(-tz);

        //child->resetMatrix();
        //child->scale(0.7 / rect.width(), 0.7 / rect.height());
        //child->translate(-center.x(), -center.y());
    }
#endif
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

void MazeScene::toggleRenderer()
{
    QGraphicsView *view = views().at(0);
    if (view->viewport()->inherits("QGLWidget"))
        view->setViewport(new QWidget);
    else
        view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
}
