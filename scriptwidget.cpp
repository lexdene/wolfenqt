/****************************************************************************

This file is part of the wolfenqt project on http://qt.gitorious.org.

Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).*
All rights reserved.

Contact:  Nokia Corporation (qt-info@nokia.com)**

You may use this file under the terms of the BSD license as follows:

"Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* Neither the name of Nokia Corporation and its Subsidiary(-ies) nor the
* names of its contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE."

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
