#include "calculator.h"
#include "ui_calculator.h"

#include <QPushButton>

// ===== V3：实现加减乘除四则运算与计算状态管理 =====
// 用 m_firstOperand / m_operator / m_waitingForSecond 三个状态
// 管理第一个操作数、运算符与第二个操作数的输入过程。

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
    const struct { const char *name; QString op; } opMap[] = {
        {"btnAdd",      QStringLiteral("+")},
        {"btnSubtract", QStringLiteral("-")},
        {"btnMultiply", QStringLiteral("*")},
        {"btnDivide",   QStringLiteral("/")}
    };
    for (const auto &e : opMap) {
        QPushButton *btn = findChild<QPushButton *>(QLatin1String(e.name));
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, e] {
                inputOperator(e.op);
            });
        }
    }
    connect(ui->btnEquals,  &QPushButton::clicked, this, [this] { inputEquals(); });
}

void Calculator::inputDigit(const QString &digit)
{
    QString text = currentText();
    if (m_waitingForSecond) {
        text = digit;
        m_waitingForSecond = false;
    } else if (text == QStringLiteral("0")) {
        text = digit;
    } else {
        text += digit;
    }
    updateDisplay(text);
}

void Calculator::inputOperator(const QString &op)
{
    if (m_operator.isEmpty()) {
        // 首次选择运算符：保存当前显示为第一个操作数
        m_firstOperand = displayValue();
    } else {
        // 已存在运算符：先完成前一次运算，实现连续运算（如 1+2+3）
        bool ok = false;
        m_firstOperand = calculate(m_firstOperand, displayValue(), m_operator, &ok);
        setDisplayValue(m_firstOperand);
    }
    m_operator = op;
    m_waitingForSecond = true;
}

void Calculator::inputEquals()
{
    if (m_operator.isEmpty())
        return;
    bool ok = false;
    double result = calculate(m_firstOperand, displayValue(), m_operator, &ok);
    setDisplayValue(result);
    m_operator.clear();
    m_waitingForSecond = false;
}

void Calculator::inputDot() {}

void Calculator::doBackspace()
{
    QString text = currentText();
    if (m_waitingForSecond) return;
    text.chop(1);
    updateDisplay(text.isEmpty() ? QStringLiteral("0") : text);
}

void Calculator::doClear()
{
    m_firstOperand = 0.0;
    m_operator.clear();
    m_waitingForSecond = false;
    updateDisplay(QStringLiteral("0"));
}

QString Calculator::currentText() const { return ui->displayEdit->text(); }
void Calculator::updateDisplay(const QString &text) { ui->displayEdit->setText(text); }
void Calculator::updateExpression(const QString &expr) { Q_UNUSED(expr); }
QString Calculator::opSymbol(const QString &op) const { Q_UNUSED(op); return QString(); }
double Calculator::displayValue() const { return currentText().toDouble(); }
void Calculator::setDisplayValue(double value) { updateDisplay(formatNumber(value)); }
double Calculator::calculate(double a, double b, const QString &op, bool *ok) const
{
    *ok = true;
    if (op == QStringLiteral("+")) return a + b;
    if (op == QStringLiteral("-")) return a - b;
    if (op == QStringLiteral("*")) return a * b;
    if (op == QStringLiteral("/")) return a / b;
    *ok = false;
    return 0.0;
}
QString Calculator::formatNumber(double value) const
{
    return QString::number(value, 'g', 12);
}
void Calculator::enterErrorState(const QString &msg) { Q_UNUSED(msg); }
