#include "calculator.h"
#include "ui_calculator.h"

// ===== V1：创建 Qt 工程骨架，通过 UI 设计器完成界面布局 =====
// 界面（显示框、17 个按钮、QGridLayout 网格布局）全部在 calculator.ui 中完成，
// 控件属性（只读、右对齐、按钮文本等）也由 UI 设计器设置。

Calculator::Calculator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Calculator)
{
    ui->setupUi(this);
    setWindowTitle("带键盘事件的计算器");
}

Calculator::~Calculator()
{
    delete ui;
}

void Calculator::applyStyleSheet() {}

void Calculator::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
}

void Calculator::connectButtons() {}

void Calculator::inputDigit(const QString &digit) { Q_UNUSED(digit); }
void Calculator::inputOperator(const QString &op)  { Q_UNUSED(op); }
void Calculator::inputDot() {}
void Calculator::inputEquals() {}
void Calculator::doBackspace() {}
void Calculator::doClear() {}

QString Calculator::currentText() const { return ui->displayEdit->text(); }
void Calculator::updateDisplay(const QString &text) { ui->displayEdit->setText(text); }
void Calculator::updateExpression(const QString &expr) { Q_UNUSED(expr); }
QString Calculator::opSymbol(const QString &op) const { Q_UNUSED(op); return QString(); }
double Calculator::displayValue() const { return currentText().toDouble(); }
void Calculator::setDisplayValue(double value) { updateDisplay(QString::number(value)); }
double Calculator::calculate(double a, double b, const QString &op, bool *ok) const
{
    Q_UNUSED(a); Q_UNUSED(b); Q_UNUSED(op);
    *ok = false;
    return 0.0;
}
QString Calculator::formatNumber(double value) const { return QString::number(value); }
void Calculator::enterErrorState(const QString &msg) { Q_UNUSED(msg); }
