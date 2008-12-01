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

#include "scriptwidget.h"
#include "mazescene.h"
#include "entity.h"

static QScriptValue qsRand(QScriptContext *, QScriptEngine *engine)
{
    QScriptValue value(engine, qrand() / (RAND_MAX + 1.0));
    return value;
}

void ScriptWidget::setPreset(int preset)
{
    const char *presets[] =
    {
        "// available functions:\n"
        "// entity.turnLeft()\n"
        "// entity.turnRight()\n"
        "// entity.turnTowards(x, y)\n"
        "// entity.walk()\n"
        "// entity.stop()\n"
        "// rand()\n"
        "// script.display()\n"
        "\n"
        "// available variables:\n"
        "// my_x\n"
        "// my_y\n"
        "// player_x\n"
        "// player_y\n"
        "// time\n"
        "\n"
        "entity.stop();\n",
        "entity.walk();\n"
        "if ((time % 20000) < 10000) {\n"
        "  entity.turnTowards(10, 2.5);\n"
        "  if (my_x >= 5.5)\n"
        "    entity.stop();\n"
        "} else {\n"
        "  entity.turnTowards(-10, 2.5);\n"
        "  if (my_x <= 2.5)\n"
        "    entity.stop();\n"
        "}\n",
        "dx = player_x - my_x;\n"
        "dy = player_y - my_y;\n"
        "if (dx * dx + dy * dy < 5) {\n"
        "  entity.stop();\n"
        "} else {\n"
        "  entity.walk();\n"
        "  entity.turnTowards(player_x, player_y);\n"
        "}\n"
    };

    m_sourceEdit->setPlainText(QLatin1String(presets[preset]));
}

ScriptWidget::ScriptWidget(MazeScene *scene, Entity *entity)
    : m_scene(scene)
    , m_entity(entity)
{
    new QVBoxLayout(this);

    m_statusView = new QLineEdit;
    m_statusView->setReadOnly(true);
    layout()->addWidget(m_statusView);

    m_sourceEdit = new QPlainTextEdit;
    layout()->addWidget(m_sourceEdit);

    QPushButton *compileButton = new QPushButton(QLatin1String("Compile"));
    layout()->addWidget(compileButton);

    QComboBox *combo = new QComboBox;
    layout()->addWidget(combo);

    combo->addItem(QLatin1String("Default"));
    combo->addItem(QLatin1String("Patrol"));
    combo->addItem(QLatin1String("Follow"));

    setPreset(0);
    connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(setPreset(int)));
    connect(compileButton, SIGNAL(clicked()), this, SLOT(updateSource()));

    m_engine = new QScriptEngine(this);
    QScriptValue entityObject = m_engine->newQObject(m_entity);
    m_engine->globalObject().setProperty("entity", entityObject);
    QScriptValue widgetObject = m_engine->newQObject(this);
    m_engine->globalObject().setProperty("script", widgetObject);
    m_engine->globalObject().setProperty("rand", m_engine->newFunction(qsRand));

    m_engine->setProcessEventsInterval(5);

    resize(300, 400);
    updateSource();

    startTimer(50);
    m_time.start();
}

void ScriptWidget::timerEvent(QTimerEvent *)
{
    QPointF player = m_scene->camera().pos();
    QPointF entity = m_entity->pos();

    QScriptValue px(m_engine, player.x());
    QScriptValue py(m_engine, player.y());
    QScriptValue ex(m_engine, entity.x());
    QScriptValue ey(m_engine, entity.y());
    QScriptValue time(m_engine, m_time.elapsed());

    m_engine->globalObject().setProperty("player_x", px);
    m_engine->globalObject().setProperty("player_y", py);
    m_engine->globalObject().setProperty("my_x", ex);
    m_engine->globalObject().setProperty("my_y", ey);
    m_engine->globalObject().setProperty("time", time);

    m_engine->evaluate(m_source);
    if (m_engine->hasUncaughtException()) {
        QString text = m_engine->uncaughtException().toString();
        m_statusView->setText(text);
    }
}

void ScriptWidget::display(QScriptValue value)
{
    m_statusView->setText(value.toString());
}

void ScriptWidget::updateSource()
{
    bool wasEvaluating = m_engine->isEvaluating();
    if (wasEvaluating)
        m_engine->abortEvaluation();

    m_time.restart();
    m_source = m_sourceEdit->toPlainText();
    if (wasEvaluating)
        m_statusView->setText(QLatin1String("Aborted long running evaluation!"));
    else if (m_engine->canEvaluate(m_source))
        m_statusView->setText(QLatin1String("Evaluation succeeded"));
    else
        m_statusView->setText(QLatin1String("Evaluation failed"));
}
