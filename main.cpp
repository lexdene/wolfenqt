/****************************************************************************
**
** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Graphics Dojo project on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

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
        "* @@   &"
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
    view.resize(800, 600);
    view.setScene(scene);
    view.show();

    scene->toggleRenderer();

    return app.exec();
}
