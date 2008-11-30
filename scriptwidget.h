#ifndef SCRIPTWIDGET_H
#define SCRIPTWIDGET_H

#include <QScriptEngine>
#include <QtGui>

class MazeScene;
class Entity;

class ScriptWidget : public QWidget
{
    Q_OBJECT
public:
    ScriptWidget(MazeScene *scene, Entity *entity);

public slots:
    void display(QScriptValue value);

private slots:
    void updateSource();
    void setPreset(int preset);

protected:
    void timerEvent(QTimerEvent *event);

private:
    MazeScene *m_scene;
    Entity *m_entity;
    QScriptEngine *m_engine;
    QPlainTextEdit *m_sourceEdit;
    QLineEdit *m_statusView;
    QString m_source;
    QTime m_time;
};

#endif
