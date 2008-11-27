#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointF>
#include <QTime>
#include <QTimeLine>

class MazeScene;

class View : public QGraphicsView
{
public:
    View();
    void resizeEvent(QResizeEvent *event);
};

class WallItem : public QGraphicsItem
{
public:
    WallItem(MazeScene *scene, const QPointF &a, const QPointF &b, int type);

    QPointF a() const { return m_a; }
    QPointF b() const { return m_b; }

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QGraphicsProxyWidget *childItem() const
    {
        return m_childItem;
    }

    int type() const { return m_type; }

    void setDepths(qreal za, qreal zb);
    void setAnimationTime(qreal time);

private:
    QPointF m_a;
    QPointF m_b;
    QGraphicsProxyWidget *m_childItem;
    QGraphicsRectItem *m_shadowItem;
    int m_type;
    QRectF m_targetRect;
};

class MazeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    MazeScene(const char *map, int width, int height);

    void addWall(const QPointF &a, const QPointF &b, int type);
    void drawBackground(QPainter *painter, const QRectF &rect);

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    bool handleKey(int key, bool pressed);

public slots:
    void move();
    void toggleRenderer();
    void toggleDoors();

private slots:
    void moveDoors(qreal value);

private:
    QVector<WallItem *> m_walls;
    QVector<WallItem *> m_doors;

    QPointF m_cameraPos;
    qreal m_cameraAngle;
    qreal m_walkingVelocity;
    qreal m_turningVelocity;

    QTime m_time;
    QTimeLine *m_doorAnimation;
    long m_simulationTime;
    long m_walkTime;
    bool m_dirty;
};
