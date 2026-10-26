#include "calculator.h"
#include "ui_calculator.h"

#include <QPushButton>

// ===== V4：添加小数输入，处理连续操作符、重复小数点、除零等异常输入 =====

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
    connect(ui->btnDot,     &QPushButton::clicked, this, [this] { inputDot(); });
    connect(ui->btnEquals,  &QPushButton::clicked, this, [this] { inputEquals(); });
    connect(ui->btnBackspace, &QPushButton::clicked, this, [this] { doBackspace(); });
    connect(ui->btnClear,   &QPushButton::clicked, this, [this] { doClear(); });
}

void Calculator::inputDigit(const QString &digit)
{
    if (m_error) return;
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
    if (m_error) return;
    if (m_waitingForSecond) {
        // 连续操作符（如 1 + + 2）：用新运算符覆盖旧运算符
        m_operator = op;
        return;
    }
    if (m_operator.isEmpty()) {
        m_firstOperand = displayValue();
    } else {
        bool ok = false;
        double result = calculate(m_firstOperand, displayValue(), m_operator, &ok);
        if (!ok) {
            enterErrorState(QStringLiteral("错误：除数不能为 0"));
            return;
        }
        m_firstOperand = result;
        setDisplayValue(result);
    }
    m_operator = op;
    m_waitingForSecond = true;
}

void Calculator::inputDot()
{
    if (m_error) return;
    if (m_waitingForSecond) {
        updateDisplay(QStringLiteral("0."));
        m_waitingForSecond = false;
        return;
    }
    QString text = currentText();
    // 关键：同一操作数只允许一个小数点，重复输入直接忽略
    if (text.contains(QLatin1Char('.')))
        return;
    updateDisplay(text + QLatin1Char('.'));
}

void Calculator::inputEquals()
{
    if (m_error) return;
    if (m_operator.isEmpty()) return;
    bool ok = false;
    double result = calculate(m_firstOperand, displayValue(), m_operator, &ok);
    if (!ok) {
        enterErrorState(QStringLiteral("错误：除数不能为 0"));
        return;
    }
    setDisplayValue(result);
    m_operator.clear();
    m_waitingForSecond = false;
}

void Calculator::doBackspace()
{
    if (m_error || m_waitingForSecond) return;
    QString text = currentText();
    text.chop(1);
    updateDisplay(text.isEmpty() ? QStringLiteral("0") : text);
}

void Calculator::doClear()
{
    m_firstOperand = 0.0;
    m_operator.clear();
    m_waitingForSecond = false;
    m_error = false;
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
    if (op == QStringLiteral("/")) {
        if (b == 0.0) {          // 除零保护
            *ok = false;
            return 0.0;
        }
        return a / b;
    }
    *ok = false;
    return 0.0;
}
QString Calculator::formatNumber(double value) const
{
    return QString::number(value, 'g', 12);
}
void Calculator::enterErrorState(const QString &msg)
{
    m_error = true;
    m_operator.clear();
    m_waitingForSecond = false;
    updateDisplay(msg);
}
