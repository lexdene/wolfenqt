#include <QtGui>

#include "mazescene.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MazeScene *scene = new MazeScene;

    QGraphicsView view;
    view.resize(640, 480);
    view.setScene(scene);
    view.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    view.show();

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

    view.scale(200, 200);
    return app.exec();
}
