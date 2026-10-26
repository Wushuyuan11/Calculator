#include "calculator.h"
#include "ui_calculator.h"

#include <QPushButton>

// ===== V2：实现数字输入、退格与清除功能 =====
// 数字按钮统一通过对象名 btn0...btn9 查找并连接到 inputDigit，
// 避免编写 10 段重复代码。

Calculator::Calculator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Calculator)
{
    ui->setupUi(this);
    setWindowTitle("带键盘事件的计算器");
    connectButtons();
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

void Calculator::connectButtons()
{
    for (int i = 0; i <= 9; ++i) {
        QPushButton *btn = findChild<QPushButton *>(QStringLiteral("btn%1").arg(i));
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, i] {
                inputDigit(QString::number(i));
            });
        }
    }
}

void Calculator::inputDigit(const QString &digit)
{
    QString text = currentText();
    // 显示为 0 时用数字替换，避免出现 "07" 前导零
    text = (text == QStringLiteral("0")) ? digit : text + digit;
    updateDisplay(text);
}

void Calculator::inputOperator(const QString &op) { Q_UNUSED(op); }
void Calculator::inputDot() {}
void Calculator::inputEquals() {}

void Calculator::doBackspace()
{
    QString text = currentText();
    text.chop(1);
    updateDisplay(text.isEmpty() ? QStringLiteral("0") : text);
}

void Calculator::doClear()
{
    updateDisplay(QStringLiteral("0"));
}

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
