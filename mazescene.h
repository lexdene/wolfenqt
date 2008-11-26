#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <QTime>

class MazeScene;

class WallItem : public QGraphicsItem
{
public:
    WallItem(MazeScene *scene, const QPointF &a, const QPointF &b);

    QPointF a() const { return m_a; }
    QPointF b() const { return m_b; }

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QGraphicsProxyWidget *childItem() const
    {
        return m_childItem;
    }

    void setDepths(qreal za, qreal zb);

private:
    QPointF m_a;
    QPointF m_b;
    QGraphicsProxyWidget *m_childItem;
    QGraphicsRectItem *m_shadowItem;
};

class MazeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    MazeScene();

    void addWall(const QPointF &a, const QPointF &b);
    void drawBackground(QPainter *painter, const QRectF &rect);

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    bool handleKey(int key, bool pressed);

public slots:
    void move();
    void toggleRenderer();

private:
    QVector<WallItem *> m_walls;

    QPointF m_cameraPos;
    qreal m_cameraAngle;
    qreal m_walkingVelocity;
    qreal m_turningVelocity;

    QTime m_time;
    long m_simulationTime;
    long m_walkTime;
    bool m_dirty;
};
