#include <QtGui>
#include "mazescene.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("WolfenQt");
    QPixmapCache::setCacheLimit(100 * 1024); // 100 MB

    const char *map =
        "###&?###"
        "#      #"
        "=      &"
        "#      #"
        "# #### #"
        "&    # #"
        "# @@   &"
        "# @@ # #"
        "#      #"
        "###&%-##"
        "$      !"
        "#&&&&&&#";

    QVector<Light> lights;
    lights << Light(QPointF(3.5, 2.5), 1)
           << Light(QPointF(3.5, 6.5), 1)
           << Light(QPointF(1.5, 10.5), 0.3);

    MazeScene *scene = new MazeScene(lights, map, 8, 12);

    View view;
    view.setScene(scene);
    view.show();

    return app.exec();
}
