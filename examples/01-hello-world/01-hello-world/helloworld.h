#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class HelloWorld;
}
QT_END_NAMESPACE

class HelloWorld : public QMainWindow
{
    Q_OBJECT

public:
    HelloWorld(QWidget *parent = nullptr);
    ~HelloWorld();

private:
    Ui::HelloWorld *ui;
};
#endif // HELLOWORLD_H
