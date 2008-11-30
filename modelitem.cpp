/****************************************************************************
**
** Copyright (C) 2008-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Graphics Dojo project on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include "modelitem.h"
#include "model.h"

#include <QtGui>
#include <QtOpenGL>

#include "mazescene.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

void ModelItem::updateTransform(const Camera &camera, qreal time)
{
    ProjectedItem::updateTransform(camera, time);

    setTransform(QTransform());
    m_matrix = Matrix4x4::fromTranslation(3, 0, 7) * camera.matrix(time);
}

QRectF ModelItem::boundingRect() const
{
    return scene()->sceneRect();
}

void ModelItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!m_model)
        return;

    Matrix4x4 projectionMatrix = Matrix4x4::fromProjection(1);
    projectionMatrix *= Matrix4x4::fromQTransform(painter->transform());

    Matrix4x4 modelMatrix;
    const int delta = m_time.elapsed() - m_lastTime;
    m_rotation += m_angularMomentum * (delta / 1000.0);
    m_lastTime += delta;

    Point3d size = m_model->size();
    float extent = qSqrt(2.0);
    float scale = 1 / qMax(size.y, qMax(size.x / extent, size.z / extent));
    modelMatrix *= Matrix4x4::fromScale(scale, -scale, scale);

    modelMatrix *= Matrix4x4::fromRotation(m_rotation.z, Qt::ZAxis);
    modelMatrix *= Matrix4x4::fromRotation(m_rotation.y, Qt::YAxis);
    modelMatrix *= Matrix4x4::fromRotation(m_rotation.x, Qt::XAxis);

    ProjectedItem::update();

    if (painter->paintEngine()->type() != QPaintEngine::OpenGL) {
        m_wireframe->setEnabled(false);
        m_wireframe->setChecked(false);
        m_wireframeEnabled = false;
        painter->setTransform(QTransform());
        painter->setPen(m_modelColor);
        m_model->render(painter, modelMatrix * m_matrix * projectionMatrix, m_normalsEnabled);
        return;
    }

    m_wireframe->setEnabled(true);

    glClearDepth(0);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, painter->device()->width(), painter->device()->height(), 0, -1, 1);
    glMultMatrixf(projectionMatrix.data());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(m_matrix.data());

    const float ambient[] = { 0.1, 0.1, 0.1, 1 };
    const float pos[] = { 50, -500, 200 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);

    glMultMatrixf(modelMatrix.data());

    glColor4f(m_modelColor.redF(), m_modelColor.greenF(), m_modelColor.blueF(), 1.0f);

    glEnable(GL_MULTISAMPLE);
    m_model->render(m_wireframeEnabled, m_normalsEnabled);
    glDisable(GL_MULTISAMPLE);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}


ModelItem::ModelItem()
    : ProjectedItem(QRectF(), false)
    , m_wireframeEnabled(false)
    , m_normalsEnabled(false)
    , m_modelColor(153, 255, 0)
    , m_model(0)
    , m_lastTime(0)
    , m_distance(1.4f)
    , m_angularMomentum(0, 40, 0)
{
    QPointF pos(3, 7);
    setPosition(pos, pos);
    setLayout(new QVBoxLayout);

    m_modelButton = new QPushButton(tr("Load model"));
    connect(m_modelButton, SIGNAL(clicked()), this, SLOT(loadModel()));
#ifndef QT_NO_CONCURRENT
    connect(&m_modelLoader, SIGNAL(finished()), this, SLOT(modelLoaded()));
#endif
    layout()->addWidget(m_modelButton);

    m_wireframe = new QCheckBox(tr("Render as wireframe"));
    connect(m_wireframe, SIGNAL(toggled(bool)), this, SLOT(enableWireframe(bool)));
    layout()->addWidget(m_wireframe);

    QCheckBox *normals = new QCheckBox(tr("Display normals vectors"));
    connect(normals, SIGNAL(toggled(bool)), this, SLOT(enableNormals(bool)));
    layout()->addWidget(normals);

    QPushButton *colorButton = new QPushButton(tr("Choose model color"));
    connect(colorButton, SIGNAL(clicked()), this, SLOT(setModelColor()));
    layout()->addWidget(colorButton);

    loadModel(QLatin1String("qt.obj"));
    m_time.start();
}

static Model *loadModel(const QString &filePath)
{
    return new Model(filePath);
}

void ModelItem::loadModel()
{
    loadModel(QFileDialog::getOpenFileName(0, tr("Choose model"), QString(), QLatin1String("*.obj")));
}

void ModelItem::loadModel(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    m_modelButton->setEnabled(false);
    QApplication::setOverrideCursor(Qt::BusyCursor);
#ifndef QT_NO_CONCURRENT
    m_modelLoader.setFuture(QtConcurrent::run(::loadModel, filePath));
#else
    setModel(::loadModel(filePath));
    modelLoaded();
#endif
}

void ModelItem::modelLoaded()
{
#ifndef QT_NO_CONCURRENT
    setModel(m_modelLoader.result());
#endif
    m_modelButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void ModelItem::setModel(Model *model)
{
    delete m_model;
    m_model = model;

    ProjectedItem::update();
}

void ModelItem::enableWireframe(bool enabled)
{
    m_wireframeEnabled = enabled;
    ProjectedItem::update();
}

void ModelItem::enableNormals(bool enabled)
{
    m_normalsEnabled = enabled;
    ProjectedItem::update();
}

void ModelItem::setModelColor()
{
    const QColor color = QColorDialog::getColor(m_modelColor);
    if (color.isValid()) {
        m_modelColor = color;
        ProjectedItem::update();
    }
}