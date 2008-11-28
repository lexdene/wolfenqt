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

    MazeScene *scene = new MazeScene(map, 8, 12);

    View view;
    view.setScene(scene);
    view.show();

    return app.exec();
}
