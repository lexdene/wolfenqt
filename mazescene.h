#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <QTime>

class WebItem;

class WallItem : public QGraphicsItem
{
public:
    WallItem(const QPointF &a, const QPointF &b);

    QPointF a() const { return m_a; }
    QPointF b() const { return m_b; }

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QGraphicsProxyWidget *webItem() const
    {
        return m_webItem;
    }

private:
    QPointF m_a;
    QPointF m_b;
    QGraphicsProxyWidget *m_webItem;
};

class MazeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    MazeScene();

    void addWall(const QPointF &a, const QPointF &b);

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    bool handleKey(int key, bool pressed);

public slots:
    void move();

private:
    QVector<WallItem *> m_walls;

    QPointF m_cameraPos;
    qreal m_cameraAngle;
    qreal m_walkingVelocity;
    qreal m_turningVelocity;

    QTime m_time;
    long m_simulationTime;
    bool m_dirty;
};
