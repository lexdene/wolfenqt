#include <QtGui>
#include <QGLWidget>

#include "mazescene.h"

class View : public QGraphicsView
{
public:
    View();
    void resizeEvent(QResizeEvent *event);
};

View::View()
{
    resize(1024, 768);
    setViewport(new QGLWidget);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

void View::resizeEvent(QResizeEvent *event)
{
    resetMatrix();

    qreal factor = width() / 4.0;
    scale(factor, factor);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QPixmapCache::setCacheLimit(100 * 1024); // 100 MB

    MazeScene *scene = new MazeScene;

    const char *map =
        "########"
        "#      #"
        "# #### #"
        "#    # #"
        "# ##   #"
        "# ## # #"
        "#      #"
        "########";

    const int width = 8;
    const int height = 8;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < height; ++x) {
            if (map[y*width+x] != ' ')
                continue;

            if (map[(y-1)*width+x] != ' ')
                scene->addWall(QPointF(x, y), QPointF(x+1, y));
            if (map[(y+1)*width+x] != ' ')
                scene->addWall(QPointF(x+1, y+1), QPointF(x, y+1));
            if (map[y*width+x-1] != ' ')
                scene->addWall(QPointF(x, y+1), QPointF(x, y));
            if (map[y*width+x+1] != ' ')
                scene->addWall(QPointF(x+1, y), QPointF(x+1, y+1));
        }
    }

    View view;
    view.setScene(scene);
    view.show();

    return app.exec();
}
