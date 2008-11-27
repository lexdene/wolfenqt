#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointF>
#include <QPushButton>
#include <QTime>
#include <QTimeLine>

class MazeScene;

class View : public QGraphicsView
{
    Q_OBJECT
public:
    View();
    void resizeEvent(QResizeEvent *event);
};

class ProjectedItem : public QGraphicsItem
{
public:
    ProjectedItem(const QRectF &bounds, bool shadow = true);

    QPointF a() const { return m_a; }
    QPointF b() const { return m_b; }

    QRectF boundingRect() const;

    void setPosition(const QPointF &a, const QPointF &b);
    void updateTransform(const QPointF &cameraPos, qreal cameraRotation, qreal time);
    void setDepths(qreal za, qreal zb);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void setAnimationTime(qreal time);
    void setImage(const QImage &image);

private:
    QPointF m_a;
    QPointF m_b;
    QRectF m_bounds;
    QRectF m_targetRect;
    QImage m_image;
    QGraphicsRectItem *m_shadowItem;
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

private:
    QGraphicsProxyWidget *m_childItem;
    int m_type;
};

class Entity : public ProjectedItem
{
public:
    Entity(const QPointF &pos);
    void updateTransform(const QPointF &cameraPos, qreal cameraRotation, qreal time);

private:
    QPointF m_pos;
};

class MazeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    MazeScene(const char *map, int width, int height);

    void addEntity(Entity *entity);
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
    QVector<QGraphicsItem *> m_floorTiles;
    QVector<QPushButton *> m_buttons;
    QVector<Entity *> m_entities;

    QPointF m_cameraPos;
    qreal m_cameraAngle;
    qreal m_walkingVelocity;
    qreal m_turningVelocity;

    QTime m_time;
    QTimeLine *m_doorAnimation;
    long m_simulationTime;
    long m_walkTime;
    bool m_dirty;
    int m_width;
    int m_height;
};
