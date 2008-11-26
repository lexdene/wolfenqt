#include <QtGui>
#include "mazescene.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QPixmapCache::setCacheLimit(100 * 1024); // 100 MB

    const char *map =
        "#####&##"
        "#      #"
        "# #### #"
        "&    # #"
        "# @@   &"
        "# @@ # #"
        "#      #"
        "###&####";

    const int width = 8;
    const int height = 8;

    MazeScene *scene = new MazeScene(map, width, height);

    View view;
    view.setScene(scene);
    view.show();

    return app.exec();
}
