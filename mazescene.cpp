#include "mazescene.h"

#include <QCheckBox>
#include <QGLWidget>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QKeyEvent>
#include <QTimer>
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
    , m_dirty(true)
{
    QMap<char, int> types;
    types[' '] = -1;
    types['#'] = 0;
    types['&'] = 1;
    types['@'] = 2;

    int type;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < height; ++x) {
            if (map[y*width+x] != ' ')
                continue;

            type = types[map[(y-1)*width+x]];
            if (type >= 0)
                addWall(QPointF(x, y), QPointF(x+1, y), type);

            type = types[map[(y+1)*width+x]];
            if (type >= 0)
                addWall(QPointF(x+1, y+1), QPointF(x, y+1), type);

            type = types[map[y*width+x-1]];
            if (type >= 0)
                addWall(QPointF(x, y+1), QPointF(x, y), type);

            type = types[map[y*width+x+1]];
            if (type >= 0)
                addWall(QPointF(x+1, y), QPointF(x+1, y+1), type);
        }
    }


    QTimer *timer = new QTimer(this);
    timer->setInterval(20);
    timer->start();
    connect(timer, SIGNAL(timeout()), this, SLOT(move()));

    m_time.start();
    move();
}

void MazeScene::addWall(const QPointF &a, const QPointF &b, int type)
{
    WallItem *item = new WallItem(this, a, b, type);
    addItem(item);
    m_walls << item;

    setSceneRect(-1, -1, 2, 2);
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
    painter->fillRect(rect, g);
}

WallItem::WallItem(MazeScene *scene, const QPointF &a, const QPointF &b, int type)
    : m_a(a)
    , m_b(b)
    , m_type(type)
{
    static const char *urls[] =
    {
        "http://www.google.com",
        "http://www.youtube.com",
        "http://programming.reddit.com",
        "http://www.trolltech.com",
        "http://www.planetkde.org",
        "http://chaos.troll.no/~tavestbo/webkit"
    };

    m_shadowItem = new QGraphicsRectItem(boundingRect(), this);
    m_shadowItem->setPen(Qt::NoPen);
    m_shadowItem->setZValue(10);

    static int index = 0;
    if (type == 1 || index >= 4 && (qrand() % 100) >= 20) {
        m_childItem = 0;
        return;
    }

    m_childItem = new QGraphicsProxyWidget(this);

    if (index == 0) {
        QWidget *widget = new QWidget;

        QCheckBox *checkBox = new QCheckBox("Use OpenGL", widget);
        checkBox->setChecked(true);
        QObject::connect(checkBox, SIGNAL(toggled(bool)), scene, SLOT(toggleRenderer()), Qt::QueuedConnection);

        QPalette palette;
        palette.setColor(QPalette::Window, QColor(Qt::transparent));
        widget->setPalette(palette);

        m_childItem->setWidget(widget);
    } else if (index < 4) {
        ++index;
        const char *map = "#####"
                          "#   #"
                          "# @ #"
                          "#   #"
                          "#####";
        MazeScene *embeddedScene = new MazeScene(map, 5, 5);
        View *view = new View;
        view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        view->setScene(embeddedScene);
        view->resize(480, 320); // not soo big
        view->setViewport(new QWidget); // no OpenGL here
        m_childItem->setWidget(view);
    } else {
        const char *url = urls[index % (sizeof(urls)/sizeof(char*))];

        QWebView *view = new QWebView;
        view->setUrl(QUrl(url));

        m_childItem->setWidget(view);
    }

    m_childItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    QRectF rect = m_childItem->boundingRect();
    QPointF center = rect.center();

    qreal scale = index ? 0.8 : 0.2;

    scale = qMin(scale / rect.width(), scale / rect.height());
    m_childItem->scale(scale, scale);
    m_childItem->translate(-center.x(), -center.y());

    ++index;
}

void WallItem::setDepths(qreal za, qreal zb)
{
    const qreal falloff = 40;
    const int maxAlpha = 180;
    int va = int(falloff * zb);
    int vb = int(falloff * za);

    if (va >= maxAlpha && vb >= maxAlpha) {
        m_shadowItem->setBrush(QColor(0, 0, 0, maxAlpha));
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

QRectF WallItem::boundingRect() const
{
    return QRectF(-0.5, -0.5, 1, 1);
}

void WallItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    static QImage brown = QImage("brown.gif").convertToFormat(QImage::Format_RGB32);
    static QImage book = QImage("book.gif").convertToFormat(QImage::Format_RGB32);
    if (m_type != 2) {
        if (m_type == 1) {
            painter->drawImage(boundingRect(), book, book.rect());
        } else {
            painter->drawImage(boundingRect(), brown, brown.rect());
        }
    }
}

static inline QTransform rotatingTransform(qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return transform;
}

static void updateTransform(WallItem *item, const QPointF &a, const QPointF &b, const QPointF &cameraPos, qreal cameraAngle, qreal time)
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

    QTransform project(mx, 0, mz * fov, 0, 1, 0, tx, 0.04 * qSin(10 * time), tz * fov);

    item->setVisible(true);
    item->setZValue(-tz);
    item->setTransform(project);

    item->setDepths(QLineF(QPointF(), ca).length(), QLineF(QPointF(), cb).length());
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

        bool walking = m_walkingVelocity != 0;
        bool turning = m_turningVelocity != 0;
        if (!m_dirty)
            m_dirty = turning || walking;

        if (walking)
            m_walkTime += 5;
    }

    if (m_dirty) {
        m_dirty = false;
        foreach (WallItem *item, m_walls)
            updateTransform(item, item->a(), item->b(), m_cameraPos, m_cameraAngle, m_walkTime * 0.001);
        setFocusItem(0); // setVisible(true) might give focus to one of the items
    }
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
